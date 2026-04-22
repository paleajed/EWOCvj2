"""
Download diverse videos from Pexels and extract frame pairs for temporal training
"""
import os
import sys
import json
import hashlib
import random
import urllib.request
import urllib.parse
import subprocess
import shutil

# Pexels API configuration
PEXELS_API_KEY = "YJApZD6dShnhYgWIvD9WCC3OB18anYV7Zg4Bio0UVES92e3oCW4xsP2e"
PEXELS_API_URL = "https://api.pexels.com/videos/search"

# Output configuration
OUTPUT_DIR = "C:/ProgramData/EWOCvj2/datasets/video"
TEMP_DIR = os.path.join(OUTPUT_DIR, "_temp_videos")
TARGET_RESOLUTION = 1024
FRAMES_PER_SEQUENCE = 2
TARGET_SEQUENCES = 500  # Total sequences to collect
SEQUENCES_PER_VIDEO = 20  # Extract multiple frame pairs per video

# Global ffmpeg paths (set by find_ffmpeg)
FFMPEG_PATH = None
FFPROBE_PATH = None

# Diverse search queries for varied content
SEARCH_QUERIES = [
    # Nature
    "forest", "ocean waves", "mountains", "waterfall", "sunset clouds",
    "river flowing", "wind trees", "rain drops", "snow falling", "flowers blooming",
    # Animals
    "birds flying", "fish swimming", "dog running", "cat playing", "horses galloping",
    "butterflies", "wildlife", "underwater life", "safari animals", "insects",
    # Urban/City
    "city traffic", "street walking", "neon lights", "subway train", "skyscrapers",
    "busy intersection", "night city", "urban crowd", "shopping street", "cafe scene",
    # People/Action
    "dancing", "sports action", "running", "cooking", "painting art",
    "musician playing", "yoga", "skateboarding", "surfing", "climbing",
    # Abstract/Effects
    "fire flames", "smoke", "water splash", "light particles", "abstract motion",
    "ink water", "bubbles", "sparks", "fog mist", "aurora",
    # Technology/Industrial
    "factory machines", "data center", "robotics", "3d printing", "laboratory",
    # Weather/Sky
    "thunderstorm", "clouds timelapse", "starry night", "northern lights", "foggy morning",
    # Food/Objects
    "pouring liquid", "food preparation", "coffee brewing", "candle flame", "fabric flowing"
]

def find_ffmpeg():
    """Find ffmpeg and ffprobe executables"""
    global FFMPEG_PATH, FFPROBE_PATH

    # Common locations to check on Windows
    search_paths = [
        # System PATH
        "ffmpeg",
        "ffprobe",
        # Common install locations
        "C:/Program Files/ffmpeg/bin/ffmpeg.exe",
        "C:/Program Files/ffmpeg/bin/ffprobe.exe",
        "C:/ffmpeg/bin/ffmpeg.exe",
        "C:/ffmpeg/bin/ffprobe.exe",
        # ComfyUI portable location
        "C:/ProgramData/EWOCvj2/ComfyUI/python_embeded/Scripts/ffmpeg.exe",
        "C:/ProgramData/EWOCvj2/ComfyUI/python_embeded/Scripts/ffprobe.exe",
        # Chocolatey install
        "C:/ProgramData/chocolatey/bin/ffmpeg.exe",
        "C:/ProgramData/chocolatey/bin/ffprobe.exe",
        # Scoop install
        os.path.expanduser("~/scoop/shims/ffmpeg.exe"),
        os.path.expanduser("~/scoop/shims/ffprobe.exe"),
    ]

    # Try to find ffmpeg
    ffmpeg_found = False
    ffprobe_found = False

    for path in search_paths:
        if "ffmpeg" in path and not ffmpeg_found:
            try:
                if os.path.isfile(path):
                    # Test if it works
                    result = subprocess.run([path, "-version"], capture_output=True, timeout=10)
                    if result.returncode == 0:
                        FFMPEG_PATH = path
                        ffmpeg_found = True
                        print(f"Video Frame Pairs: Found ffmpeg at {path}", file=sys.stderr)
                elif path == "ffmpeg":
                    # Try system PATH
                    result = subprocess.run(["ffmpeg", "-version"], capture_output=True, timeout=10)
                    if result.returncode == 0:
                        FFMPEG_PATH = "ffmpeg"
                        ffmpeg_found = True
                        print(f"Video Frame Pairs: Found ffmpeg in PATH", file=sys.stderr)
            except Exception:
                pass

        if "ffprobe" in path and not ffprobe_found:
            try:
                if os.path.isfile(path):
                    result = subprocess.run([path, "-version"], capture_output=True, timeout=10)
                    if result.returncode == 0:
                        FFPROBE_PATH = path
                        ffprobe_found = True
                        print(f"Video Frame Pairs: Found ffprobe at {path}", file=sys.stderr)
                elif path == "ffprobe":
                    result = subprocess.run(["ffprobe", "-version"], capture_output=True, timeout=10)
                    if result.returncode == 0:
                        FFPROBE_PATH = "ffprobe"
                        ffprobe_found = True
                        print(f"Video Frame Pairs: Found ffprobe in PATH", file=sys.stderr)
            except Exception:
                pass

    return ffmpeg_found and ffprobe_found

def get_video_id_hash(video_id):
    """Generate a short hash from video ID for folder naming"""
    return hashlib.md5(str(video_id).encode()).hexdigest()[:8]

def download_file(url, local_path, headers=None):
    """Download a file with optional headers"""
    req = urllib.request.Request(url)
    if headers:
        for key, value in headers.items():
            req.add_header(key, value)

    with urllib.request.urlopen(req, timeout=120) as response:
        with open(local_path, 'wb') as f:
            shutil.copyfileobj(response, f)
    return True

def search_pexels_videos(query, per_page=15, page=1):
    """Search for videos on Pexels"""
    url = f"{PEXELS_API_URL}?query={urllib.parse.quote(query)}&per_page={per_page}&page={page}"

    req = urllib.request.Request(url)
    req.add_header("Authorization", PEXELS_API_KEY)

    try:
        with urllib.request.urlopen(req, timeout=30) as response:
            data = json.loads(response.read().decode())
            return data.get("videos", [])
    except Exception as e:
        print(f"Video Frame Pairs: API error for '{query}': {e}", file=sys.stderr)
        return []

def get_best_video_file(video):
    """Get the best quality video file URL (prefer HD)"""
    video_files = video.get("video_files", [])

    # Sort by quality - prefer HD files around 1080p or 720p
    suitable = []
    for vf in video_files:
        height = vf.get("height", 0)
        width = vf.get("width", 0)
        if height >= 720 and vf.get("link"):
            suitable.append((vf, height))

    if not suitable:
        # Fallback to any video file
        for vf in video_files:
            if vf.get("link"):
                return vf.get("link")
        return None

    # Sort by height, prefer around 1080p
    suitable.sort(key=lambda x: abs(x[1] - 1080))
    return suitable[0][0].get("link")

def get_video_info(video_path):
    """Get video duration and fps using ffprobe"""
    try:
        probe_cmd = [
            FFPROBE_PATH, "-v", "quiet", "-print_format", "json",
            "-show_format", "-show_streams", video_path
        ]
        result = subprocess.run(probe_cmd, capture_output=True, text=True, timeout=30)
        if result.returncode != 0:
            print(f"Video Frame Pairs: ffprobe failed: {result.stderr}", file=sys.stderr)
            return None, None

        info = json.loads(result.stdout)

        # Find video stream
        video_stream = None
        for stream in info.get("streams", []):
            if stream.get("codec_type") == "video":
                video_stream = stream
                break

        if not video_stream:
            return None, None

        # Get fps
        fps_str = video_stream.get("r_frame_rate", "30/1")
        fps_parts = fps_str.split("/")
        fps = float(fps_parts[0]) / float(fps_parts[1]) if len(fps_parts) == 2 and float(fps_parts[1]) != 0 else 30.0

        # Get duration - try multiple sources
        duration = None
        if "duration" in video_stream:
            duration = float(video_stream["duration"])
        elif "format" in info and "duration" in info["format"]:
            duration = float(info["format"]["duration"])

        if duration is None or duration < 1:
            return None, None

        return duration, fps

    except Exception as e:
        print(f"Video Frame Pairs: ffprobe exception: {e}", file=sys.stderr)
        return None, None

def extract_frame_pairs(video_path, video_hash, start_seq_num, existing_count):
    """Extract frame pairs from video using ffmpeg"""
    sequences_created = []

    duration, fps = get_video_info(video_path)
    if duration is None or fps is None:
        return sequences_created

    if duration < 2:
        return sequences_created

    # Calculate frame intervals for extraction
    # Skip first and last 10% of video
    start_time = duration * 0.1
    end_time = duration * 0.9
    usable_duration = end_time - start_time

    # Space out the frame pairs evenly
    num_pairs = min(SEQUENCES_PER_VIDEO, int(usable_duration * fps / 10))
    if num_pairs < 1:
        num_pairs = 1

    time_interval = usable_duration / num_pairs

    for i in range(num_pairs):
        time_pos = start_time + (i * time_interval)
        seq_num = start_seq_num + len(sequences_created)
        seq_dir = os.path.join(OUTPUT_DIR, f"seq_{video_hash}_{seq_num:04d}")

        if os.path.exists(seq_dir):
            continue

        os.makedirs(seq_dir, exist_ok=True)

        # Frame 0
        frame0_path = os.path.join(seq_dir, "frame_0000.jpg")
        cmd0 = [
            FFMPEG_PATH, "-y", "-ss", str(time_pos),
            "-i", video_path,
            "-vframes", "1",
            "-vf", f"scale={TARGET_RESOLUTION}:{TARGET_RESOLUTION}:force_original_aspect_ratio=increase,crop={TARGET_RESOLUTION}:{TARGET_RESOLUTION}",
            "-q:v", "2",
            frame0_path
        ]
        result0 = subprocess.run(cmd0, capture_output=True, timeout=30)

        # Frame 1 (next frame - 1/fps later)
        time_pos_next = time_pos + (1.0 / fps)
        frame1_path = os.path.join(seq_dir, "frame_0001.jpg")
        cmd1 = [
            FFMPEG_PATH, "-y", "-ss", str(time_pos_next),
            "-i", video_path,
            "-vframes", "1",
            "-vf", f"scale={TARGET_RESOLUTION}:{TARGET_RESOLUTION}:force_original_aspect_ratio=increase,crop={TARGET_RESOLUTION}:{TARGET_RESOLUTION}",
            "-q:v", "2",
            frame1_path
        ]
        result1 = subprocess.run(cmd1, capture_output=True, timeout=30)

        # Verify both frames exist and have size > 0
        if (os.path.exists(frame0_path) and os.path.exists(frame1_path) and
            os.path.getsize(frame0_path) > 1000 and os.path.getsize(frame1_path) > 1000):
            sequences_created.append(seq_dir)
        else:
            # Cleanup failed extraction
            shutil.rmtree(seq_dir, ignore_errors=True)

    return sequences_created

def count_existing_sequences():
    """Count existing sequence directories"""
    if not os.path.exists(OUTPUT_DIR):
        return 0
    count = 0
    for name in os.listdir(OUTPUT_DIR):
        if name.startswith("seq_") and os.path.isdir(os.path.join(OUTPUT_DIR, name)):
            count += 1
    return count

def main():
    print("Video Frame Pairs: Starting download...")
    sys.stdout.flush()

    # Find ffmpeg
    if not find_ffmpeg():
        print("Video Frame Pairs: ERROR - ffmpeg/ffprobe not found!")
        print("Video Frame Pairs: Searched common locations and PATH")
        print("Video Frame Pairs: Please install ffmpeg: https://ffmpeg.org/download.html")
        sys.stdout.flush()
        return 1

    print(f"Video Frame Pairs: Using ffmpeg={FFMPEG_PATH}, ffprobe={FFPROBE_PATH}")
    sys.stdout.flush()

    # Create directories
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    os.makedirs(TEMP_DIR, exist_ok=True)

    # Check existing sequences
    existing = count_existing_sequences()
    print(f"Video Frame Pairs: {existing} sequences exist, target is {TARGET_SEQUENCES}")
    sys.stdout.flush()

    if existing >= TARGET_SEQUENCES:
        print(f"Video Frame Pairs: Already complete ({existing} sequences)")
        sys.stdout.flush()
        return 0

    needed = TARGET_SEQUENCES - existing
    print(f"Video Frame Pairs: Need {needed} more sequences")
    sys.stdout.flush()

    # Shuffle queries for variety
    queries = SEARCH_QUERIES.copy()
    random.shuffle(queries)

    total_created = 0
    videos_processed = 0
    videos_failed = 0
    video_ids_used = set()

    for query in queries:
        if total_created >= needed:
            break

        print(f"Video Frame Pairs: Searching for '{query}'...", file=sys.stderr)
        videos = search_pexels_videos(query, per_page=10)

        if not videos:
            print(f"Video Frame Pairs: No videos found for '{query}'", file=sys.stderr)
            continue

        for video in videos:
            if total_created >= needed:
                break

            video_id = video.get("id")
            if video_id in video_ids_used:
                continue

            video_url = get_best_video_file(video)
            if not video_url:
                continue

            video_hash = get_video_id_hash(video_id)
            video_path = os.path.join(TEMP_DIR, f"{video_hash}.mp4")

            try:
                print(f"Video Frame Pairs: Downloading video {video_id}...", file=sys.stderr)
                download_file(video_url, video_path)
                video_ids_used.add(video_id)

                sequences = extract_frame_pairs(video_path, video_hash, total_created, existing)

                if sequences:
                    total_created += len(sequences)
                    videos_processed += 1
                    print(f"Video Frame Pairs: Extracted {len(sequences)} sequences from video {video_id}", file=sys.stderr)
                else:
                    videos_failed += 1
                    print(f"Video Frame Pairs: Failed to extract frames from video {video_id}", file=sys.stderr)

                # Clean up video file
                if os.path.exists(video_path):
                    os.remove(video_path)

            except Exception as e:
                videos_failed += 1
                print(f"Video Frame Pairs: Exception processing video {video_id}: {e}", file=sys.stderr)
                if os.path.exists(video_path):
                    try:
                        os.remove(video_path)
                    except:
                        pass

            # Progress update every video
            current_total = existing + total_created
            print(f"Video Frame Pairs: {current_total}/{TARGET_SEQUENCES} sequences ({videos_processed} videos OK, {videos_failed} failed)")
            sys.stdout.flush()

    # Update dataset info
    final_count = count_existing_sequences()
    info = {
        "name": "EWOCvj2 Temporal Training Dataset",
        "version": "1.0",
        "resolution": TARGET_RESOLUTION,
        "total_sequences": final_count,
        "frames_per_sequence": FRAMES_PER_SEQUENCE,
        "format": "JPEG",
        "quality": 90,
        "description": "Frame pairs for temporal coherence training in style transfer"
    }

    with open(os.path.join(OUTPUT_DIR, "dataset_info.json"), "w") as f:
        json.dump(info, f, indent=2)

    if final_count >= TARGET_SEQUENCES:
        print(f"Video Frame Pairs: Complete! {final_count} total sequences")
    else:
        print(f"Video Frame Pairs: Partial completion - {final_count}/{TARGET_SEQUENCES} sequences")
    sys.stdout.flush()

    return 0

if __name__ == "__main__":
    sys.exit(main())
