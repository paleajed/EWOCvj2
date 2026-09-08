"""
EWOCvj2 LTX Conditioning Utils

Restores the unprocessed_ltxav_embeds flag on conditioning loaded from
LTXVSaveConditioning/LTXVLoadConditioning. The Gemma encoder marks its output
with this flag so the diffusion model knows to run preprocess_text_embeds on it.
LTXVSaveConditioning only saves the tensor + attention_mask, dropping the flag.
This node re-adds it so the saved conditioning round-trips correctly.
"""


class LTXMarkEmbedsUnprocessed:
    RETURN_TYPES = ("CONDITIONING",)
    FUNCTION = "execute"
    CATEGORY = "ewocvj2/ltx"

    @classmethod
    def INPUT_TYPES(cls):
        return {"required": {"conditioning": ("CONDITIONING",)}}

    def execute(self, conditioning):
        result = []
        for tensor, opts in conditioning:
            new_opts = dict(opts)
            new_opts["unprocessed_ltxav_embeds"] = True
            result.append([tensor, new_opts])
        return (result,)


NODE_CLASS_MAPPINGS = {
    "LTXMarkEmbedsUnprocessed": LTXMarkEmbedsUnprocessed,
}
NODE_DISPLAY_NAME_MAPPINGS = {
    "LTXMarkEmbedsUnprocessed": "LTX Mark Embeds Unprocessed (EWOCvj2)",
}
