"""
EWOCvj2 MoGe Metric Export

Saves MoGe's raw per-pixel METRIC depth (not a percentile-normalized preview render) straight
to a binary file, so EWOCvj2's own Camera Path Editor can reconstruct its orbit-preview point
cloud in the SAME coordinate system CrossViewWarp's pivot/distance math actually operates in.

comfy_extras/nodes_moge.py's MoGeInference already returns real metric metres in
moge_geometry["depth"] - ComfyUI-CrossViewWarp's own crossview_warp_node.py confirms this
directly: "Metric metres straight from the model - no normalisation, so no scale to guess."
No existing node exposes that raw tensor to a workflow's outputs though - MoGeRender's "depth"
output only gives a percentile-normalized visualization image, which throws the real scale away.
This node writes the raw values to a plain binary file instead, read back directly by
CameraPathEditor.cpp (see PointCloudBin / buildPointCloudsThreadFunc).

License: GPL3
"""

import os
import struct
import numpy as np


class EwocMogeMetricExport:
    CATEGORY = "EWOCvj2"
    RETURN_TYPES = ()
    FUNCTION = "save"
    OUTPUT_NODE = True

    @classmethod
    def INPUT_TYPES(cls):
        return {
            "required": {
                "moge_geometry": ("MOGE_GEOMETRY",),
                "output_path": ("STRING", {"default": ""}),
            }
        }

    def save(self, moge_geometry, output_path):
        depth = moge_geometry.get("depth")
        if depth is None:
            raise ValueError("EwocMogeMetricExport: moge_geometry has no 'depth' field")
        z = depth.detach().float().cpu().numpy()  # (B, H, W), metric metres

        # -1 sentinel for invalid/masked pixels (sky, out-of-range) - mirrors
        # crossview_warp_node.py's own handling of moge_geometry's mask (build(): masked pixels
        # become NaN so the percentile cut/warp treats them as holes, not far-plane content).
        mask = moge_geometry.get("mask")
        if mask is not None:
            m = mask.detach().cpu().numpy()
            z = np.where(m > 0.5, z, -1.0)
        else:
            z = np.where(np.isfinite(z) & (z > 0), z, -1.0)

        B, H, W = z.shape

        # fx in pixels - same reading crossview_warp_node.py's build() uses for its own hfov=0
        # fallback: K[0][0, 0] is fx normalised by image width, from frame 0's intrinsics (stays
        # correct across any resize since it's stored as a fraction of width, not raw pixels).
        K = moge_geometry.get("intrinsics")
        if K is not None:
            fx = float(K[0][0, 0]) * W
        else:
            fx = W / (2.0 * np.tan(np.radians(50.0) / 2.0))

        out_dir = os.path.dirname(output_path)
        if out_dir:
            os.makedirs(out_dir, exist_ok=True)
        with open(output_path, "wb") as f:
            f.write(b"MOGZ")                             # magic
            f.write(struct.pack("<iiif", B, H, W, fx))    # header: numFrames, H, W, fx(px)
            f.write(np.ascontiguousarray(z, dtype=np.float32).tobytes())

        print(f"EwocMogeMetricExport: wrote {B}x{H}x{W} metric depth "
              f"({z.nbytes / 1e6:.1f} MB) to {output_path}")
        return {}


NODE_CLASS_MAPPINGS = {"EwocMogeMetricExport": EwocMogeMetricExport}
NODE_DISPLAY_NAME_MAPPINGS = {"EwocMogeMetricExport": "EWOCvj2 MoGe Metric Export"}
