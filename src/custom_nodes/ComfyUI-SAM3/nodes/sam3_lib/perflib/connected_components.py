# Copyright (c) Meta Platforms, Inc. and affiliates. All Rights Reserved
import logging

import torch
import torch.nn.functional as F

try:
    from cc_torch import get_connected_components
    HAS_CC_TORCH = True
except ImportError:
    HAS_CC_TORCH = False


# ---------------------------------------------------------------------------
# GPU renumbering — pure PyTorch, stays on device
# ---------------------------------------------------------------------------

def _renumber_gpu(labels: torch.Tensor, fg: torch.Tensor):
    """
    Convert raw min-index labels to dense 1..K labels and compute per-pixel
    component sizes, entirely on the GPU.

    labels : (B, H, W) int32 — each fg pixel holds the minimum 1-based pixel
             index of its component (batch-local).  0 = background.
    fg     : (B, H, W) bool

    Returns (labels_dense, counts) both (B, H, W) int32.
    """
    B, H, W = labels.shape
    device  = labels.device
    N       = H * W

    labels_dense = torch.zeros_like(labels)
    counts       = torch.zeros_like(labels)

    for b in range(B):   # B is 1 for video; this loop is negligible
        flat_lab = labels[b].view(-1)   # (N,) int32
        flat_fg  = fg[b].view(-1)       # (N,) bool

        fg_labels = flat_lab[flat_fg]   # (num_fg,)
        if fg_labels.numel() == 0:
            continue

        _, inverse = torch.unique(fg_labels, return_inverse=True)
        # inverse[i] ∈ [0, K-1]: dense component index for this fg pixel

        dense_fg = (inverse + 1).to(torch.int32)   # 1-based

        dense_full = torch.zeros(N, dtype=torch.int32, device=device)
        dense_full[flat_fg] = dense_fg
        labels_dense[b] = dense_full.view(H, W)

        n_comp = int(inverse.max().item()) + 1
        sizes  = torch.bincount(inverse, minlength=n_comp)   # (K,) long
        cnt_fg = sizes[inverse].to(torch.int32)

        cnt_full = torch.zeros(N, dtype=torch.int32, device=device)
        cnt_full[flat_fg] = cnt_fg
        counts[b] = cnt_full.view(H, W)

    return labels_dense, counts


# ---------------------------------------------------------------------------
# Pure-PyTorch GPU connected components
# ---------------------------------------------------------------------------

def _connected_components_gpu(fg: torch.Tensor):
    """
    GPU connected components using min-label propagation + pointer jumping.
    No custom CUDA extensions or Triton needed — works with any PyTorch.

    Algorithm (mirrors the Union-Find / pointer-jumping used in cc_torch):

      Phase 1 — seed: 4 rounds of 4-connected min-label propagation.
        Each fg pixel's label = min label in its 4-hop spatial neighbourhood.
        This establishes short-range pointers so Phase 2 has non-trivial
        structure to exploit.

      Phase 2 — pointer jumping:
        label[i] = label[label[i] - 1]
        Each jump follows the pointer one level, but because every node has
        already followed its own pointer, the effective path doubles each
        round → O(log diameter) rounds to converge.
        For a 1920×1080 mask the diameter is at most ~3000 px; log2(750) < 10,
        so fewer than 10 jumps suffice in practice.

    fg : (B, H, W) bool on CUDA
    Returns (labels, counts) both (B, H, W) int32.
    """
    B, H, W = fg.shape
    N       = H * W
    device  = fg.device
    zero32  = torch.zeros(1, dtype=torch.int32, device=device)

    # ---- initialise: each fg pixel = its own 1-based linear index ----------
    idx   = torch.arange(1, N + 1, device=device, dtype=torch.int32).view(1, H, W)
    label = torch.where(fg, idx.expand(B, -1, -1), zero32).clone()

    # ---- Phase 1: 4 rounds of 4-neighbour min propagation ------------------
    # Uses F.pad so boundary pixels see "∞" rather than wrap-around values.
    INF = float(N + 1)
    fg_f = fg.float()

    for _ in range(4):
        L   = label.float()
        Lp  = F.pad(L,    [1, 1, 1, 1], value=INF)
        fgp = F.pad(fg_f, [1, 1, 1, 1], value=0.0)

        # Effective neighbour label: neighbour's label if fg, else own label
        # (substituting own label is a no-op for torch.minimum)
        up    = torch.where(fgp[:,  :-2, 1:-1] > 0, Lp[:,  :-2, 1:-1], L)
        dn    = torch.where(fgp[:, 2:  , 1:-1] > 0, Lp[:, 2:  , 1:-1], L)
        left  = torch.where(fgp[:, 1:-1,  :-2] > 0, Lp[:, 1:-1,  :-2], L)
        right = torch.where(fgp[:, 1:-1, 2:  ] > 0, Lp[:, 1:-1, 2:  ], L)

        min_lab = torch.minimum(L, torch.minimum(up,
                  torch.minimum(dn, torch.minimum(left, right))))
        label = torch.where(fg, min_lab.to(torch.int32), zero32)

    # ---- Phase 2: pointer jumping until convergence ------------------------
    # label[i] = label[ label[i] - 1 ]   (batch-local, 1-based → 0-based index)
    flat_fg    = fg.view(B, -1)       # (B, N) bool
    flat_label = label.view(B, -1)    # (B, N) int32

    for _ in range(30):               # log2(4096²) < 24; 30 is ample
        prev       = flat_label.clone()
        ptr_idx    = (flat_label - 1).clamp(0, N - 1).long()
        flat_label = torch.where(flat_fg, flat_label.gather(1, ptr_idx), zero32)
        if torch.equal(flat_label, prev):
            break

    return _renumber_gpu(flat_label.view(B, H, W), fg)


# ---------------------------------------------------------------------------
# CPU fallback (original implementation, unchanged)
# ---------------------------------------------------------------------------

def connected_components_cpu_single(values: torch.Tensor):
    assert values.dim() == 2
    from skimage.measure import label

    labels, num = label(values.cpu().numpy(), return_num=True)
    labels = torch.from_numpy(labels)
    counts = torch.zeros_like(labels)
    for i in range(1, num + 1):
        cur_mask  = labels == i
        cur_count = cur_mask.sum()
        counts[cur_mask] = cur_count
    return labels, counts


def connected_components_cpu(input_tensor: torch.Tensor):
    out_shape = input_tensor.shape
    if input_tensor.dim() == 4 and input_tensor.shape[1] == 1:
        input_tensor = input_tensor.squeeze(1)
    else:
        assert (
            input_tensor.dim() == 3
        ), "Input tensor must be (B, H, W) or (B, 1, H, W)."

    batch_size = input_tensor.shape[0]
    if batch_size == 0:
        return torch.zeros_like(input_tensor), torch.zeros_like(input_tensor)
    labels_list = []
    counts_list = []
    for b in range(batch_size):
        labels, counts = connected_components_cpu_single(input_tensor[b])
        labels_list.append(labels)
        counts_list.append(counts)
    labels_tensor = torch.stack(labels_list, dim=0).to(input_tensor.device)
    counts_tensor = torch.stack(counts_list, dim=0).to(input_tensor.device)
    return labels_tensor.view(out_shape), counts_tensor.view(out_shape)


# ---------------------------------------------------------------------------
# Public entry point
# ---------------------------------------------------------------------------

def connected_components(input_tensor: torch.Tensor):
    """
    Computes connected components labeling on a batch of 2D tensors.

    Priority:
      1. cc_torch  — compiled CUDA extension (fastest, if installed)
      2. Pure PyTorch GPU  — pointer-jumping, O(log diameter) passes on GPU
      3. CPU       — skimage, always available

    Args:
        input_tensor: BxHxW or Bx1xHxW integer/bool tensor.
                      Non-zero values are foreground.

    Returns:
        (labels, counts) — same shape as input_tensor.
        labels: dense component indices (1-based); 0 = background.
        counts: number of pixels in each pixel's component; 0 = background.
    """
    if input_tensor.dim() == 3:
        input_tensor = input_tensor.unsqueeze(1)

    assert (
        input_tensor.dim() == 4 and input_tensor.shape[1] == 1
    ), "Input tensor must be (B, H, W) or (B, 1, H, W)."

    if input_tensor.is_cuda:
        if HAS_CC_TORCH:
            return get_connected_components(input_tensor.to(torch.uint8))

        try:
            fg = input_tensor.squeeze(1).bool()   # (B, H, W)
            labels, counts = _connected_components_gpu(fg)
            out_shape = input_tensor.shape
            return labels.view(out_shape), counts.view(out_shape)
        except Exception as e:
            logging.warning(f"[cc] GPU connected-components failed ({e}), "
                            "falling back to CPU skimage")

    return connected_components_cpu(input_tensor)
