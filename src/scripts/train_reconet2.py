"""
train_reconet.py

Main training script for ReCoNet style transfer with temporal coherence
Based on the original ReCoNet paper: https://arxiv.org/abs/1807.01197

Reads configuration from JSON file, trains model, exports to ONNX

Features:
- Single image training (basic style transfer)
- Temporal training on video frames (flicker-free video style transfer)
  - Output-level temporal loss with luminance warping constraint
  - Feature-level temporal loss
  - Occlusion mask support
  - Style loss on video frames (prevents temporal from degrading style)

Usage:
    python train_reconet.py --config config.json --output model.onnx

License: GPL3
"""

import argparse
import json
import os
import sys
import time
import glob
import random
from PIL import Image
import numpy as np

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
    """Load and preprocess an image to tensor in [-1, 1] range"""
    img = Image.open(image_path).convert('RGB')
    img = img.resize((resolution, resolution), Image.LANCZOS)
    img_array = np.array(img).astype(np.float32) / 255.0
    img_array = img_array * 2.0 - 1.0  # [0, 1] -> [-1, 1]
    img_tensor = torch.from_numpy(img_array).permute(2, 0, 1)  # HWC -> CHW
    return img_tensor


class ContentDataset(Dataset):
    """
    Dataset for content images (photos, video frames, etc.)
    Used as input to the style transfer model during training.
    """

    def __init__(self, folder_path, resolution):
        self.resolution = resolution
        self.image_paths = []

        # Find all images in folder
        if os.path.isdir(folder_path):
            for ext in ['*.jpg', '*.jpeg', '*.png', '*.bmp', '*.webp']:
                import glob
                self.image_paths.extend(glob.glob(os.path.join(folder_path, ext)))
                self.image_paths.extend(glob.glob(os.path.join(folder_path, ext.upper())))

        if len(self.image_paths) == 0:
            raise ValueError(f"No images found in content folder: {folder_path}")

        print(f"[Content Dataset] Found {len(self.image_paths)} content images")

    def __len__(self):
        return len(self.image_paths)

    def __getitem__(self, idx):
        """Load and preprocess a content image"""
        try:
            return load_and_preprocess_image(self.image_paths[idx], self.resolution)
        except Exception as e:
            print(f"[WARNING] Failed to load {self.image_paths[idx]}: {e}")
            return torch.zeros(3, self.resolution, self.resolution)


class StyleDataset(Dataset):
    """
    Dataset for style images (artistic references)
    Used as style targets during training.
    """

    def __init__(self, image_paths, resolution):
        self.image_paths = [p for p in image_paths if os.path.exists(p)]
        self.resolution = resolution

        if len(self.image_paths) == 0:
            raise ValueError("No valid style image paths found")

        # Pre-load all style images (typically small number)
        self.style_images = []
        for path in self.image_paths:
            try:
                tensor = load_and_preprocess_image(path, resolution)
                self.style_images.append(tensor)
            except Exception as e:
                print(f"[WARNING] Failed to load style image {path}: {e}")

        if len(self.style_images) == 0:
            raise ValueError("No valid style images loaded")

        print(f"[Style Dataset] Loaded {len(self.style_images)} style images")

    def __len__(self):
        return len(self.style_images)

    def __getitem__(self, idx):
        """Return a style image"""
        return self.style_images[idx]


class VideoFrameDataset(Dataset):
    """
    Dataset for video frame sequences (for temporal coherence training)

    Loads consecutive frame pairs from video files or frame folders.
    Each item returns a sequence of frames for temporal loss computation.
    """

    def __init__(self, video_paths, resolution, sequence_length=2):
        """
        Args:
            video_paths: List of video file paths or folders with frame images
            resolution: Target resolution for frames
            sequence_length: Number of consecutive frames to return (2 for pairs)
        """
        self.resolution = resolution
        self.sequence_length = sequence_length
        self.frame_sequences = []  # List of (start_frame_path, folder_path) or (video_path, start_idx)

        # Try to import cv2 for video reading
        try:
            import cv2
            self.cv2 = cv2
            self.has_cv2 = True
        except ImportError:
            self.cv2 = None
            self.has_cv2 = False
            print("[WARNING] OpenCV not available, can only load frame folders")

        for path in video_paths:
            if os.path.isdir(path):
                # Folder with frame images
                self._add_frame_folder(path)
            elif os.path.isfile(path) and self.has_cv2:
                # Video file
                self._add_video_file(path)

        if len(self.frame_sequences) == 0:
            raise ValueError("No valid video frame sequences found")

        print(f"[Video Dataset] Found {len(self.frame_sequences)} frame sequences")

    def _add_frame_folder(self, folder_path):
        """Add frame sequences from a folder of images or subfolders"""
        # First check if this folder has images directly
        image_paths = []
        for ext in ['*.jpg', '*.jpeg', '*.png', '*.bmp']:
            image_paths.extend(glob.glob(os.path.join(folder_path, ext)))
            image_paths.extend(glob.glob(os.path.join(folder_path, ext.upper())))

        if len(image_paths) >= self.sequence_length:
            # This folder has images - treat as frame sequence
            image_paths = sorted(image_paths)
            for i in range(len(image_paths) - self.sequence_length + 1):
                sequence_paths = image_paths[i:i + self.sequence_length]
                self.frame_sequences.append(('folder', sequence_paths))
        else:
            # No images here - check subdirectories for sequence folders
            try:
                for subdir in os.listdir(folder_path):
                    subdir_path = os.path.join(folder_path, subdir)
                    if os.path.isdir(subdir_path) and not subdir.startswith('_'):
                        # Recursively check this subdirectory
                        sub_images = []
                        for ext in ['*.jpg', '*.jpeg', '*.png', '*.bmp']:
                            sub_images.extend(glob.glob(os.path.join(subdir_path, ext)))
                            sub_images.extend(glob.glob(os.path.join(subdir_path, ext.upper())))

                        if len(sub_images) >= self.sequence_length:
                            sub_images = sorted(sub_images)
                            for i in range(len(sub_images) - self.sequence_length + 1):
                                sequence_paths = sub_images[i:i + self.sequence_length]
                                self.frame_sequences.append(('folder', sequence_paths))
            except Exception as e:
                print(f"[WARNING] Error scanning subdirectories: {e}")

    def _add_video_file(self, video_path):
        """Add frame sequences from a video file"""
        cap = self.cv2.VideoCapture(video_path)
        frame_count = int(cap.get(self.cv2.CAP_PROP_FRAME_COUNT))
        cap.release()

        if frame_count < self.sequence_length:
            return

        # Create sequences (sample every few frames to get diverse motion)
        step = max(1, frame_count // 1000)  # Limit to ~1000 sequences per video
        for i in range(0, frame_count - self.sequence_length + 1, step):
            self.frame_sequences.append(('video', video_path, i))

    def __len__(self):
        return len(self.frame_sequences)

    def _load_frame_from_video(self, video_path, frame_idx):
        """Load a single frame from video file"""
        cap = self.cv2.VideoCapture(video_path)
        cap.set(self.cv2.CAP_PROP_POS_FRAMES, frame_idx)
        ret, frame = cap.read()
        cap.release()

        if not ret:
            return None

        # Convert BGR to RGB
        frame = self.cv2.cvtColor(frame, self.cv2.COLOR_BGR2RGB)

        # Resize
        frame = self.cv2.resize(frame, (self.resolution, self.resolution),
                                interpolation=self.cv2.INTER_LANCZOS4)

        # Convert to tensor [-1, 1]
        frame = frame.astype(np.float32) / 255.0
        frame = frame * 2.0 - 1.0  # [0, 1] -> [-1, 1]
        tensor = torch.from_numpy(frame).permute(2, 0, 1)

        return tensor

    def __getitem__(self, idx):
        """Return a sequence of consecutive frames"""
        item = self.frame_sequences[idx]

        if item[0] == 'folder':
            # Load from image files
            paths = item[1]
            frames = []
            for path in paths:
                try:
                    tensor = load_and_preprocess_image(path, self.resolution)
                    frames.append(tensor)
                except Exception as e:
                    print(f"[WARNING] Failed to load frame {path}: {e}")
                    frames.append(torch.zeros(3, self.resolution, self.resolution))
            return torch.stack(frames)

        elif item[0] == 'video':
            # Load from video file
            video_path = item[1]
            start_idx = item[2]
            frames = []
            for i in range(self.sequence_length):
                tensor = self._load_frame_from_video(video_path, start_idx + i)
                if tensor is None:
                    tensor = torch.zeros(3, self.resolution, self.resolution)
                frames.append(tensor)
            return torch.stack(frames)

        return torch.zeros(self.sequence_length, 3, self.resolution, self.resolution)


def train_reconet(config_path, output_path):
    """
    Main training function with optional temporal coherence

    Args:
        config_path: Path to JSON config file
        output_path: Path to save ONNX model
    """

    # Load config
    print(f"[Training] Loading config from {config_path}")
    with open(config_path, 'r') as f:
        config = json.load(f)

    print(f"[Training] Config: {json.dumps(config, indent=2)}")

    # Extract config
    model_name = config['model_name']
    resolution = config['resolution']
    batch_size = config['batch_size']
    iterations = config['iterations']
    learning_rate = config['learning_rate']
    content_weight = config['content_weight']
    style_weight = config['style_weight']
    tv_weight = config['tv_weight']
    use_gpu = config['use_gpu']
    image_paths = config['image_paths']  # Style images
    content_path = config.get('content_dataset', '')  # Content images folder

    # Temporal coherence settings (optional)
    temporal_weight = config.get('temporal_weight', 0.0)  # 0 = disabled
    video_dataset_path = config.get('video_dataset', '')  # Path to video files/frame folders
    sequence_length = config.get('sequence_length', 2)  # Frames per sequence

    use_temporal = temporal_weight > 0 and video_dataset_path

    # Setup device
    if use_gpu and torch.cuda.is_available():
        device = torch.device('cuda')
        print(f"[Training] Using GPU: {torch.cuda.get_device_name(0)}")
    else:
        device = torch.device('cpu')
        print(f"[Training] Using CPU")

    # Create model
    print("[Training] Creating ReCoNet model...")
    model = ReCoNet().to(device)
    print(f"[Training] Model parameters: {model.get_param_count():,}")

    # Create optimizer (original ReCoNet uses Adam)
    optimizer = optim.Adam(model.parameters(), lr=learning_rate)

    # Learning rate scheduler (matching original ReCoNet)
    # Decay by factor of 1.2 every 500 iterations, minimum 1e-4
    min_lr = 1e-4
    decay_factor = 1.2
    decay_step = 500

    def lr_lambda(iteration):
        # Calculate decayed LR
        num_decays = iteration // decay_step
        decayed_lr = learning_rate / (decay_factor ** num_decays)
        # Enforce minimum
        decayed_lr = max(decayed_lr, min_lr)
        # Return multiplier (scheduler multiplies base_lr by this)
        return decayed_lr / learning_rate

    scheduler = optim.lr_scheduler.LambdaLR(optimizer, lr_lambda)

    # Load style images
    print("[Training] Loading style images...")
    style_dataset = StyleDataset(image_paths, resolution)

    # Load content dataset
    if not content_path or not os.path.isdir(content_path):
        print(f"[ERROR] Content dataset path not found: {content_path}")
        print("[ERROR] Please specify a valid 'content_dataset' folder in config")
        print("[ERROR] You can use COCO, ImageNet, or any folder with photos")
        return False

    print(f"[Training] Loading content images from: {content_path}")
    content_dataset = ContentDataset(content_path, resolution)

    # Adjust batch size if needed
    actual_batch_size = min(batch_size, len(content_dataset))
    if actual_batch_size < batch_size:
        print(f"[Training] Reducing batch size to {actual_batch_size} (content dataset size)")

    content_loader = DataLoader(content_dataset, batch_size=actual_batch_size, shuffle=True,
                                num_workers=0, drop_last=True)

    # Load video dataset for temporal training (if enabled)
    video_loader = None
    video_iter = None
    if use_temporal:
        print(f"\n[Temporal] Temporal coherence ENABLED (weight={temporal_weight})")
        print(f"[Temporal] Using original ReCoNet approach:")
        print(f"[Temporal]   - Output temporal loss with luminance warping constraint")
        print(f"[Temporal]   - Feature-level temporal loss (LAMBDA_F ratio = 0.01)")
        print(f"[Temporal]   - Occlusion mask for excluding occluded regions")
        print(f"[Temporal]   - Style loss on video frames (prevents style degradation)")
        try:
            # video_dataset_path can be a single path or comma-separated list
            video_paths = [p.strip() for p in video_dataset_path.split(',')]
            video_dataset = VideoFrameDataset(video_paths, resolution, sequence_length)
            video_loader = DataLoader(video_dataset, batch_size=1, shuffle=True,
                                     num_workers=0, drop_last=True)
            video_iter = iter(video_loader)
            print(f"[Temporal] Loaded {len(video_dataset)} frame sequences")
        except Exception as e:
            print(f"[WARNING] Failed to load video dataset: {e}")
            print("[WARNING] Continuing without temporal training")
            use_temporal = False
            temporal_weight = 0.0
    else:
        print(f"\n[Temporal] Temporal coherence DISABLED")
        if not video_dataset_path:
            print("[Temporal] To enable: set 'video_dataset' in config to video files or frame folders")
        if temporal_weight <= 0:
            print("[Temporal] To enable: set 'temporal_weight' > 0 in config (try 1e2 to 1e4)")

    # Create perceptual loss
    print("\n[Training] Initializing VGG16 for perceptual loss...")
    perceptual_loss_fn = PerceptualLoss().to(device)

    # Pre-compute style targets on GPU
    style_images_tensor = torch.stack(style_dataset.style_images).to(device)
    num_styles = len(style_dataset.style_images)
    print(f"[Training] Style targets: {num_styles} images")
    print(f"[Training] Content images: {len(content_dataset)} images")

    # Feature temporal loss weight ratio (relative to output temporal)
    # Original ReCoNet: LAMBDA_O = 1e6, LAMBDA_F = 1e4, ratio = 0.01
    lambda_f_ratio = 0.01

    # Training loop
    print(f"\n[Training] Starting training for {iterations} iterations...")
    print(f"[Training] Content images -> Model -> Style transfer")
    if use_temporal:
        print(f"[Training] + Temporal coherence on video frames (ReCoNet-style)")
    model.train()
    start_time = time.time()
    content_iter = iter(content_loader)

    for iteration in range(iterations):
        # Get content batch (real images)
        try:
            content_batch = next(content_iter)
        except StopIteration:
            content_iter = iter(content_loader)
            content_batch = next(content_iter)

        content_batch = content_batch.to(device)

        # Get random style target for this batch
        style_idx = torch.randint(0, num_styles, (content_batch.size(0),))
        style_target = style_images_tensor[style_idx]

        # Forward pass: transform content with style
        optimizer.zero_grad()

        # Model returns (features, output)
        features, output = model(content_batch, return_features=True)

        # Compute losses:
        # - Content loss: output should preserve structure of content input
        # - Style loss: output should have style of style target images
        c_loss = perceptual_loss_fn.content_loss(output, content_batch)

        # Debug style loss every 500 iterations to see Gram matrix values
        debug_style = (iteration % 500 == 0)
        if debug_style:
            print(f"[DEBUG] Style loss breakdown at iteration {iteration}:")
        s_loss = perceptual_loss_fn.style_loss(output, style_target, debug=debug_style)
        tv_loss = total_variation_loss(output)

        # Combined loss (without temporal for now)
        total_loss = (content_weight * c_loss +
                     style_weight * s_loss +
                     tv_weight * tv_loss)

        # Temporal coherence loss (if enabled)
        # Uses original ReCoNet approach with all improvements
        t_output_loss = torch.tensor(0.0, device=device)
        t_feature_loss = torch.tensor(0.0, device=device)
        video_style_loss = torch.tensor(0.0, device=device)

        if use_temporal and video_iter is not None:
            try:
                # Get video frame sequence [1, seq_len, 3, H, W]
                frame_sequence = next(video_iter)
            except StopIteration:
                video_iter = iter(video_loader)
                frame_sequence = next(video_iter)

            # Remove batch dimension and move to device
            # frame_sequence shape: [1, seq_len, 3, H, W] -> [seq_len, 3, H, W]
            frames = frame_sequence.squeeze(0).to(device)

            # Stylize each frame in the sequence and collect features
            stylized_frames = []
            feature_maps = []
            for i in range(frames.size(0)):
                frame = frames[i:i+1]  # Keep batch dimension [1, 3, H, W]
                frame_features, stylized = model(frame, return_features=True)
                stylized_frames.append(stylized)
                feature_maps.append(frame_features)

                # Compute style loss on each video frame (prevents temporal from degrading style)
                video_style_loss = video_style_loss + perceptual_loss_fn.style_loss(
                    stylized, style_images_tensor[torch.randint(0, num_styles, (1,))]
                )

            # Average style loss over video frames
            if len(stylized_frames) > 0:
                video_style_loss = video_style_loss / len(stylized_frames)

            # Compute temporal losses between consecutive frames
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
                # Formula: ||(Ŝ₂ - warp(Ŝ₁)) - (I₂ - warp(I₁))||²
                o_loss = output_temporal_loss(styled_t, styled_t1, input_t, input_t1, flow, mask)
                t_output_loss = t_output_loss + o_loss

                # Feature-level temporal loss
                # Formula: ||F₂ - warp(F₁)||²
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

        # Backward pass
        total_loss.backward()
        optimizer.step()
        scheduler.step()

        # Print progress every 100 iterations
        if iteration % 100 == 0 or iteration == iterations - 1:
            elapsed = time.time() - start_time
            avg_time_per_iter = elapsed / (iteration + 1)
            remaining = avg_time_per_iter * (iterations - iteration - 1)

            if use_temporal:
                # Show all loss components
                c_weighted = content_weight * c_loss.item()
                s_weighted = style_weight * s_loss.item()
                vs_weighted = style_weight * video_style_loss.item()
                tv_weighted = tv_weight * tv_loss.item()
                to_weighted = temporal_weight * t_output_loss.item()
                tf_weighted = temporal_weight * lambda_f_ratio * t_feature_loss.item()
                print(f"[Iteration {iteration}/{iterations}] "
                      f"Loss: {total_loss.item():.2f} "
                      f"(C:{c_weighted:.2f} S:{s_weighted:.2f} VS:{vs_weighted:.2f} "
                      f"TV:{tv_weighted:.2e} To:{to_weighted:.2e} Tf:{tf_weighted:.2e}) "
                      f"LR: {scheduler.get_last_lr()[0]:.6f} "
                      f"Time: {elapsed:.1f}s Remain: {remaining:.1f}s")
            else:
                # Calculate weighted contributions for display
                c_weighted = content_weight * c_loss.item()
                s_weighted = style_weight * s_loss.item()
                tv_weighted = tv_weight * tv_loss.item()
                print(f"[Iteration {iteration}/{iterations}] "
                      f"Loss: {total_loss.item():.2f} "
                      f"(C:{c_weighted:.2f} S:{s_weighted:.2f} TV:{tv_weighted:.2e}) "
                      f"raw_tv:{tv_loss.item():.2f} "
                      f"LR: {scheduler.get_last_lr()[0]:.6f} "
                      f"Time: {elapsed:.1f}s Remain: {remaining:.1f}s")
            sys.stdout.flush()  # Force print to stdout (C++ reads this)

    total_time = time.time() - start_time
    print(f"\n[Training] Training completed in {total_time:.1f}s ({total_time/60:.1f} minutes)")

    # Export to ONNX
    print(f"[ONNX Export] Exporting model to {output_path}...")

    model.eval()

    # Use wrapper that only returns output (no features tuple)
    export_model = ReCoNetExport(model)
    # Dummy input in [-1, 1] range
    dummy_input = torch.rand(1, 3, resolution, resolution).to(device) * 2.0 - 1.0

    torch.onnx.export(
        export_model,
        dummy_input,
        output_path,
        opset_version=18,
        input_names=["input"],
        output_names=["output"],
        dynamic_axes={
            "input": {2: "height", 3: "width"},
            "output": {2: "height", 3: "width"}
        },
        export_params=True,
        do_constant_folding=True,
        dynamo=False,  # Use legacy exporter to avoid Unicode issues on Windows
    )

    print(f"[ONNX Export] Model exported successfully")
    print(f"[ONNX Export] Model saved to: {output_path}")

    # Verify export
    if os.path.exists(output_path):
        file_size_mb = os.path.getsize(output_path) / (1024 * 1024)
        print(f"[ONNX Export] File size: {file_size_mb:.2f} MB")
    else:
        print(f"[ERROR] ONNX export failed - file not found")
        return False

    print(f"\n[SUCCESS] Training and export completed for '{model_name}'")
    return True


def main():
    parser = argparse.ArgumentParser(description='Train ReCoNet style transfer model')
    parser.add_argument('--config', type=str, required=True,
                       help='Path to config JSON file')
    parser.add_argument('--output', type=str, required=True,
                       help='Path to save output ONNX model')

    args = parser.parse_args()

    try:
        success = train_reconet(args.config, args.output)
        sys.exit(0 if success else 1)
    except Exception as e:
        print(f"[ERROR] Training failed: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()
