# Copyright (c) Meta Platforms, Inc. and affiliates. All Rights Reserved

import logging

import numpy as np
import torch

from .masks_ops import mask_iou


try:
    from torch_generic_nms import generic_nms as generic_nms_cuda

    GENERIC_NMS_AVAILABLE = True
except ImportError:
    GENERIC_NMS_AVAILABLE = False


def nms_masks(
    pred_probs: torch.Tensor,
    pred_masks: torch.Tensor,
    prob_threshold: float,
    iou_threshold: float,
) -> torch.Tensor:
    """
    Args:
      - pred_probs: (num_det,) float Tensor, containing the score (probability) of each detection
      - pred_masks: (num_det, H_mask, W_mask) float Tensor, containing the binary segmentation mask of each detection
      - prob_threshold: float, score threshold to prefilter detections (NMS is performed on detections above threshold)
      - iou_threshold: float, mask IoU threshold for NMS

    Returns:
     - keep: (num_det,) bool Tensor, indicating whether each detection is kept after score thresholding + NMS
    """
    # prefilter the detections with prob_threshold ("valid" are those above prob_threshold)
    is_valid = pred_probs > prob_threshold  # (num_det,)
    probs = pred_probs[is_valid]  # (num_valid,)
    masks_binary = pred_masks[is_valid] > 0  # (num_valid, H_mask, W_mask)
    if probs.numel() == 0:
        return is_valid  # no valid detection, return empty keep mask

    ious = mask_iou(masks_binary, masks_binary)  # (num_valid, num_valid)
    kept_inds = generic_nms(ious, probs, iou_threshold)

    # valid_inds are the indices among `probs` of valid detections before NMS (or -1 for invalid)
    valid_inds = torch.where(is_valid, is_valid.cumsum(dim=0) - 1, -1)  # (num_det,)
    keep = torch.isin(valid_inds, kept_inds)  # (num_det,)
    return keep


def generic_nms(
    ious: torch.Tensor, scores: torch.Tensor, iou_threshold=0.5
) -> torch.Tensor:
    """A generic version of `torchvision.ops.nms` that takes a pairwise IoU matrix."""

    assert ious.dim() == 2 and ious.size(0) == ious.size(1)
    assert scores.dim() == 1 and scores.size(0) == ious.size(0)

    if GENERIC_NMS_AVAILABLE and ious.is_cuda:
        return generic_nms_cuda(ious, scores, iou_threshold, use_iou_matrix=True)

    return _generic_nms_bitmask(ious, scores, iou_threshold)


def _generic_nms_bitmask(
    ious: torch.Tensor, scores: torch.Tensor, iou_threshold: float = 0.5
) -> torch.Tensor:
    """
    NMS from a precomputed IoU matrix using the bitmask strategy from torchvision's CUDA NMS kernel.

    The CUDA NMS kernel (torchvision/csrc/ops/cuda/nms_kernel.cu) has two phases:
      1. Parallel GPU phase: each thread checks one (box_i, 64-box-block) pair and writes a
         64-bit suppression bitmask.  One uint64 word encodes whether each of 64 boxes is
         suppressed by box_i.
      2. Sequential CPU phase: walk boxes in score order; if not removed, mark kept and OR
         its bitmask row into `removed`.  This is O(N * N/64) uint64 OR ops.

    We replicate this exactly for a pre-computed IoU matrix:
      - Phase 1 runs on the GPU (comparison + bitmask packing in vectorised tensor ops).
      - The packed bitmask transferred to CPU is 32× smaller than the raw float IoU matrix
        (N×⌈N/64⌉ int64 vs N×N float32).
      - Phase 2 is identical: sequential greedy with bitmask OR on CPU.

    For N=500:  transfer 32 KB instead of 1 MB;  sequential phase does 4 000 uint64 OR ops.
    """
    n = scores.size(0)
    device = scores.device

    if n == 0:
        return torch.zeros(0, dtype=torch.int64, device=device)

    # --- GPU phase: sort, compare, pack -------------------------------------------

    # Sort by score descending (on GPU if tensors live there)
    order = torch.argsort(scores, descending=True)

    # Reorder IoU matrix so row/col 0 = highest-score box
    ious_sorted = ious[order][:, order]  # (n, n)

    # sup[i, j] = True  →  sorted box i suppresses sorted box j  (only j > i matters)
    sup = (ious_sorted > iou_threshold).triu(diagonal=1)  # (n, n) bool

    # Pack 64 bool columns into one int64 word (GPU vectorised, same bit layout as CUDA kernel)
    #   sup_packed[i, w]  =  bitmask of boxes [w*64 .. w*64+63] suppressed by box i
    BITS = 64
    n_words = (n + BITS - 1) // BITS
    padded_n = n_words * BITS

    if padded_n > n:
        # Pad columns to a multiple of 64 with zeros (no extra suppressions)
        pad = torch.zeros(n, padded_n - n, dtype=torch.bool, device=device)
        sup = torch.cat([sup, pad], dim=1)  # (n, padded_n)

    # sup_int: (n, n_words, 64)  — 0 or 1
    sup_int = sup.view(n, n_words, BITS).to(torch.int64)

    # powers[k] = 2^k  (int64; bit 63 goes negative but bit-pattern is correct)
    powers = 1 << torch.arange(BITS, dtype=torch.int64, device=device)  # (64,)

    # Dot-product along the 64-bit axis packs bools into one int64 per word.
    # No overflow: each bit position is touched by exactly one element.
    sup_packed = (sup_int * powers).sum(dim=2)  # (n, n_words) int64

    # Single GPU→CPU transfer — 32× smaller than transferring the float IoU matrix
    # Reinterpret as uint64 so Python bit-shifts behave correctly (no sign issues)
    sup_np = sup_packed.cpu().numpy().view(np.uint64)  # (n, n_words) uint64

    # --- CPU phase: sequential greedy with bitmask OR (mirrors gather_keep_from_mask) ---

    removed = np.zeros(n_words, dtype=np.uint64)
    keep = np.zeros(n, dtype=np.bool_)
    _u1 = np.uint64(1)

    for i in range(n):
        word_i = i >> 6        # i // 64
        bit_i  = np.uint64(i & 63)   # i % 64
        if not (removed[word_i] & (_u1 << bit_i)):
            keep[i] = True
            removed |= sup_np[i]   # O(N/64) uint64 OR ops per kept box

    # Map sorted-order indices back to original indices and return on original device
    kept_sorted = np.where(keep)[0]
    kept_tensor = torch.from_numpy(kept_sorted).to(device=device, dtype=torch.int64)
    return order[kept_tensor]
