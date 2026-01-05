"""
build_video_dataset.py

Builds a diverse video frame dataset for temporal coherence training.
Downloads free stock videos or processes local videos, extracts frame pairs
with significant motion, and saves them efficiently.

Usage:
    python build_video_dataset.py --output C:/ProgramData/EWOCvj2/datasets/video
    python build_video_dataset.py --source /path/to/local/videos --output ./video_dataset
    python build_video_dataset.py --download --output ./video_dataset

License: GPL3
"""

import argparse
import os
import sys
import glob
import json
import random
import hashlib
from pathlib import Path

try:
    import cv2
    import numpy as np
    from PIL import Image
except ImportError as e:
    print(f"[ERROR] Missing required package: {e}")
    print("[ERROR] Install with: pip install opencv-python numpy pillow")
    sys.exit(1)

# Target resolution for frames
TARGET_SIZE = 1024
JPEG_QUALITY = 90

# Motion detection threshold (frames with less motion are skipped)
MIN_MOTION_THRESHOLD = 0.5  # Percentage of pixels that must change
MAX_MOTION_THRESHOLD = 30.0  # Skip frames with too much motion (scene cuts)

# Frame extraction settings
MIN_FRAME_INTERVAL = 1   # Minimum frames between pairs (1 = consecutive)
MAX_FRAME_INTERVAL = 3   # Maximum frames between pairs (for varied motion speed)
PAIRS_PER_VIDEO = 20     # Max frame pairs to extract per video

# Free video sources (Pexels API - free for commercial use)
PEXELS_API_KEY = ""  # User must provide their own API key
SAMPLE_VIDEO_URLS = [
    # These are example URLs - in practice, use Pexels API or local videos
    # Format: (url, description)
]


def compute_motion_score(frame1, frame2):
    """
    Compute motion score between two frames.
    Returns percentage of pixels with significant change.
    """
    # Convert to grayscale
    gray1 = cv2.cvtColor(frame1, cv2.COLOR_BGR2GRAY)
    gray2 = cv2.cvtColor(frame2, cv2.COLOR_BGR2GRAY)

    # Compute absolute difference
    diff = cv2.absdiff(gray1, gray2)

    # Threshold to binary (pixels with significant change)
    _, thresh = cv2.threshold(diff, 25, 255, cv2.THRESH_BINARY)

    # Calculate percentage of changed pixels
    motion_score = (np.count_nonzero(thresh) / thresh.size) * 100

    return motion_score


def compute_optical_flow_score(frame1, frame2):
    """
    Compute optical flow magnitude as motion score.
    More accurate but slower than pixel difference.
    """
    gray1 = cv2.cvtColor(frame1, cv2.COLOR_BGR2GRAY)
    gray2 = cv2.cvtColor(frame2, cv2.COLOR_BGR2GRAY)

    # Compute dense optical flow
    flow = cv2.calcOpticalFlowFarneback(
        gray1, gray2, None,
        pyr_scale=0.5, levels=3, winsize=15,
        iterations=3, poly_n=5, poly_sigma=1.2, flags=0
    )

    # Compute flow magnitude
    magnitude = np.sqrt(flow[..., 0]**2 + flow[..., 1]**2)

    # Average magnitude (pixels moved on average)
    avg_motion = np.mean(magnitude)

    return avg_motion


def resize_frame(frame, target_size):
    """
    Resize frame to target size with center crop to maintain aspect ratio.
    """
    h, w = frame.shape[:2]

    # Calculate scale to fit target size
    scale = max(target_size / w, target_size / h)
    new_w = int(w * scale)
    new_h = int(h * scale)

    # Resize with high-quality interpolation
    resized = cv2.resize(frame, (new_w, new_h), interpolation=cv2.INTER_LANCZOS4)

    # Center crop to exact target size
    start_x = (new_w - target_size) // 2
    start_y = (new_h - target_size) // 2
    cropped = resized[start_y:start_y + target_size, start_x:start_x + target_size]

    return cropped


def extract_frame_pairs(video_path, output_dir, max_pairs=PAIRS_PER_VIDEO):
    """
    Extract diverse frame pairs from a video file.
    Returns number of pairs extracted.
    """
    cap = cv2.VideoCapture(video_path)
    if not cap.isOpened():
        print(f"[WARNING] Cannot open video: {video_path}")
        return 0

    fps = cap.get(cv2.CAP_PROP_FPS)
    total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))

    if total_frames < 10:
        print(f"[WARNING] Video too short: {video_path}")
        cap.release()
        return 0

    print(f"[Processing] {os.path.basename(video_path)}: {total_frames} frames @ {fps:.1f} fps")

    # Generate candidate frame positions (spread across video)
    # Skip first/last 5% to avoid intros/outros
    start_frame = int(total_frames * 0.05)
    end_frame = int(total_frames * 0.95)

    # Sample frame positions
    if end_frame - start_frame < max_pairs * 10:
        step = 2
    else:
        step = (end_frame - start_frame) // (max_pairs * 2)

    candidate_positions = list(range(start_frame, end_frame - MAX_FRAME_INTERVAL, step))
    random.shuffle(candidate_positions)

    pairs_extracted = 0
    video_hash = hashlib.md5(video_path.encode()).hexdigest()[:8]

    for pos in candidate_positions:
        if pairs_extracted >= max_pairs:
            break

        # Read first frame
        cap.set(cv2.CAP_PROP_POS_FRAMES, pos)
        ret1, frame1 = cap.read()
        if not ret1:
            continue

        # Choose frame interval (1-3 frames apart for varied motion)
        interval = random.randint(MIN_FRAME_INTERVAL, MAX_FRAME_INTERVAL)

        # Read second frame
        cap.set(cv2.CAP_PROP_POS_FRAMES, pos + interval)
        ret2, frame2 = cap.read()
        if not ret2:
            continue

        # Check motion score
        motion = compute_motion_score(frame1, frame2)

        if motion < MIN_MOTION_THRESHOLD:
            # Too little motion - skip
            continue

        if motion > MAX_MOTION_THRESHOLD:
            # Too much motion (likely scene cut) - skip
            continue

        # Resize frames
        frame1_resized = resize_frame(frame1, TARGET_SIZE)
        frame2_resized = resize_frame(frame2, TARGET_SIZE)

        # Create sequence folder
        seq_name = f"seq_{video_hash}_{pairs_extracted:04d}"
        seq_dir = os.path.join(output_dir, seq_name)
        os.makedirs(seq_dir, exist_ok=True)

        # Save frames as JPEG
        frame1_path = os.path.join(seq_dir, "frame_0000.jpg")
        frame2_path = os.path.join(seq_dir, "frame_0001.jpg")

        # Convert BGR to RGB for saving
        cv2.imwrite(frame1_path, frame1_resized, [cv2.IMWRITE_JPEG_QUALITY, JPEG_QUALITY])
        cv2.imwrite(frame2_path, frame2_resized, [cv2.IMWRITE_JPEG_QUALITY, JPEG_QUALITY])

        pairs_extracted += 1

    cap.release()
    print(f"[Extracted] {pairs_extracted} frame pairs from {os.path.basename(video_path)}")

    return pairs_extracted


def process_local_videos(source_dir, output_dir, target_pairs=500):
    """
    Process all videos in source directory.
    """
    # Find all video files
    video_extensions = ['*.mp4', '*.avi', '*.mov', '*.mkv', '*.webm', '*.m4v']
    video_files = []

    for ext in video_extensions:
        video_files.extend(glob.glob(os.path.join(source_dir, ext)))
        video_files.extend(glob.glob(os.path.join(source_dir, ext.upper())))
        # Also search subdirectories
        video_files.extend(glob.glob(os.path.join(source_dir, '**', ext), recursive=True))

    video_files = list(set(video_files))  # Remove duplicates

    if not video_files:
        print(f"[ERROR] No video files found in {source_dir}")
        return 0

    print(f"[Found] {len(video_files)} video files")

    # Calculate pairs per video
    pairs_per_video = max(5, target_pairs // len(video_files))

    total_pairs = 0
    for video_path in video_files:
        if total_pairs >= target_pairs:
            break

        remaining = target_pairs - total_pairs
        max_pairs = min(pairs_per_video, remaining)

        pairs = extract_frame_pairs(video_path, output_dir, max_pairs)
        total_pairs += pairs

    return total_pairs


def download_pexels_videos(api_key, output_dir, queries, videos_per_query=5):
    """
    Download videos from Pexels API.
    Requires API key from https://www.pexels.com/api/
    """
    try:
        import requests
    except ImportError:
        print("[ERROR] requests package required for downloading")
        print("[ERROR] Install with: pip install requests")
        return []

    headers = {"Authorization": api_key}
    downloaded = []

    for query in queries:
        print(f"[Pexels] Searching for: {query}")

        url = f"https://api.pexels.com/videos/search?query={query}&per_page={videos_per_query}&size=medium"

        try:
            response = requests.get(url, headers=headers, timeout=30)
            response.raise_for_status()
            data = response.json()
        except Exception as e:
            print(f"[WARNING] Failed to search Pexels: {e}")
            continue

        for video in data.get('videos', []):
            # Get medium quality video file
            video_files = video.get('video_files', [])
            medium_files = [f for f in video_files if f.get('quality') == 'hd' and f.get('width', 0) >= 1280]

            if not medium_files:
                medium_files = [f for f in video_files if f.get('quality') == 'sd']

            if not medium_files:
                continue

            video_url = medium_files[0]['link']
            video_id = video['id']

            # Download video
            video_path = os.path.join(output_dir, f"pexels_{video_id}.mp4")

            if os.path.exists(video_path):
                print(f"[Skip] Already downloaded: {video_id}")
                downloaded.append(video_path)
                continue

            print(f"[Download] Pexels video {video_id}...")

            try:
                video_response = requests.get(video_url, timeout=120)
                video_response.raise_for_status()

                with open(video_path, 'wb') as f:
                    f.write(video_response.content)

                downloaded.append(video_path)
                print(f"[Downloaded] {video_path}")

            except Exception as e:
                print(f"[WARNING] Failed to download video {video_id}: {e}")

    return downloaded


def create_dataset_info(output_dir, total_pairs):
    """
    Create dataset info JSON file.
    """
    info = {
        "name": "EWOCvj2 Temporal Training Dataset",
        "version": "1.0",
        "resolution": TARGET_SIZE,
        "total_sequences": total_pairs,
        "frames_per_sequence": 2,
        "format": "JPEG",
        "quality": JPEG_QUALITY,
        "description": "Frame pairs for temporal coherence training in style transfer"
    }

    info_path = os.path.join(output_dir, "dataset_info.json")
    with open(info_path, 'w') as f:
        json.dump(info, f, indent=2)

    print(f"[Created] Dataset info: {info_path}")


def main():
    parser = argparse.ArgumentParser(description='Build video frame dataset for temporal training')
    parser.add_argument('--source', type=str, default='',
                       help='Source directory with local video files')
    parser.add_argument('--output', type=str, required=True,
                       help='Output directory for frame pairs')
    parser.add_argument('--download', action='store_true',
                       help='Download videos from Pexels (requires API key)')
    parser.add_argument('--pexels-key', type=str, default='',
                       help='Pexels API key for downloading')
    parser.add_argument('--pairs', type=int, default=500,
                       help='Target number of frame pairs (default: 500)')
    parser.add_argument('--resolution', type=int, default=1024,
                       help='Target resolution (default: 1024)')

    args = parser.parse_args()

    global TARGET_SIZE
    TARGET_SIZE = args.resolution

    # Create output directory
    os.makedirs(args.output, exist_ok=True)

    total_pairs = 0

    # Download from Pexels if requested
    if args.download:
        if not args.pexels_key:
            print("[ERROR] Pexels API key required for downloading")
            print("[ERROR] Get a free key at: https://www.pexels.com/api/")
            print("[ERROR] Usage: --download --pexels-key YOUR_KEY")
        else:
            # Diverse motion categories
            queries = [
                "walking people",
                "running water",
                "driving car",
                "flying birds",
                "ocean waves",
                "city traffic",
                "wind trees",
                "clouds sky",
                "sports action",
                "dance movement",
                "swimming fish",
                "fire flames",
                "rain drops",
                "snow falling",
                "crowd people"
            ]

            temp_video_dir = os.path.join(args.output, "_temp_videos")
            os.makedirs(temp_video_dir, exist_ok=True)

            downloaded = download_pexels_videos(
                args.pexels_key, temp_video_dir, queries,
                videos_per_query=3
            )

            if downloaded:
                total_pairs = process_local_videos(temp_video_dir, args.output, args.pairs)

    # Process local videos
    if args.source and os.path.isdir(args.source):
        pairs = process_local_videos(args.source, args.output, args.pairs - total_pairs)
        total_pairs += pairs

    if total_pairs == 0:
        print("\n[WARNING] No frame pairs extracted!")
        print("[INFO] To build dataset, either:")
        print("  1. Provide local videos: --source /path/to/videos")
        print("  2. Download from Pexels: --download --pexels-key YOUR_KEY")
        print("\nExample with local videos:")
        print(f"  python {sys.argv[0]} --source ~/Videos --output {args.output}")
        print("\nExample downloading from Pexels:")
        print(f"  python {sys.argv[0]} --download --pexels-key YOUR_KEY --output {args.output}")
        return

    # Create dataset info
    create_dataset_info(args.output, total_pairs)

    # Calculate dataset size
    total_size = 0
    for root, dirs, files in os.walk(args.output):
        for f in files:
            if f.endswith('.jpg'):
                total_size += os.path.getsize(os.path.join(root, f))

    print(f"\n[SUCCESS] Dataset created!")
    print(f"  Location: {args.output}")
    print(f"  Frame pairs: {total_pairs}")
    print(f"  Resolution: {TARGET_SIZE}x{TARGET_SIZE}")
    print(f"  Total size: {total_size / (1024*1024):.1f} MB")


if __name__ == "__main__":
    main()
