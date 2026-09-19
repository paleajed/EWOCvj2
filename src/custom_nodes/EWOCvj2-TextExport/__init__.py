"""
EWOCvj2 Text Export

Saves an LLM node's STRING output straight to a plain text file, so EWOCvj2's C++ side can read
back a one-off text result (the "Enhance" prompt-rewrite job, workflows/*/enhance.json) without
needing a websocket/history round trip through ComfyUI's own execution-output plumbing - same
"write to a known local path, poll /history for completion, then just read the file" pattern
EwocMogeMetricExport (custom_nodes/EWOCvj2-MogeMetricExport) uses for the Camera Path Editor's
depth preview.

License: GPL3
"""

import os


class EwocSaveText:
    CATEGORY = "EWOCvj2"
    RETURN_TYPES = ()
    FUNCTION = "save"
    OUTPUT_NODE = True

    @classmethod
    def INPUT_TYPES(cls):
        return {
            "required": {
                "text": ("STRING", {"default": "", "forceInput": True}),
                "output_path": ("STRING", {"default": ""}),
            }
        }

    def save(self, text, output_path):
        out_dir = os.path.dirname(output_path)
        if out_dir:
            os.makedirs(out_dir, exist_ok=True)
        with open(output_path, "w", encoding="utf-8") as f:
            f.write(text)
        return {}


NODE_CLASS_MAPPINGS = {"EwocSaveText": EwocSaveText}
NODE_DISPLAY_NAME_MAPPINGS = {"EwocSaveText": "EWOCvj2 Save Text"}
