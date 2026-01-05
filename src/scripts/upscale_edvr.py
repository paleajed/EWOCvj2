#!/usr/bin/env python3
"""
upscale_edvr.py

EDVR (Enhanced Deformable Video Restoration) Video Upscaling Script
Supports FAST (EDVR_M) and BALANCED (EDVR_L) quality modes

Usage:
    python upscale_edvr.py --config config.json

Config JSON format:
{
    "input_dir": "path/to/input/frames",
    "output_dir": "path/to/output/frames",
    "model_name": "EDVR_M" or "EDVR_L",
    "scale_factor": 4,
    "cleanup_mode": false,
    "use_gpu": true,
    "gpu_id": 0,
    "tile_size": 0,
    "temporal_radius": 2
}

Progress output format:
    [Frame 450/2000] fps: 12.34 ETA: 120.3s

License: GPL3
"""

import os
import sys
import json
import argparse
import time
import glob
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor

import torch
import torch.nn.functional as F
import numpy as np
from PIL import Image

# Try to import BasicSR's EDVR - fall back to standalone if not available
try:
    from basicsr.archs.edvr_arch import EDVR
    BASICSR_AVAILABLE = True
except ImportError:
    BASICSR_AVAILABLE = False
    print("[EDVR] BasicSR not found, using standalone EDVR implementation")


class EDVRNet(torch.nn.Module):
    """
    Standalone EDVR network for when BasicSR is not available.
    This is a simplified version - for full features use BasicSR.
    """
    def __init__(self, num_in_ch=3, num_out_ch=3, num_feat=64, num_frame=5,
                 deformable_groups=8, num_extract_block=5, num_reconstruct_block=10,
                 center_frame_idx=None, hr_in=False, with_predeblur=False,
                 with_tsa=True, scale=4):
        super().__init__()
        self.scale = scale
        self.center_frame_idx = center_frame_idx if center_frame_idx is not None else num_frame // 2
        self.hr_in = hr_in

        # Placeholder - in production, load actual EDVR architecture
        # For now, use a simple upscaling network as fallback
        self.conv_first = torch.nn.Conv2d(num_in_ch * num_frame, num_feat, 3, 1, 1)
        self.body = torch.nn.Sequential(
            *[torch.nn.Sequential(
                torch.nn.Conv2d(num_feat, num_feat, 3, 1, 1),
                torch.nn.LeakyReLU(0.1, inplace=True)
            ) for _ in range(num_reconstruct_block)]
        )
        self.upscale = torch.nn.Sequential(
            torch.nn.Conv2d(num_feat, num_feat * scale * scale, 3, 1, 1),
            torch.nn.PixelShuffle(scale),
            torch.nn.Conv2d(num_feat, num_out_ch, 3, 1, 1)
        )

    def forward(self, x):
        # x: (B, N, C, H, W) where N is number of frames
        b, n, c, h, w = x.size()
        # Concatenate frames along channel dimension
        x = x.view(b, n * c, h, w)
        feat = self.conv_first(x)
        feat = self.body(feat) + feat
        out = self.upscale(feat)
        return out


def lanczos_kernel(x, a=3):
    """Lanczos kernel for high-quality downscaling."""
    if x == 0:
        return 1.0
    if abs(x) >= a:
        return 0.0
    pi_x = np.pi * x
    return (a * np.sin(pi_x) * np.sin(pi_x / a)) / (pi_x * pi_x)


def lanczos_resize(img, target_size, a=3):
    """
    Resize image using Lanczos interpolation.
    img: PIL Image or numpy array (H, W, C)
    target_size: (width, height)
    """
    if isinstance(img, np.ndarray):
        img = Image.fromarray(img.astype(np.uint8))
    return img.resize(target_size, Image.LANCZOS)


def load_image(path):
    """Load image as tensor (C, H, W) in range [0, 1]."""
    img = Image.open(path).convert('RGB')
    img = np.array(img).astype(np.float32) / 255.0
    img = torch.from_numpy(img).permute(2, 0, 1)  # (H, W, C) -> (C, H, W)
    return img


def save_image(tensor, path):
    """Save tensor (C, H, W) in range [0, 1] as image."""
    img = tensor.clamp(0, 1).cpu().numpy()
    save_image_numpy(img, path)


def save_image_numpy(img, path):
    """Save numpy array (C, H, W) in range [0, 1] as image. Thread-safe."""
    img = (img * 255).astype(np.uint8)
    img = np.transpose(img, (1, 2, 0))  # (C, H, W) -> (H, W, C)
    # Use cv2 for faster saving with minimal PNG compression
    try:
        import cv2
        cv2.imwrite(path, cv2.cvtColor(img, cv2.COLOR_RGB2BGR), [cv2.IMWRITE_PNG_COMPRESSION, 1])
    except ImportError:
        Image.fromarray(img).save(path, compress_level=1)


def get_frame_paths(input_dir):
    """Get sorted list of frame paths."""
    patterns = ['*.png', '*.jpg', '*.jpeg', '*.bmp']
    frame_paths = []
    for pattern in patterns:
        frame_paths.extend(glob.glob(os.path.join(input_dir, pattern)))
    frame_paths.sort()
    return frame_paths


def load_edvr_model(model_name, scale, device, num_frame=5):
    """Load EDVR model."""
    if BASICSR_AVAILABLE:
        # Use BasicSR's EDVR
        if model_name == 'EDVR_M':
            model = EDVR(
                num_in_ch=3, num_out_ch=3, num_feat=64,
                num_frame=num_frame, deformable_groups=8,
                num_extract_block=5, num_reconstruct_block=10,
                center_frame_idx=num_frame // 2,
                hr_in=False, with_predeblur=False, with_tsa=True
            )
        else:  # EDVR_L
            model = EDVR(
                num_in_ch=3, num_out_ch=3, num_feat=128,
                num_frame=num_frame, deformable_groups=8,
                num_extract_block=5, num_reconstruct_block=40,
                center_frame_idx=num_frame // 2,
                hr_in=False, with_predeblur=False, with_tsa=True
            )
    else:
        # Use standalone implementation
        num_feat = 64 if model_name == 'EDVR_M' else 128
        num_reconstruct = 10 if model_name == 'EDVR_M' else 40
        model = EDVRNet(
            num_in_ch=3, num_out_ch=3, num_feat=num_feat,
            num_frame=num_frame, num_reconstruct_block=num_reconstruct,
            scale=scale
        )

    model = model.to(device)
    model.eval()

    # Try torch.compile for PyTorch 2.0+ (can provide ~20-50% speedup)
    # Disabled by default as deformable convolutions may have issues
    use_compile = os.environ.get('EDVR_COMPILE', '0') == '1'
    if use_compile and hasattr(torch, 'compile'):
        try:
            print("[EDVR] Compiling model with torch.compile()...")
            model = torch.compile(model, mode='reduce-overhead')
        except Exception as e:
            print(f"[EDVR] torch.compile failed: {e}, using eager mode")

    # Try to load pretrained weights
    models_dir = os.path.join(os.path.dirname(__file__), '..', 'models', 'upscale')

    # Map model names to actual downloaded filenames
    model_files = {
        'EDVR_M': 'EDVR_M_x4_SR_REDS_official-32075921.pth',
        'EDVR_L': 'EDVR_L_x4_SR_REDS_official-9f5f5039.pth',
    }

    weight_paths = [
        os.path.join(models_dir, model_files.get(model_name, f'{model_name}.pth')),
        os.path.join(models_dir, f'{model_name}_x{scale}.pth'),
        os.path.join(models_dir, f'{model_name}.pth'),
    ]

    # Also search for any file matching the model name pattern
    import glob as glob_module
    pattern_matches = glob_module.glob(os.path.join(models_dir, f'{model_name}*.pth'))
    weight_paths.extend(pattern_matches)

    weight_loaded = False
    for weight_path in weight_paths:
        if os.path.exists(weight_path):
            print(f"[EDVR] Loading weights from {weight_path}")
            state_dict = torch.load(weight_path, map_location=device, weights_only=False)
            if 'params' in state_dict:
                state_dict = state_dict['params']
            elif 'params_ema' in state_dict:
                state_dict = state_dict['params_ema']
            model.load_state_dict(state_dict, strict=False)
            weight_loaded = True
            break

    if not weight_loaded:
        print(f"[EDVR] WARNING: No pretrained weights found for {model_name}")
        print(f"[EDVR] Searched: {weight_paths}")
        print(f"[EDVR] Model will use random weights (output will be poor)")

    return model


def process_frames(model, frame_paths, output_dir, config, device):
    """Process all frames using EDVR with temporal information."""
    num_frames = len(frame_paths)
    temporal_radius = config.get('temporal_radius', 2)
    num_input_frames = temporal_radius * 2 + 1  # e.g., 5 for radius=2
    scale_factor = config.get('scale_factor', 4)
    cleanup_mode = config.get('cleanup_mode', False)
    tile_size = config.get('tile_size', 0)
    use_fp16 = config.get('use_fp16', True) and device.type == 'cuda'

    print(f"[EDVR] Processing {num_frames} frames")
    print(f"[EDVR] Temporal radius: {temporal_radius} ({num_input_frames} input frames)")
    print(f"[EDVR] Scale factor: {scale_factor}")
    print(f"[EDVR] Cleanup mode: {cleanup_mode}")
    print(f"[EDVR] FP16 acceleration: {use_fp16}")

    os.makedirs(output_dir, exist_ok=True)

    start_time = time.time()
    frame_times = []

    # Estimate memory for frame caching
    sample_frame = load_image(frame_paths[0])
    frame_mem_bytes = sample_frame.numel() * 4  # float32 = 4 bytes
    total_cache_mem = frame_mem_bytes * num_frames / (1024**3)  # GB
    available_mem = torch.cuda.get_device_properties(0).total_memory / (1024**3) if device.type == 'cuda' else 0

    # Use full caching if it fits in ~50% of VRAM, otherwise use sliding window
    use_full_cache = device.type == 'cuda' and total_cache_mem < available_mem * 0.5

    frame_cache = {}
    if use_full_cache:
        print(f"[EDVR] Pre-loading all {num_frames} frames to GPU ({total_cache_mem:.1f} GB)...")
        for idx, path in enumerate(frame_paths):
            frame_cache[idx] = load_image(path).to(device)
        print(f"[EDVR] Frames loaded to GPU")
    else:
        print(f"[EDVR] Using sliding window cache (video too large for full GPU cache)")

    # Async I/O: save frames in background threads while GPU processes next frame
    save_executor = ThreadPoolExecutor(max_workers=4)
    save_futures = []

    with torch.no_grad(), torch.amp.autocast('cuda', enabled=use_fp16):
        for i, center_path in enumerate(frame_paths):
            frame_start = time.time()

            # Gather temporal neighborhood
            input_frames = []
            for offset in range(-temporal_radius, temporal_radius + 1):
                idx = i + offset
                # Mirror padding at boundaries
                idx = max(0, min(num_frames - 1, idx))
                if use_full_cache:
                    input_frames.append(frame_cache[idx])
                else:
                    # Load on demand for sliding window mode
                    if idx not in frame_cache:
                        frame_cache[idx] = load_image(frame_paths[idx]).to(device)
                    input_frames.append(frame_cache[idx])

            # Clean up old frames from sliding window cache
            if not use_full_cache and len(frame_cache) > num_input_frames * 2:
                old_keys = [k for k in frame_cache.keys() if k < i - temporal_radius - 1]
                for k in old_keys:
                    del frame_cache[k]

            # Stack frames: (N, C, H, W) -> (1, N, C, H, W)
            input_tensor = torch.stack(input_frames, dim=0).unsqueeze(0)

            # Get original size for cleanup mode
            _, _, _, h, w = input_tensor.shape

            # Process with EDVR
            if tile_size > 0:
                # Tile-based processing for large frames
                output = tile_process(model, input_tensor, tile_size, scale_factor)
            else:
                output = model(input_tensor)

            # output shape: (1, C, H*scale, W*scale)
            output = output.squeeze(0)  # (C, H*scale, W*scale)

            # Cleanup mode: downscale back to original resolution using Lanczos
            if cleanup_mode:
                output_np = output.clamp(0, 1).cpu().numpy()
                output_np = (output_np * 255).astype(np.uint8)
                output_np = np.transpose(output_np, (1, 2, 0))  # (C, H, W) -> (H, W, C)
                output_pil = Image.fromarray(output_np)
                output_pil = output_pil.resize((w, h), Image.LANCZOS)
                output_np = np.array(output_pil).astype(np.float32) / 255.0
                output = torch.from_numpy(output_np).permute(2, 0, 1)  # Back to (C, H, W)

            # Save output frame (async)
            output_filename = os.path.basename(center_path)
            output_path = os.path.join(output_dir, output_filename)
            # Convert to CPU numpy now to free GPU memory, save in background
            output_np = output.clamp(0, 1).cpu().numpy()
            save_futures.append(save_executor.submit(save_image_numpy, output_np, output_path))

            # Calculate timing
            frame_time = time.time() - frame_start
            frame_times.append(frame_time)

            # Calculate stats
            elapsed = time.time() - start_time
            avg_frame_time = sum(frame_times[-100:]) / len(frame_times[-100:])  # Rolling average
            fps = 1.0 / avg_frame_time if avg_frame_time > 0 else 0
            remaining_frames = num_frames - (i + 1)
            eta = remaining_frames * avg_frame_time

            # Print progress in expected format
            print(f"[Frame {i+1}/{num_frames}] fps: {fps:.2f} ETA: {eta:.1f}s", flush=True)

    # Wait for all saves to complete
    print(f"[EDVR] Waiting for {len(save_futures)} frames to finish saving...")
    for future in save_futures:
        future.result()  # This will raise any exceptions that occurred
    save_executor.shutdown(wait=True)

    total_time = time.time() - start_time
    avg_fps = num_frames / total_time if total_time > 0 else 0
    print(f"[EDVR] Complete! Processed {num_frames} frames in {total_time:.1f}s ({avg_fps:.2f} fps)")


def tile_process(model, input_tensor, tile_size, scale):
    """Process large frames in tiles to reduce VRAM usage."""
    b, n, c, h, w = input_tensor.shape

    # Calculate output size
    out_h, out_w = h * scale, w * scale

    # Tile parameters
    tile_pad = 32  # Padding to avoid edge artifacts

    # Create output tensor
    output = torch.zeros((b, c, out_h, out_w), device=input_tensor.device)

    # Process tiles
    for y in range(0, h, tile_size):
        for x in range(0, w, tile_size):
            # Calculate tile boundaries with padding
            y1 = max(0, y - tile_pad)
            x1 = max(0, x - tile_pad)
            y2 = min(h, y + tile_size + tile_pad)
            x2 = min(w, x + tile_size + tile_pad)

            # Extract tile from all frames
            tile = input_tensor[:, :, :, y1:y2, x1:x2]

            # Process tile
            tile_output = model(tile)

            # Calculate output region (accounting for padding)
            out_y1 = (y - y1) * scale
            out_x1 = (x - x1) * scale
            out_y2 = out_y1 + min(tile_size, h - y) * scale
            out_x2 = out_x1 + min(tile_size, w - x) * scale

            # Place tile in output
            output[:, :, y*scale:(y+tile_size)*scale, x*scale:(x+tile_size)*scale] = \
                tile_output[:, :, out_y1:out_y2, out_x1:out_x2]

    return output


def main():
    parser = argparse.ArgumentParser(description='EDVR Video Upscaling')
    parser.add_argument('--config', type=str, required=True, help='Path to config JSON')
    args = parser.parse_args()

    # Load config
    with open(args.config, 'r') as f:
        config = json.load(f)

    print(f"[EDVR] Config loaded from {args.config}")
    print(f"[EDVR] Input dir: {config['input_dir']}")
    print(f"[EDVR] Output dir: {config['output_dir']}")
    print(f"[EDVR] Model: {config['model_name']}")

    # Setup device
    use_gpu = config.get('use_gpu', True)
    gpu_id = config.get('gpu_id', 0)

    if use_gpu and torch.cuda.is_available():
        device = torch.device(f'cuda:{gpu_id}')
        print(f"[EDVR] Using CUDA device {gpu_id}: {torch.cuda.get_device_name(gpu_id)}")
        # Enable cuDNN optimizations for consistent input sizes
        torch.backends.cudnn.benchmark = True
        torch.backends.cudnn.deterministic = False
    else:
        device = torch.device('cpu')
        print("[EDVR] Using CPU")

    # Get frame paths
    frame_paths = get_frame_paths(config['input_dir'])
    if not frame_paths:
        print(f"[EDVR] ERROR: No frames found in {config['input_dir']}")
        sys.exit(1)

    print(f"[EDVR] Found {len(frame_paths)} frames")

    # Load model
    temporal_radius = config.get('temporal_radius', 2)
    num_frame = temporal_radius * 2 + 1
    scale = config.get('scale_factor', 4)

    print(f"[EDVR] Loading model...")
    model = load_edvr_model(config['model_name'], scale, device, num_frame)

    # Process frames
    process_frames(model, frame_paths, config['output_dir'], config, device)

    print("[EDVR] Done!")


if __name__ == '__main__':
    main()
