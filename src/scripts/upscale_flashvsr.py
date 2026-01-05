#!/usr/bin/env python3
"""
upscale_flashvsr.py

FlashVSR Video Super-Resolution Script using official FlashVSR v1.1
Uses FlashVSR TinyLong pipeline with TCDecoder for optimal VRAM efficiency.
TCDecoder provides virtually identical quality to full VAE at 7x faster decoding.

Usage:
    python upscale_flashvsr.py --config config.json

Config JSON format:
{
    "input_dir": "path/to/input/frames",
    "output_dir": "path/to/output/frames",
    "model_name": "FlashVSR_Lite",
    "scale_factor": 4,
    "cleanup_mode": false,
    "use_gpu": true,
    "gpu_id": 0
}

Progress output format:
    [Frame 450/2000] fps: 12.34 ETA: 120.3s

License: GPL3
"""

import os
import sys

# Set CUDA memory allocation to avoid fragmentation (before importing torch)
os.environ['PYTORCH_CUDA_ALLOC_CONF'] = 'expandable_segments:True'  # New name
os.environ['PYTORCH_ALLOC_CONF'] = 'expandable_segments:True'  # Deprecated but some versions need it

import json
import argparse
import time
import glob
import re
import gc
import numpy as np
from pathlib import Path
from PIL import Image
from concurrent.futures import ThreadPoolExecutor

import torch
import torch.nn.functional as F
from einops import rearrange

# Custom tqdm wrapper that outputs parseable progress
# MUST be defined and patched BEFORE any diffsynth imports
class ProgressTqdm:
    """Wrapper for tqdm that outputs parseable progress for C++ to read."""
    def __init__(self, iterable=None, total=None, desc=None, disable=False, **kwargs):
        self.iterable = iterable
        self.total = total if total is not None else (len(iterable) if iterable is not None and hasattr(iterable, '__len__') else 0)
        self.desc = desc or "Processing"
        self.current = 0
        self.start_time = None
        self.disable = disable
        # Print immediately so we know tqdm was called
        if not disable:
            print(f"[FlashVSR] Starting iteration: {self.desc} ({self.total} items)", flush=True)
            sys.stdout.flush()

    def __iter__(self):
        import time
        self.start_time = time.time()
        if self.iterable is None:
            return
        for item in self.iterable:
            yield item
            self.current += 1
            if not self.disable:
                elapsed = time.time() - self.start_time
                fps = self.current / elapsed if elapsed > 0 else 0
                eta = (self.total - self.current) / fps if fps > 0 else 0
                # Output in format that parseProgressLine can read
                print(f"[Inference {self.current}/{self.total}] fps: {fps:.2f} ETA: {eta:.1f}s", flush=True)
                sys.stdout.flush()

    def __enter__(self):
        return self

    def __exit__(self, *args):
        pass

    def update(self, n=1):
        self.current += n
        if not self.disable:
            print(f"[Inference {self.current}/{self.total}] updated", flush=True)
            sys.stdout.flush()

    def set_description(self, desc):
        self.desc = desc

    def close(self):
        pass

# Patch tqdm.tqdm IMMEDIATELY after defining ProgressTqdm
# This MUST happen before ANY imports that might load diffsynth modules
import tqdm as tqdm_module
original_tqdm = tqdm_module.tqdm  # Save original for our own use if needed
tqdm_module.tqdm = ProgressTqdm
print("[FlashVSR] Patched tqdm.tqdm with ProgressTqdm BEFORE diffsynth imports", flush=True)

# Add FlashVSR to path
FLASHVSR_PATH = "C:/source/FlashVSR"
if os.path.isdir(FLASHVSR_PATH) and FLASHVSR_PATH not in sys.path:
    sys.path.insert(0, FLASHVSR_PATH)

# Add examples/WanVSR to path for utils
WANVSR_UTILS_PATH = os.path.join(FLASHVSR_PATH, "examples", "WanVSR")
if os.path.isdir(WANVSR_UTILS_PATH) and WANVSR_UTILS_PATH not in sys.path:
    sys.path.insert(0, WANVSR_UTILS_PATH)

# Import FlashVSR TinyLong pipeline (optimal for consumer GPUs)
try:
    from diffsynth import ModelManager, FlashVSRTinyLongPipeline
    from utils.utils import Causal_LQ4x_Proj
    from utils.TCDecoder import build_tcdecoder
    print("[FlashVSR] FlashVSRTinyLongPipeline loaded successfully")
except ImportError as e:
    print(f"[FlashVSR] ERROR: Could not import FlashVSR: {e}")
    print(f"[FlashVSR] Make sure FlashVSR is installed at {FLASHVSR_PATH}")
    sys.exit(1)


def natural_key(name: str):
    """Natural sorting key for filenames."""
    return [int(t) if t.isdigit() else t.lower() for t in re.split(r'([0-9]+)', os.path.basename(name))]


def get_frame_paths(input_dir):
    """Get sorted list of frame paths."""
    patterns = ['*.png', '*.jpg', '*.jpeg', '*.bmp']
    frame_paths = []
    for pattern in patterns:
        frame_paths.extend(glob.glob(os.path.join(input_dir, pattern)))
    frame_paths.sort(key=natural_key)
    return frame_paths


def largest_8n1_leq(n):
    """Get largest number of form 8n+1 that is <= n."""
    return 0 if n < 1 else ((n - 1) // 8) * 8 + 1


def compute_scaled_dims(w0, h0, scale=4, multiple=128):
    """Compute scaled and target dimensions (must be multiple of 128, rounded UP)."""
    sW, sH = w0 * scale, h0 * scale
    # Round UP to nearest multiple of 128 (ceiling division)
    tW = ((sW + multiple - 1) // multiple) * multiple
    tH = ((sH + multiple - 1) // multiple) * multiple
    return sW, sH, tW, tH


def pil_to_tensor(img, dtype=torch.bfloat16, device='cuda'):
    """Convert PIL image to tensor in range [-1, 1]."""
    # Use .copy() to make array writable and avoid PyTorch warning
    t = torch.from_numpy(np.asarray(img, np.uint8).copy()).to(device=device, dtype=torch.float32)
    t = t.permute(2, 0, 1) / 255.0 * 2.0 - 1.0  # CHW in [-1, 1]
    return t.to(dtype)


def tensor_to_pil(tensor):
    """Convert tensor in range [-1, 1] to PIL image."""
    t = tensor.float().cpu()
    t = ((t + 1) * 127.5).clamp(0, 255).to(torch.uint8)
    t = t.permute(1, 2, 0).numpy()  # CHW -> HWC
    return Image.fromarray(t)


def upscale_and_pad(img, scale, tW, tH):
    """Upscale image and pad to target dimensions (centered)."""
    w0, h0 = img.size
    sW, sH = w0 * scale, h0 * scale
    up = img.resize((sW, sH), Image.BICUBIC)

    # If target is larger than scaled, pad with black (centered)
    if tW > sW or tH > sH:
        padded = Image.new('RGB', (tW, tH), (0, 0, 0))
        paste_x = (tW - sW) // 2
        paste_y = (tH - sH) // 2
        padded.paste(up, (paste_x, paste_y))
        return padded
    elif tW < sW or tH < sH:
        # Target smaller - center crop (shouldn't happen with correct rounding)
        l = (sW - tW) // 2
        t = (sH - tH) // 2
        return up.crop((l, t, l + tW, t + tH))
    else:
        return up


def init_flashvsr_pipeline(models_dir, device='cuda'):
    """Initialize FlashVSR TinyLong pipeline with TCDecoder."""
    print(f"[FlashVSR] Initializing FlashVSR pipeline...", flush=True)
    sys.stdout.flush()
    sys.stderr.flush()

    try:
        # Model paths
        flashvsr_dir = os.path.join(models_dir, "FlashVSR-v1.1")
        diffusion_model = os.path.join(flashvsr_dir, "diffusion_pytorch_model_streaming_dmd.safetensors")
        lq_proj_model = os.path.join(flashvsr_dir, "LQ_proj_in.ckpt")
        tcdecoder_model = os.path.join(flashvsr_dir, "TCDecoder.ckpt")

        print(f"[FlashVSR] Checking model paths...", flush=True)
        print(f"[FlashVSR]   diffusion_model: {diffusion_model} (exists: {os.path.exists(diffusion_model)})", flush=True)
        print(f"[FlashVSR]   lq_proj_model: {lq_proj_model} (exists: {os.path.exists(lq_proj_model)})", flush=True)
        print(f"[FlashVSR]   tcdecoder_model: {tcdecoder_model} (exists: {os.path.exists(tcdecoder_model)})", flush=True)

        if not os.path.exists(diffusion_model):
            raise FileNotFoundError(f"Diffusion model not found: {diffusion_model}")

        print(f"[FlashVSR] Loading models from {flashvsr_dir}", flush=True)

        # Initialize model manager (no VAE for Tiny)
        print("[FlashVSR] Step 1/10: Creating ModelManager...", flush=True)
        mm = ModelManager(torch_dtype=torch.bfloat16, device="cpu")
        print("[FlashVSR] Step 2/10: Loading diffusion model...", flush=True)
        mm.load_models([diffusion_model])
        print("[FlashVSR] Step 2/10: Diffusion model loaded OK", flush=True)

        # Create pipeline (using Long pipeline for efficient handling of long videos)
        print("[FlashVSR] Step 3/10: Creating pipeline from model manager...", flush=True)
        pipe = FlashVSRTinyLongPipeline.from_model_manager(mm, device=device)
        print("[FlashVSR] Step 3/10: Pipeline created OK (using TinyLong for chunked processing)", flush=True)

        # Initialize LQ projection module
        print("[FlashVSR] Step 4/10: Creating LQ_proj_in...", flush=True)
        lq_proj = Causal_LQ4x_Proj(in_dim=3, out_dim=1536, layer_num=1)
        print("[FlashVSR] Step 4/10: LQ_proj_in created, moving to device...", flush=True)
        lq_proj = lq_proj.to(device, dtype=torch.bfloat16)
        print("[FlashVSR] Step 4/10: Attaching to denoising_model...", flush=True)
        pipe.denoising_model().LQ_proj_in = lq_proj
        print("[FlashVSR] Step 4/10: LQ_proj_in attached OK", flush=True)

        if os.path.exists(lq_proj_model):
            print(f"[FlashVSR] Step 5/10: Loading LQ_proj_in weights...", flush=True)
            lq_weights = torch.load(lq_proj_model, map_location="cpu")
            pipe.denoising_model().LQ_proj_in.load_state_dict(lq_weights, strict=True)
            print("[FlashVSR] Step 5/10: LQ_proj_in weights loaded OK", flush=True)
        else:
            print("[FlashVSR] Step 5/10: No LQ_proj_in weights file, skipping", flush=True)

        print("[FlashVSR] Step 6/10: Ensuring LQ_proj_in on device...", flush=True)
        pipe.denoising_model().LQ_proj_in.to(device)
        print("[FlashVSR] Step 6/10: LQ_proj_in on device OK", flush=True)

        # Initialize TCDecoder (replaces VAE for Tiny mode)
        print("[FlashVSR] Step 7/10: Building TCDecoder...", flush=True)
        multi_scale_channels = [512, 256, 128, 128]
        tc_decoder = build_tcdecoder(
            new_channels=multi_scale_channels,
            new_latent_channels=16 + 768
        )
        print("[FlashVSR] Step 7/10: TCDecoder built OK, attaching to pipeline...", flush=True)
        pipe.TCDecoder = tc_decoder
        print("[FlashVSR] Step 7/10: TCDecoder attached OK", flush=True)

        if os.path.exists(tcdecoder_model):
            print(f"[FlashVSR] Step 8/10: Loading TCDecoder weights...", flush=True)
            tc_weights = torch.load(tcdecoder_model, map_location="cpu")
            missing = pipe.TCDecoder.load_state_dict(tc_weights, strict=False)
            if missing.missing_keys:
                print(f"[FlashVSR] Step 8/10: TCDecoder missing keys: {len(missing.missing_keys)}", flush=True)
            else:
                print("[FlashVSR] Step 8/10: TCDecoder weights loaded OK", flush=True)
        else:
            print("[FlashVSR] Step 8/10: WARNING - No TCDecoder weights file, this may cause issues!", flush=True)

        # Set TCDecoder to eval mode and move to device
        print("[FlashVSR] Step 8/10: Setting TCDecoder to eval mode and moving to device...", flush=True)
        pipe.TCDecoder = pipe.TCDecoder.to(device, dtype=torch.bfloat16)
        pipe.TCDecoder.eval()

        # Enable VRAM management and initialize
        print("[FlashVSR] Step 9/10: Moving pipeline to device...", flush=True)
        pipe.to(device)
        print("[FlashVSR] Step 9/10: Pipeline on device OK", flush=True)

        # Enable aggressive VRAM management with CPU offloading
        # The DiT model has ~1.7B parameters (3.4GB in bfloat16)
        # By setting num_persistent_param_in_dit, we limit how many params stay in VRAM
        # The rest are automatically offloaded to CPU and swapped in/out as needed
        # Adaptive based on available VRAM - prioritize compatibility over speed
        total_vram_gb = torch.cuda.get_device_properties(0).total_memory / 1024**3 if torch.cuda.is_available() else 24
        if total_vram_gb < 8:
            num_persistent_params = 10_000_000   # ~20MB in VRAM - extreme offload for 8GB
        elif total_vram_gb < 12:
            num_persistent_params = 30_000_000   # ~60MB in VRAM for 12GB
        elif total_vram_gb < 16:
            num_persistent_params = 50_000_000   # ~100MB in VRAM for 16GB
        elif total_vram_gb < 24:
            num_persistent_params = 100_000_000  # ~200MB in VRAM for 20GB
        else:
            num_persistent_params = 200_000_000  # ~400MB in VRAM for 24GB+
        print(f"[FlashVSR] Step 9/10: VRAM detected: {total_vram_gb:.0f}GB -> keeping {num_persistent_params/1e6:.0f}M params in VRAM", flush=True)
        pipe.enable_vram_management(num_persistent_param_in_dit=num_persistent_params)
        print("[FlashVSR] Step 9/10: VRAM management enabled with CPU offloading OK", flush=True)

        print("[FlashVSR] Step 9/10: Initializing cross-attention KV...", flush=True)
        # Load prompt tensor from FlashVSR installation directory
        # The library has a hardcoded relative path that doesn't work from our script location
        prompt_tensor_path = os.path.join(FLASHVSR_PATH, "examples", "WanVSR", "prompt_tensor", "posi_prompt.pth")
        if os.path.exists(prompt_tensor_path):
            print(f"[FlashVSR] Step 9/10: Loading prompt tensor from {prompt_tensor_path}", flush=True)
            context_tensor = torch.load(prompt_tensor_path, map_location=device)
            pipe.init_cross_kv(context_tensor=context_tensor)
        else:
            print(f"[FlashVSR] Step 9/10: WARNING - Prompt tensor not found at {prompt_tensor_path}", flush=True)
            # Try changing to FlashVSR directory and calling without tensor
            original_cwd = os.getcwd()
            try:
                os.chdir(os.path.join(FLASHVSR_PATH, "diffsynth", "pipelines"))
                pipe.init_cross_kv()
            finally:
                os.chdir(original_cwd)
        print("[FlashVSR] Step 9/10: cross-attention KV initialized OK", flush=True)

        print("[FlashVSR] Step 10/10: Loading models to device...", flush=True)
        # Tiny mode uses TCDecoder instead of VAE - only load dit to save VRAM
        pipe.load_models_to_device(["dit"])

        # Report actual VRAM usage
        if torch.cuda.is_available():
            allocated = torch.cuda.memory_allocated() / 1024**3
            reserved = torch.cuda.memory_reserved() / 1024**3
            print(f"[FlashVSR] VRAM after model load: {allocated:.2f}GB allocated, {reserved:.2f}GB reserved", flush=True)

        print("[FlashVSR] Step 10/10: DIT model loaded to device OK (no VAE needed for Tiny)", flush=True)

        print("[FlashVSR] TINY pipeline initialized successfully!", flush=True)
        sys.stdout.flush()
        sys.stderr.flush()
        return pipe

    except Exception as e:
        import traceback
        print(f"[FlashVSR] ERROR initializing TINY pipeline: {e}", flush=True)
        print("[FlashVSR] Full traceback:", flush=True)
        traceback.print_exc()
        sys.stdout.flush()
        sys.stderr.flush()
        raise


def process_frames_flashvsr(pipe, frame_paths, output_dir, config, device):
    """Process frames using FlashVSR pipeline with chunked processing for memory efficiency."""
    num_frames = len(frame_paths)
    scale_factor = config.get('scale_factor', 4)
    cleanup_mode = config.get('cleanup_mode', False)

    # Get original dimensions from first frame first (needed for chunk size calculation)
    with Image.open(frame_paths[0]) as img:
        orig_w, orig_h = img.size

    sW, sH, tW, tH = compute_scaled_dims(orig_w, orig_h, scale=scale_factor, multiple=128)

    overlap_frames = config.get('overlap_frames', 8)
    # FlashVSR TinyLong trims ~4 boundary frames from output
    expected_trim_frames = 4

    # Detect available VRAM for adaptive settings
    available_vram_gb = 24.0  # Default assumption
    if torch.cuda.is_available():
        total_vram = torch.cuda.get_device_properties(0).total_memory / 1024**3
        allocated_vram = torch.cuda.memory_allocated() / 1024**3
        available_vram_gb = total_vram - allocated_vram - 1.0  # Leave 1GB headroom
        print(f"[FlashVSR] Detected GPU VRAM: {total_vram:.1f}GB total, ~{available_vram_gb:.1f}GB available for inference", flush=True)

    # AGGRESSIVE VRAM optimization for consumer hardware
    resolution_pixels = sW * sH
    resolution_mp = resolution_pixels / 1000000.0

    # Calculate VRAM budget based on available memory
    inference_vram_budget = available_vram_gb - 0.5  # Reserve 0.5GB for safety

    # Estimate VRAM needed per megapixel (empirical: ~6 GB per MP with TCDecoder)
    vram_per_mp = 6.0
    max_mp = inference_vram_budget / vram_per_mp
    max_resolution = int(max_mp * 1000000)

    print(f"[FlashVSR] VRAM budget: {inference_vram_budget:.1f}GB -> max ~{max_mp:.1f}MP", flush=True)

    # Tile size for TCDecoder (smaller = less VRAM, slower)
    if available_vram_gb < 8:
        tile_size = (9, 9)  # Very small tiles for 8GB cards
        tile_stride = (5, 5)
    elif available_vram_gb < 12:
        tile_size = (17, 17)  # Small tiles for 12GB cards
        tile_stride = (9, 9)
    elif available_vram_gb < 16:
        tile_size = (25, 25)  # Medium tiles for 16GB cards
        tile_stride = (13, 13)
    else:
        tile_size = (34, 34)  # Default for 24GB+ cards
        tile_stride = (18, 18)

    print(f"[FlashVSR] Tile size: {tile_size} (stride: {tile_stride})", flush=True)

    # Check resolution limit for ULTRA mode (TinyLong + TCDecoder)
    # TCDecoder is more efficient than VAE, allowing ~1.5x higher resolution
    if resolution_pixels > max_resolution * 1.5:
        print(f"[FlashVSR] ERROR: Resolution {sW}x{sH} ({resolution_mp:.1f}MP) exceeds limit for your GPU", flush=True)
        print(f"[FlashVSR] Please use 2x scale or CLEANUP mode", flush=True)
        raise MemoryError(f"Resolution too high for {available_vram_gb:.0f}GB VRAM: {sW}x{sH}")

    # Scale chunk size based on resolution - always conservative
    if resolution_pixels > 2000000 or available_vram_gb < 12:
        chunk_size = 25  # Minimum is 25 (8*3+1) for TinyLong pipeline
    elif resolution_pixels > 1000000 or available_vram_gb < 16:
        chunk_size = 25  # 8*3+1
    elif resolution_pixels > 500000 or available_vram_gb < 20:
        chunk_size = 33  # 8*4+1
    else:
        chunk_size = 41  # 8*5+1 - only for low res + high VRAM

    print(f"[FlashVSR] Chunk size: {chunk_size} for {resolution_mp:.1f}MP @ {available_vram_gb:.0f}GB VRAM", flush=True)

    os.makedirs(output_dir, exist_ok=True)

    print(f"[FlashVSR] Original: {orig_w}x{orig_h} -> Scaled: {sW}x{sH} -> Target: {tW}x{tH}")

    start_time = time.time()
    frames_saved = 0

    # Sparse attention ratio - lower = less VRAM, potentially lower quality
    # Adjust based on available VRAM
    if available_vram_gb < 8:
        sparse_ratio = 1.0  # Very sparse for 8GB cards
        kv_ratio = 2.0
    elif available_vram_gb < 12:
        sparse_ratio = 1.5  # Sparse for 12GB cards
        kv_ratio = 2.5
    elif available_vram_gb < 16:
        sparse_ratio = 1.75  # Moderate for 16GB cards
        kv_ratio = 3.0
    else:
        sparse_ratio = 2.0  # Default for 24GB+ cards
        kv_ratio = 3.0

    print(f"[FlashVSR] Attention: sparse_ratio={sparse_ratio}, kv_ratio={kv_ratio}", flush=True)
    print(f"[FlashVSR] Processing {num_frames} frames in ULTRA mode (TinyLong + TCDecoder)", flush=True)
    print(f"[FlashVSR] Scale factor: {scale_factor}", flush=True)
    print(f"[FlashVSR] Cleanup mode: {cleanup_mode}", flush=True)
    print(f"[FlashVSR] Chunk size: {chunk_size} frames, overlap: {overlap_frames}, expected trim: {expected_trim_frames}", flush=True)

    # Calculate chunks
    # effective_chunk_size accounts for: overlap (for temporal smoothing) + trimmed frames (FlashVSR boundary trim)
    # This ensures consecutive chunks seamlessly cover all input frames
    effective_chunk_size = chunk_size - overlap_frames - expected_trim_frames
    num_chunks = max(1, (num_frames + effective_chunk_size - 1) // effective_chunk_size)

    # Track actual trim to adjust if different from expected
    actual_trim_frames = expected_trim_frames

    print(f"[FlashVSR] Will process in {num_chunks} chunks", flush=True)

    # Async frame saving for better performance
    save_executor = ThreadPoolExecutor(max_workers=4)
    save_futures = []
    saved_frame_indices = set()  # Track which frames have been saved to avoid duplicates

    def save_frame_async(pil_image, path):
        """Save PIL image to path (thread-safe)."""
        pil_image.save(path)

    # Minimum frames for FlashVSR TinyLong pipeline
    # The pipeline requires: process_total_num = (num_frames - 1) // 8 - 2 >= 1
    # This means: num_frames >= 25 (8*3+1) is the absolute minimum
    # Higher resolution needs fewer frames to fit in VRAM
    if resolution_pixels > 1500000 or available_vram_gb < 16:
        min_frames = 25  # 8*3+1 - minimum that works
    else:
        min_frames = 33  # 8*4+1 - better quality with more temporal context

    print(f"[FlashVSR] Minimum frames per chunk: {min_frames} (8n+1 format)", flush=True)

    for chunk_idx in range(num_chunks):
        chunk_start = chunk_idx * effective_chunk_size
        chunk_end = min(chunk_start + chunk_size, num_frames)

        # For last chunk, extend backwards if too small
        if chunk_end - chunk_start < min_frames:
            chunk_start = max(0, chunk_end - min_frames)
            if chunk_idx > 0:
                print(f"[FlashVSR] Extended last chunk backwards to get {chunk_end - chunk_start} frames", flush=True)

        chunk_frame_paths = frame_paths[chunk_start:chunk_end]
        chunk_len = len(chunk_frame_paths)

        print(f"[FlashVSR] Chunk {chunk_idx+1}/{num_chunks}: frames {chunk_start}-{chunk_end-1} ({chunk_len} frames)", flush=True)

        # Pad chunk for 8n+1 format
        padded_paths = list(chunk_frame_paths)
        while len(padded_paths) < min_frames:
            padded_paths.append(chunk_frame_paths[-1])  # Repeat last frame

        # Ensure 8n+1 format
        target_frame_count = largest_8n1_leq(len(padded_paths) + 4)  # +4 for padding headroom
        if target_frame_count < min_frames:
            target_frame_count = min_frames

        # Pad to target
        while len(padded_paths) < target_frame_count:
            padded_paths.append(chunk_frame_paths[-1])
        padded_paths = padded_paths[:target_frame_count]

        print(f"[FlashVSR] Chunk padded to {target_frame_count} frames (8n+1 format)", flush=True)

        # Load chunk frames as LQ tensor
        frames_tensor = []
        for i, path in enumerate(padded_paths):
            with Image.open(path).convert('RGB') as img:
                img_scaled = upscale_and_pad(img, scale_factor, tW, tH)
                frames_tensor.append(pil_to_tensor(img_scaled, torch.bfloat16, device))

        # Stack frames: (1, C, F, H, W)
        LQ_video = torch.stack(frames_tensor, dim=0)  # (F, C, H, W)
        del frames_tensor  # Free memory
        LQ_video = LQ_video.permute(1, 0, 2, 3).unsqueeze(0)  # (1, C, F, H, W)

        print(f"[FlashVSR] Chunk LQ tensor shape: {LQ_video.shape}", flush=True)

        # Run FlashVSR inference on this chunk
        torch.cuda.empty_cache()

        # Report VRAM before inference
        if torch.cuda.is_available():
            allocated = torch.cuda.memory_allocated() / 1024**3
            reserved = torch.cuda.memory_reserved() / 1024**3
            print(f"[FlashVSR] VRAM before inference: {allocated:.2f}GB allocated, {reserved:.2f}GB reserved", flush=True)

        with torch.no_grad():
            kwargs = dict(
                prompt="",
                negative_prompt="",
                cfg_scale=1.0,
                num_inference_steps=1,
                seed=0,
                LQ_video=LQ_video,
                num_frames=target_frame_count,
                height=tH,
                width=tW,
                is_full_block=False,
                if_buffer=True,
                # Adaptive sparse attention based on VRAM
                topk_ratio=sparse_ratio * 768 * 1280 / (tH * tW),
                kv_ratio=kv_ratio,  # Adaptive based on VRAM
                local_range=9 if available_vram_gb < 16 else 11,  # Smaller range for low VRAM
                color_fix=True,
                # Enable tiled VAE processing with adaptive tile size
                tiled=True,
                tile_size=tile_size,
                tile_stride=tile_stride,
            )

            output_video = pipe(**kwargs)

        # Report VRAM after inference
        if torch.cuda.is_available():
            allocated = torch.cuda.memory_allocated() / 1024**3
            reserved = torch.cuda.memory_reserved() / 1024**3
            peak = torch.cuda.max_memory_allocated() / 1024**3
            print(f"[FlashVSR] VRAM after inference: {allocated:.2f}GB allocated, peak: {peak:.2f}GB", flush=True)
            # Reset peak tracking for next chunk
            torch.cuda.reset_peak_memory_stats()

        # Free input tensor
        del LQ_video
        torch.cuda.empty_cache()

        # Convert output to frames
        print(f"[FlashVSR] Raw output shape: {output_video.shape}", flush=True)
        output_frames = rearrange(output_video, "C T H W -> T C H W")
        print(f"[FlashVSR] Output frames shape: {output_frames.shape}", flush=True)
        del output_video

        output_frame_count = output_frames.shape[0]
        frames_trimmed = target_frame_count - output_frame_count

        # On first chunk, detect actual trim and warn if different from expected
        if chunk_idx == 0:
            actual_trim_frames = frames_trimmed
            if frames_trimmed != expected_trim_frames:
                print(f"[FlashVSR] Note: Pipeline trims {frames_trimmed} frames (expected {expected_trim_frames})", flush=True)
                print(f"[FlashVSR] Outputs {output_frame_count} frames for {target_frame_count} input", flush=True)
            else:
                print(f"[FlashVSR] Pipeline outputs {output_frame_count}/{target_frame_count} frames (trims {frames_trimmed} as expected)", flush=True)

        # Calculate which frames to save from this chunk
        # FlashVSR trims boundary frames from the output, so output frame i corresponds to input frame i
        #
        # For chunk 0: save all output frames (0 to output_frame_count-1)
        # For chunk N: skip overlap_frames to avoid re-saving frames already saved by previous chunk
        save_start = 0 if chunk_idx == 0 else overlap_frames
        # Limit save_end to actual output frames available
        save_end = min(chunk_len, output_frame_count)

        for i in range(save_start, save_end):
            frame_idx = chunk_start + i  # Global frame index
            if frame_idx >= num_frames:
                break

            # Skip if this frame was already saved by a previous chunk
            if frame_idx in saved_frame_indices:
                continue
            saved_frame_indices.add(frame_idx)

            frame_tensor = output_frames[i]

            # Debug: show frame tensor shape on first frame
            if frame_idx == 0:
                print(f"[FlashVSR] Frame tensor shape: {frame_tensor.shape}", flush=True)
                print(f"[FlashVSR] Expected crop: tW={tW}, tH={tH}, sW={sW}, sH={sH}", flush=True)

            # If cleanup mode, resize back to original dimensions
            if cleanup_mode:
                frame_pil = tensor_to_pil(frame_tensor)
                frame_pil = frame_pil.resize((orig_w, orig_h), Image.LANCZOS)
            else:
                # Crop to actual scaled dimensions if different from target
                if sW != tW or sH != tH:
                    l = (tW - sW) // 2
                    t = (tH - sH) // 2
                    frame_tensor = frame_tensor[:, t:t+sH, l:l+sW]
                frame_pil = tensor_to_pil(frame_tensor)

            # Save with same filename as input (async for better throughput)
            output_filename = os.path.basename(frame_paths[frame_idx])
            output_path = os.path.join(output_dir, output_filename)
            future = save_executor.submit(save_frame_async, frame_pil, output_path)
            save_futures.append(future)
            frames_saved = len(saved_frame_indices)

            # Progress output
            elapsed = time.time() - start_time
            fps = frames_saved / elapsed if elapsed > 0 else 0
            remaining = (num_frames - frames_saved) / fps if fps > 0 else 0
            print(f"[Frame {frames_saved}/{num_frames}] fps: {fps:.2f} ETA: {remaining:.1f}s", flush=True)

        # Clean up chunk memory aggressively
        del output_frames
        torch.cuda.empty_cache()
        torch.cuda.ipc_collect()
        gc.collect()  # Force Python garbage collection

        print(f"[FlashVSR] Chunk {chunk_idx+1}/{num_chunks} complete, saved {frames_saved} frames so far", flush=True)

        # Early termination: stop if all frames have been saved
        if len(saved_frame_indices) >= num_frames:
            print(f"[FlashVSR] All {num_frames} frames saved, stopping early", flush=True)
            break

    # Wait for all async saves to complete
    print(f"[FlashVSR] Waiting for {len(save_futures)} pending saves...", flush=True)
    for future in save_futures:
        future.result()  # Wait for completion, raises if error
    save_executor.shutdown(wait=True)
    print(f"[FlashVSR] All saves complete", flush=True)

    total_time = time.time() - start_time
    avg_fps = frames_saved / total_time if total_time > 0 else 0
    print(f"[FlashVSR] Complete! Processed {frames_saved} frames in {total_time:.1f}s ({avg_fps:.2f} fps)")


def main():
    print("[FlashVSR] Script starting...", flush=True)
    sys.stdout.flush()

    parser = argparse.ArgumentParser(description='FlashVSR Video Upscaling')
    parser.add_argument('--config', type=str, required=True, help='Path to config JSON')
    args = parser.parse_args()

    print(f"[FlashVSR] Args parsed, loading config from {args.config}", flush=True)

    # Load config
    with open(args.config, 'r') as f:
        config = json.load(f)

    print(f"[FlashVSR] Config loaded from {args.config}", flush=True)
    print(f"[FlashVSR] Input dir: {config['input_dir']}", flush=True)
    print(f"[FlashVSR] Output dir: {config['output_dir']}", flush=True)
    print(f"[FlashVSR] Model: {config['model_name']}", flush=True)
    print(f"[FlashVSR] Using ULTRA mode (TinyLong + TCDecoder)", flush=True)

    # Setup device
    use_gpu = config.get('use_gpu', True)
    gpu_id = config.get('gpu_id', 0)

    if use_gpu and torch.cuda.is_available():
        device = f'cuda:{gpu_id}'
        print(f"[FlashVSR] Using CUDA device {gpu_id}: {torch.cuda.get_device_name(gpu_id)}", flush=True)

        # Enable cuDNN optimizations
        torch.backends.cudnn.benchmark = True
        torch.backends.cudnn.deterministic = False
        print("[FlashVSR] cuDNN benchmark mode enabled", flush=True)

        # Set memory allocation strategy for better memory reuse
        # This prevents fragmentation and reduces peak memory usage
        if hasattr(torch.cuda, 'set_per_process_memory_fraction'):
            # Limit to 90% of GPU memory to leave headroom for system
            try:
                torch.cuda.set_per_process_memory_fraction(0.9, gpu_id)
                print("[FlashVSR] GPU memory limited to 90% to prevent system freeze", flush=True)
            except Exception as e:
                print(f"[FlashVSR] Could not set memory fraction: {e}", flush=True)

        # Enable memory-efficient attention if available (PyTorch 2.0+)
        if hasattr(torch.backends.cuda, 'enable_flash_sdp'):
            torch.backends.cuda.enable_flash_sdp(True)
            print("[FlashVSR] Flash SDP attention enabled", flush=True)

    else:
        print("[FlashVSR] ERROR: FlashVSR requires CUDA GPU", flush=True)
        sys.exit(1)

    # Get frame paths
    print(f"[FlashVSR] Scanning for frames in {config['input_dir']}...", flush=True)
    frame_paths = get_frame_paths(config['input_dir'])
    if not frame_paths:
        print(f"[FlashVSR] ERROR: No frames found in {config['input_dir']}", flush=True)
        sys.exit(1)

    print(f"[FlashVSR] Found {len(frame_paths)} frames", flush=True)

    # Models directory
    models_dir = os.path.join(os.path.dirname(__file__), '..', 'models', 'upscale')
    models_dir = os.path.normpath(models_dir)

    print(f"[FlashVSR] Models dir: {models_dir}", flush=True)

    try:
        # Initialize FlashVSR TinyLong pipeline with TCDecoder
        print(f"[FlashVSR] Initializing pipeline...", flush=True)
        pipe = init_flashvsr_pipeline(models_dir, device=device)

        print(f"[FlashVSR] Pipeline initialized, starting frame processing...", flush=True)

        # Process frames
        process_frames_flashvsr(pipe, frame_paths, config['output_dir'], config, device)

        print("[FlashVSR] Done!", flush=True)

    except Exception as e:
        import traceback
        print(f"[FlashVSR] FATAL ERROR: {e}", flush=True)
        print("[FlashVSR] Full traceback:", flush=True)
        traceback.print_exc()
        sys.stdout.flush()
        sys.stderr.flush()
        sys.exit(1)


if __name__ == '__main__':
    try:
        main()
    except SystemExit:
        raise  # Let sys.exit() work normally
    except Exception as e:
        import traceback
        print(f"[FlashVSR] UNCAUGHT EXCEPTION: {e}", flush=True)
        traceback.print_exc()
        sys.stdout.flush()
        sys.stderr.flush()
        sys.exit(1)
