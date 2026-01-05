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

from reconet_model import ReCoNet
from loss_functions import (
    PerceptualLoss, total_variation_loss,
    compute_optical_flow, temporal_consistency_loss
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
    def __init__(self, image_paths, resolution):
        self.image_paths = [p for p in image_paths if os.path.exists(p)]
        self.resolution = resolution
        self.style_images = []
        for path in self.image_paths:
            try:
                tensor = load_and_preprocess_image(path, resolution)
                self.style_images.append(tensor)
            except:
                pass
        if len(self.style_images) == 0:
            raise ValueError("No valid style images loaded")
        print(f"[Style Dataset] Loaded {len(self.style_images)} style images")

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
    image_paths = config['image_paths']
    content_path = config.get('content_dataset', '')
    temporal_weight = config.get('temporal_weight', 0.0)
    video_dataset_path = config.get('video_dataset', '')
    sequence_length = config.get('sequence_length', 2)

    use_temporal = temporal_weight > 0 and video_dataset_path

    device = torch.device('cuda' if use_gpu and torch.cuda.is_available() else 'cpu')
    print(f"[Training] Using: {device}")

    model = ReCoNet().to(device)
    print(f"[Training] Model parameters: {model.get_param_count():,}")

    optimizer = optim.AdamW(model.parameters(), lr=learning_rate, weight_decay=1e-5)
    scheduler = optim.lr_scheduler.CosineAnnealingLR(optimizer, T_max=iterations)

    style_dataset = StyleDataset(image_paths, resolution)
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

    perceptual_loss_fn = PerceptualLoss().to(device)
    style_images_tensor = torch.stack(style_dataset.style_images).to(device)
    num_styles = len(style_dataset.style_images)

    model.train()
    start_time = time.time()
    content_iter = iter(content_loader)

    for iteration in range(iterations):
        try:
            content_batch = next(content_iter)
        except StopIteration:
            content_iter = iter(content_loader)
            content_batch = next(content_iter)

        content_batch = content_batch.to(device)
        style_idx = torch.randint(0, num_styles, (content_batch.size(0),))
        style_target = style_images_tensor[style_idx]

        optimizer.zero_grad()
        output = model(content_batch)

        c_loss = perceptual_loss_fn.content_loss(output, content_batch)
        s_loss = perceptual_loss_fn.style_loss(output, style_target)
        tv_loss = total_variation_loss(output)

        total_loss = content_weight * c_loss + style_weight * s_loss + tv_weight * tv_loss

        t_loss = torch.tensor(0.0, device=device)
        video_style_loss = torch.tensor(0.0, device=device)

        if use_temporal and video_iter is not None:
            try:
                frame_sequence = next(video_iter)
            except StopIteration:
                video_iter = iter(video_loader)
                frame_sequence = next(video_iter)

            frames = frame_sequence.squeeze(0).to(device)
            stylized_frames = []

            for i in range(frames.size(0)):
                stylized = model(frames[i:i+1])
                stylized_frames.append(stylized)
                video_style_loss = video_style_loss + perceptual_loss_fn.style_loss(
                    stylized, style_images_tensor[torch.randint(0, num_styles, (1,))])

            if stylized_frames:
                video_style_loss = video_style_loss / len(stylized_frames)

            for i in range(len(stylized_frames) - 1):
                flow = compute_optical_flow(frames[i:i+1], frames[i+1:i+2])
                t_loss = t_loss + temporal_consistency_loss(stylized_frames[i], stylized_frames[i+1], flow)

            if len(stylized_frames) > 1:
                t_loss = t_loss / (len(stylized_frames) - 1)

            total_loss = total_loss + temporal_weight * t_loss + style_weight * video_style_loss

        total_loss.backward()
        optimizer.step()
        scheduler.step()

        if iteration % 100 == 0:
            elapsed = time.time() - start_time
            remaining = elapsed / (iteration + 1) * (iterations - iteration - 1)
            print(f"[Iteration {iteration}/{iterations}] Loss: {total_loss.item():.2f} "
                  f"Time: {elapsed:.1f}s Remain: {remaining:.1f}s")
            sys.stdout.flush()

    print(f"\n[Training] Completed in {(time.time() - start_time)/60:.1f} minutes")

    model.eval()
    dummy_input = torch.randn(1, 3, resolution, resolution).to(device)
    torch.onnx.export(model, dummy_input, output_path, opset_version=18,
                      input_names=["input"], output_names=["output"],
                      dynamic_axes={"input": {2: "height", 3: "width"}, "output": {2: "height", 3: "width"}})

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
