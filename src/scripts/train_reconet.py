"""
train_reconet.py.bak

Main training script for ReCoNet style transfer with temporal coherence

PRE-RESEARCH VERSION - before feature temporal loss and luminance constraint
Uses: ConvTranspose2d decoder, simple temporal loss, balanced style weights

License: GPL3
"""

import argparse
import json
import os
import sys
import time
import glob
import io
import random
from PIL import Image
import numpy as np

# Fix Windows console encoding for PyTorch ONNX export messages
if sys.platform == 'win32':
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', errors='replace')

import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import Dataset, DataLoader

from reconet_model import ReCoNet, ReCoNetExport
from loss_functions import (
    PerceptualLoss, total_variation_loss,
    compute_optical_flow, compute_occlusion_mask,
    output_temporal_loss, feature_temporal_loss,
    HAS_OPENCV
)


def load_and_preprocess_image(image_path, resolution):
    img = Image.open(image_path).convert('RGB')
    img = img.resize((resolution, resolution), Image.LANCZOS)
    img_array = np.array(img).astype(np.float32) / 255.0
    img_array = img_array * 2.0 - 1.0
    img_tensor = torch.from_numpy(img_array).permute(2, 0, 1)
    return img_tensor


class ContentDataset(Dataset):
    def __init__(self, folder_path, resolution):
        self.resolution = resolution
        self.image_paths = []
        if os.path.isdir(folder_path):
            for ext in ['*.jpg', '*.jpeg', '*.png', '*.bmp', '*.webp']:
                self.image_paths.extend(glob.glob(os.path.join(folder_path, ext)))
                self.image_paths.extend(glob.glob(os.path.join(folder_path, ext.upper())))
        if len(self.image_paths) == 0:
            raise ValueError(f"No images found in content folder: {folder_path}")
        print(f"[Content Dataset] Found {len(self.image_paths)} content images")

    def __len__(self):
        return len(self.image_paths)

    def __getitem__(self, idx):
        try:
            return load_and_preprocess_image(self.image_paths[idx], self.resolution)
        except:
            return torch.zeros(3, self.resolution, self.resolution)


class StyleDataset(Dataset):
    """
    Style dataset that preserves high resolution for better stroke capture.

    Unlike content images (which must match training resolution), style images
    can be any resolution since Gram matrices are resolution-independent.
    Higher resolution = more brush stroke detail captured.
    """
    def __init__(self, image_paths, resolution, max_style_res=1024):
        """
        Args:
            image_paths: Paths to style images
            resolution: Training resolution (used as minimum)
            max_style_res: Maximum style resolution (for VRAM limits)
        """
        self.image_paths = [p for p in image_paths if os.path.exists(p)]
        self.resolution = resolution
        self.max_style_res = max_style_res
        self.style_images = []

        for path in self.image_paths:
            try:
                img = Image.open(path).convert('RGB')
                orig_w, orig_h = img.size

                # Use HIGH resolution for style - preserve brush stroke detail!
                # Scale to fit within max_style_res while maintaining aspect ratio
                max_dim = max(orig_w, orig_h)
                if max_dim > max_style_res:
                    scale = max_style_res / max_dim
                    new_w = int(orig_w * scale)
                    new_h = int(orig_h * scale)
                    # Ensure dimensions are multiples of 8 for VGG
                    new_w = (new_w // 8) * 8
                    new_h = (new_h // 8) * 8
                    img = img.resize((new_w, new_h), Image.LANCZOS)
                    print(f"[Style] Loading {os.path.basename(path)}: {orig_w}x{orig_h} -> {new_w}x{new_h} (high-res preserved)")
                else:
                    # Keep original size, just ensure multiple of 8
                    new_w = (orig_w // 8) * 8
                    new_h = (orig_h // 8) * 8
                    if new_w != orig_w or new_h != orig_h:
                        img = img.resize((new_w, new_h), Image.LANCZOS)
                    print(f"[Style] Loading {os.path.basename(path)}: {orig_w}x{orig_h} -> {new_w}x{new_h} (original res)")

                # Convert to tensor [-1, 1]
                img_array = np.array(img).astype(np.float32) / 255.0
                img_array = img_array * 2.0 - 1.0
                img_tensor = torch.from_numpy(img_array).permute(2, 0, 1)
                self.style_images.append(img_tensor)

            except Exception as e:
                print(f"[Style] Failed to load {path}: {e}")

        if len(self.style_images) == 0:
            raise ValueError("No valid style images loaded")
        print(f"[Style Dataset] Loaded {len(self.style_images)} style images (HIGH-RES for brush detail)")

    def __len__(self):
        return len(self.style_images)

    def __getitem__(self, idx):
        return self.style_images[idx]


class VideoFrameDataset(Dataset):
    def __init__(self, video_paths, resolution, sequence_length=2):
        self.resolution = resolution
        self.sequence_length = sequence_length
        self.frame_sequences = []
        try:
            import cv2
            self.cv2 = cv2
            self.has_cv2 = True
        except ImportError:
            self.cv2 = None
            self.has_cv2 = False

        for path in video_paths:
            if os.path.isdir(path):
                self._add_frame_folder(path)
            elif os.path.isfile(path) and self.has_cv2:
                self._add_video_file(path)

        if len(self.frame_sequences) == 0:
            raise ValueError("No valid video frame sequences found")
        print(f"[Video Dataset] Found {len(self.frame_sequences)} frame sequences")

    def _add_frame_folder(self, folder_path):
        image_paths = []
        for ext in ['*.jpg', '*.jpeg', '*.png', '*.bmp']:
            # Search subdirectories recursively
            image_paths.extend(glob.glob(os.path.join(folder_path, '**', ext), recursive=True))
            image_paths.extend(glob.glob(os.path.join(folder_path, '**', ext.upper()), recursive=True))
        if len(image_paths) >= self.sequence_length:
            image_paths = sorted(image_paths)
            for i in range(len(image_paths) - self.sequence_length + 1):
                self.frame_sequences.append(('folder', image_paths[i:i + self.sequence_length]))

    def _add_video_file(self, video_path):
        cap = self.cv2.VideoCapture(video_path)
        frame_count = int(cap.get(self.cv2.CAP_PROP_FRAME_COUNT))
        cap.release()
        if frame_count >= self.sequence_length:
            step = max(1, frame_count // 1000)
            for i in range(0, frame_count - self.sequence_length + 1, step):
                self.frame_sequences.append(('video', video_path, i))

    def __len__(self):
        return len(self.frame_sequences)

    def __getitem__(self, idx):
        item = self.frame_sequences[idx]
        frames = []
        if item[0] == 'folder':
            for path in item[1]:
                try:
                    frames.append(load_and_preprocess_image(path, self.resolution))
                except:
                    frames.append(torch.zeros(3, self.resolution, self.resolution))
        elif item[0] == 'video':
            cap = self.cv2.VideoCapture(item[1])
            cap.set(self.cv2.CAP_PROP_POS_FRAMES, item[2])
            for _ in range(self.sequence_length):
                ret, frame = cap.read()
                if ret:
                    frame = self.cv2.cvtColor(frame, self.cv2.COLOR_BGR2RGB)
                    frame = self.cv2.resize(frame, (self.resolution, self.resolution))
                    frame = frame.astype(np.float32) / 255.0 * 2.0 - 1.0
                    frames.append(torch.from_numpy(frame).permute(2, 0, 1))
                else:
                    frames.append(torch.zeros(3, self.resolution, self.resolution))
            cap.release()
        return torch.stack(frames) if frames else torch.zeros(self.sequence_length, 3, self.resolution, self.resolution)


def train_reconet(config_path, output_path):
    with open(config_path, 'r') as f:
        config = json.load(f)

    model_name = config['model_name']
    resolution = config['resolution']
    batch_size = config['batch_size']
    iterations = config['iterations']
    learning_rate = config['learning_rate']
    content_weight = config['content_weight']
    style_weight = config['style_weight']
    tv_weight = config['tv_weight']
    use_gpu = config['use_gpu']

    # Use original high-res paths for style images (preserves brush stroke detail!)
    # Fall back to preprocessed paths if original not available
    image_paths = config.get('original_style_paths', config['image_paths'])
    if not image_paths:
        image_paths = config['image_paths']
    print(f"[Training] Using {'ORIGINAL high-res' if 'original_style_paths' in config else 'preprocessed'} style images")

    content_path = config.get('content_dataset', '')
    temporal_weight = config.get('temporal_weight', 0.0)
    video_dataset_path = config.get('video_dataset', '')
    sequence_length = config.get('sequence_length', 2)

    # Advanced: per-layer style weights (optional)
    style_layer_weights = config.get('style_layer_weights', None)
    if style_layer_weights:
        print(f"[Training] Custom style layer weights: {style_layer_weights}")

    use_temporal = temporal_weight > 0 and video_dataset_path

    device = torch.device('cuda' if use_gpu and torch.cuda.is_available() else 'cpu')
    print(f"[Training] Using: {device}")

    model = ReCoNet().to(device)
    print(f"[Training] Model parameters: {model.get_param_count():,}")
    print(f"[Training] Resolution: {resolution}x{resolution}")
    print(f"[Training] Style weight: {style_weight}, Content weight: {content_weight}, TV weight: {tv_weight}")

    optimizer = optim.AdamW(model.parameters(), lr=learning_rate, weight_decay=1e-5)
    scheduler = optim.lr_scheduler.CosineAnnealingLR(optimizer, T_max=iterations)

    # Load style images at HIGH resolution (up to 1024px) for brush stroke detail
    style_dataset = StyleDataset(image_paths, resolution, max_style_res=1024)
    content_dataset = ContentDataset(content_path, resolution)
    content_loader = DataLoader(content_dataset, batch_size=min(batch_size, len(content_dataset)),
                                shuffle=True, num_workers=0, drop_last=True)

    video_loader = None
    video_iter = None
    if use_temporal:
        try:
            video_paths = [p.strip() for p in video_dataset_path.split(',')]
            video_dataset = VideoFrameDataset(video_paths, resolution, sequence_length)
            video_loader = DataLoader(video_dataset, batch_size=1, shuffle=True, num_workers=0, drop_last=True)
            video_iter = iter(video_loader)
        except Exception as e:
            print(f"[WARNING] Failed to load video dataset: {e}")
            use_temporal = False

    perceptual_loss_fn = PerceptualLoss(style_weights=style_layer_weights).to(device)
    print(f"[Training] Style layer weights: {perceptual_loss_fn.get_style_weights()}")

    # Keep style images as list (they can be different sizes now for high-res)
    # Move to device on-demand to save VRAM
    style_images = [img.to(device) for img in style_dataset.style_images]
    num_styles = len(style_images)
    print(f"[Training] {num_styles} style images loaded at high resolution")

    # Feature temporal loss weight ratio (relative to output temporal)
    # Original ReCoNet: LAMBDA_O = 1e6, LAMBDA_F = 1e4, ratio = 0.01
    # Disabled: feature temporal loss constrains encoder features which encode input colors,
    # preventing proper color adaptation. Use luminance-only output temporal loss instead.
    lambda_f_ratio = 0.0

    model.train()
    start_time = time.time()
    content_iter = iter(content_loader)

    print(f"\n[Training] Starting training for {iterations} iterations...")
    if use_temporal:
        print(f"[Training] Temporal coherence ENABLED (weight={temporal_weight})")
        print(f"[Training] Using ReCoNet approach: output temporal + feature temporal + luminance constraint")

    for iteration in range(iterations):
        try:
            content_batch = next(content_iter)
        except StopIteration:
            content_iter = iter(content_loader)
            content_batch = next(content_iter)

        content_batch = content_batch.to(device)

        # Select random style image (can be different sizes now)
        style_idx = random.randint(0, num_styles - 1)
        style_target = style_images[style_idx].unsqueeze(0)  # Add batch dim [1, C, H, W]

        optimizer.zero_grad()

        # Get output (and features if doing temporal training)
        output = model(content_batch)

        # Style loss uses high-res style image (Gram matrices are size-independent)
        c_loss = perceptual_loss_fn.content_loss(output, content_batch)
        s_loss = perceptual_loss_fn.style_loss(output, style_target)
        tv_loss = total_variation_loss(output)

        total_loss = content_weight * c_loss + style_weight * s_loss + tv_weight * tv_loss

        # Temporal coherence losses (ReCoNet approach)
        t_output_loss = torch.tensor(0.0, device=device)
        t_feature_loss = torch.tensor(0.0, device=device)
        video_style_loss = torch.tensor(0.0, device=device)

        if use_temporal and video_iter is not None:
            try:
                frame_sequence = next(video_iter)
            except StopIteration:
                video_iter = iter(video_loader)
                frame_sequence = next(video_iter)

            frames = frame_sequence.squeeze(0).to(device)
            stylized_frames = []
            feature_maps = []

            # Stylize each frame and collect features for temporal loss
            for i in range(frames.size(0)):
                frame = frames[i:i+1]
                frame_features, stylized = model(frame, return_features=True)
                stylized_frames.append(stylized)
                feature_maps.append(frame_features)

                # Style loss on video frames (prevents temporal from degrading style)
                rand_style = style_images[random.randint(0, num_styles - 1)].unsqueeze(0)
                video_style_loss = video_style_loss + perceptual_loss_fn.style_loss(
                    stylized, rand_style)

            if stylized_frames:
                video_style_loss = video_style_loss / len(stylized_frames)

            # Compute temporal losses between consecutive frames (ReCoNet approach)
            for i in range(len(stylized_frames) - 1):
                input_t = frames[i:i+1]
                input_t1 = frames[i+1:i+2]
                styled_t = stylized_frames[i]
                styled_t1 = stylized_frames[i + 1]
                features_t = feature_maps[i]
                features_t1 = feature_maps[i + 1]

                # Compute optical flow from content frames
                flow = compute_optical_flow(input_t, input_t1)

                # Compute occlusion mask (forward-backward consistency)
                mask = None
                if HAS_OPENCV:
                    flow_backward = compute_optical_flow(input_t1, input_t)
                    mask = compute_occlusion_mask(flow, flow_backward)

                # Output-level temporal loss with luminance warping constraint
                o_loss = output_temporal_loss(styled_t, styled_t1, input_t, input_t1, flow, mask)
                t_output_loss = t_output_loss + o_loss

                # Feature-level temporal loss
                f_loss = feature_temporal_loss(features_t, features_t1, flow, mask)
                t_feature_loss = t_feature_loss + f_loss

            # Average over frame pairs
            num_pairs = len(stylized_frames) - 1
            if num_pairs > 0:
                t_output_loss = t_output_loss / num_pairs
                t_feature_loss = t_feature_loss / num_pairs

            # Add temporal losses to total
            # temporal_weight applies to output temporal, feature temporal is scaled by lambda_f_ratio
            total_loss = total_loss + temporal_weight * t_output_loss
            total_loss = total_loss + temporal_weight * lambda_f_ratio * t_feature_loss
            # Add video frame style loss
            total_loss = total_loss + style_weight * video_style_loss

        total_loss.backward()
        optimizer.step()
        scheduler.step()

        if iteration % 100 == 0:
            elapsed = time.time() - start_time
            remaining = elapsed / (iteration + 1) * (iterations - iteration - 1)
            if use_temporal:
                print(f"[Iteration {iteration}/{iterations}] Loss: {total_loss.item():.2f} "
                      f"(C:{c_loss.item():.2f} S:{s_loss.item():.4f} To:{t_output_loss.item():.4f} Tf:{t_feature_loss.item():.4f}) "
                      f"Time: {elapsed:.1f}s Remain: {remaining:.1f}s")
            else:
                print(f"[Iteration {iteration}/{iterations}] Loss: {total_loss.item():.2f} "
                      f"Time: {elapsed:.1f}s Remain: {remaining:.1f}s")
            sys.stdout.flush()

    print(f"\n[Training] Completed in {(time.time() - start_time)/60:.1f} minutes")

    # Export to ONNX
    print(f"[ONNX Export] Exporting model to {output_path}...")
    model.eval()

    # Use wrapper that only returns output (no features tuple)
    export_model = ReCoNetExport(model)
    dummy_input = torch.rand(1, 3, resolution, resolution).to(device) * 2.0 - 1.0

    # Export with FIXED dimensions matching training resolution
    # This ensures optimal inference quality - model sees exactly what it was trained on
    # Users who want higher detail can train at higher resolution (e.g., 512x512)
    torch.onnx.export(
        export_model,
        dummy_input,
        output_path,
        opset_version=17,
        input_names=["input"],
        output_names=["output"],
        # No dynamic_axes - use fixed dimensions [1, 3, resolution, resolution]
        export_params=True,
        do_constant_folding=True,
        dynamo=False  # Use legacy exporter (dynamo causes issues with model weights)
    )

    print(f"[SUCCESS] Model saved to: {output_path}")
    return True


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--config', type=str, required=True)
    parser.add_argument('--output', type=str, required=True)
    args = parser.parse_args()
    try:
        train_reconet(args.config, args.output)
    except Exception as e:
        print(f"[ERROR] {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
