"""
loss_functions.py.bak

Perceptual loss functions for style transfer training
- VGG19 perceptual loss (content + style)
- Total variation loss
- Simple temporal consistency loss

PRE-RESEARCH VERSION - before luminance constraint and feature temporal loss

License: GPL3
"""

import torch
import torch.nn as nn
import torch.nn.functional as F
from torchvision import models
import numpy as np

# Optional: OpenCV for optical flow (fallback to torch if not available)
try:
    import cv2
    HAS_OPENCV = True
except ImportError:
    HAS_OPENCV = False
    print("[WARNING] OpenCV not available, temporal loss will use simple frame difference")


class VGG19FeatureExtractor(nn.Module):
    """
    VGG19 Feature Extractor for Perceptual Loss

    Uses VGG19 (deeper than VGG16) for better style capture.
    Extracts features from layers used in Gatys et al. and Johnson et al.:
    - relu1_1: Very low-level (edges)
    - relu2_1: Low-level (colors, simple textures)
    - relu3_1: Mid-level (complex textures)
    - relu4_1: High-level (style patterns)
    - relu5_1: Very high-level (abstract style) - CRITICAL for style
    """

    def __init__(self):
        super(VGG19FeatureExtractor, self).__init__()

        # Load pretrained VGG19 (deeper network, better for style)
        vgg19 = models.vgg19(weights=models.VGG19_Weights.IMAGENET1K_V1)
        vgg_features = vgg19.features

        # Extract layers (using _1 layers as in original Gatys paper)
        self.slice1 = nn.Sequential(*list(vgg_features[:2]))   # relu1_1 (index 1)
        self.slice2 = nn.Sequential(*list(vgg_features[2:7]))  # relu2_1 (index 6)
        self.slice3 = nn.Sequential(*list(vgg_features[7:12])) # relu3_1 (index 11)
        self.slice4 = nn.Sequential(*list(vgg_features[12:21]))# relu4_1 (index 20)
        self.slice5 = nn.Sequential(*list(vgg_features[21:30]))# relu5_1 (index 29)

        # Freeze parameters (no training)
        for param in self.parameters():
            param.requires_grad = False

        # VGG normalization values (ImageNet)
        self.register_buffer('mean', torch.tensor([0.485, 0.456, 0.406]).view(1, 3, 1, 1))
        self.register_buffer('std', torch.tensor([0.229, 0.224, 0.225]).view(1, 3, 1, 1))

    def forward(self, x):
        """
        Extract features from input image

        Args:
            x: Input tensor [B, 3, H, W], range [-1, 1]

        Returns:
            Dictionary of features from different layers
        """
        # Convert from [-1, 1] to [0, 1]
        x = (x + 1.0) / 2.0

        # Normalize using ImageNet stats
        x = (x - self.mean) / self.std

        # Extract features at each layer
        relu1_1 = self.slice1(x)
        relu2_1 = self.slice2(relu1_1)
        relu3_1 = self.slice3(relu2_1)
        relu4_1 = self.slice4(relu3_1)
        relu5_1 = self.slice5(relu4_1)

        return {
            'relu1_1': relu1_1,
            'relu2_1': relu2_1,
            'relu3_1': relu3_1,
            'relu4_1': relu4_1,
            'relu5_1': relu5_1
        }


# Backward compatibility alias
VGG16FeatureExtractor = VGG19FeatureExtractor


class PerceptualLoss(nn.Module):
    """
    Perceptual Loss combining content and style losses

    Based on Gatys et al. "A Neural Algorithm of Artistic Style" and
    Johnson et al. "Perceptual Losses for Real-Time Style Transfer"

    Content Loss: MSE between relu4_1 features (high-level, preserves structure)
    Style Loss: Weighted Gram matrix loss across multiple layers
    """

    # Style layer weights - balanced (original working values from 40k run)
    STYLE_WEIGHTS = {
        'relu1_1': 0.2,   # Fine texture
        'relu2_1': 0.4,   # Small patterns
        'relu3_1': 0.8,   # Medium patterns
        'relu4_1': 1.6,   # Brush strokes
        'relu5_1': 3.2,   # Abstract style
    }

    def __init__(self):
        super(PerceptualLoss, self).__init__()
        self.vgg = VGG19FeatureExtractor()

    def gram_matrix(self, features):
        """
        Compute Gram matrix for style loss
        """
        B, C, H, W = features.shape
        features = features.view(B, C, H * W)
        gram = torch.bmm(features, features.transpose(1, 2))
        # Normalize by number of elements (standard Gatys normalization)
        return gram / (C * H * W)

    def content_loss(self, output, target, layer='relu4_1'):
        """
        Content loss: MSE between high-level features
        """
        output_features = self.vgg(output)
        target_features = self.vgg(target)

        loss = F.mse_loss(output_features[layer], target_features[layer])
        return loss

    def style_loss(self, output, target, layers=None, debug=False):
        """
        Style loss: Weighted Gram matrix loss across multiple layers
        """
        if layers is None:
            layers = ['relu1_1', 'relu2_1', 'relu3_1', 'relu4_1', 'relu5_1']

        output_features = self.vgg(output)
        target_features = self.vgg(target)

        loss = 0.0
        total_weight = 0.0

        for layer in layers:
            weight = self.STYLE_WEIGHTS.get(layer, 1.0)
            total_weight += weight

            output_gram = self.gram_matrix(output_features[layer])
            target_gram = self.gram_matrix(target_features[layer])
            layer_loss = F.mse_loss(output_gram, target_gram)

            loss += weight * layer_loss

            if debug:
                print(f"  {layer} (w={weight}): out_gram={output_gram.mean().item():.2e}, "
                      f"tgt_gram={target_gram.mean().item():.2e}, "
                      f"loss={layer_loss.item():.2e}, weighted={weight * layer_loss.item():.2e}")

        return loss / total_weight


def total_variation_loss(image):
    """
    Total Variation Loss for smoothness
    """
    diff_h = torch.abs(image[:, :, 1:, :] - image[:, :, :-1, :])
    diff_w = torch.abs(image[:, :, :, 1:] - image[:, :, :, :-1])

    loss = torch.mean(diff_h) + torch.mean(diff_w)
    return loss


def compute_optical_flow(frame1, frame2):
    """
    Compute optical flow between two frames using Farneback algorithm
    """
    if not HAS_OPENCV:
        B, C, H, W = frame1.shape
        return torch.zeros(B, 2, H, W, device=frame1.device)

    device = frame1.device
    B, C, H, W = frame1.shape
    flows = []

    for b in range(B):
        f1 = ((frame1[b].permute(1, 2, 0).cpu().numpy() + 1.0) * 127.5).astype(np.uint8)
        f2 = ((frame2[b].permute(1, 2, 0).cpu().numpy() + 1.0) * 127.5).astype(np.uint8)

        gray1 = cv2.cvtColor(f1, cv2.COLOR_RGB2GRAY)
        gray2 = cv2.cvtColor(f2, cv2.COLOR_RGB2GRAY)

        flow = cv2.calcOpticalFlowFarneback(
            gray1, gray2,
            None,
            pyr_scale=0.5,
            levels=3,
            winsize=15,
            iterations=3,
            poly_n=5,
            poly_sigma=1.2,
            flags=0
        )

        flow_tensor = torch.from_numpy(flow).permute(2, 0, 1).float()
        flows.append(flow_tensor)

    flow_batch = torch.stack(flows, dim=0).to(device)
    return flow_batch


def warp_image(image, flow):
    """
    Warp an image using optical flow (backward warping)
    """
    B, C, H, W = image.shape
    device = image.device

    grid_y, grid_x = torch.meshgrid(
        torch.linspace(-1, 1, H, device=device),
        torch.linspace(-1, 1, W, device=device),
        indexing='ij'
    )
    grid = torch.stack([grid_x, grid_y], dim=-1)
    grid = grid.unsqueeze(0).expand(B, -1, -1, -1)

    flow_normalized = flow.permute(0, 2, 3, 1)
    flow_normalized = flow_normalized.clone()
    flow_normalized[..., 0] = flow_normalized[..., 0] / (W / 2)
    flow_normalized[..., 1] = flow_normalized[..., 1] / (H / 2)

    sample_grid = grid + flow_normalized

    warped = F.grid_sample(image, sample_grid, mode='bilinear',
                           padding_mode='border', align_corners=True)

    return warped


def temporal_consistency_loss(output_t, output_t1, flow_t_to_t1, mask=None, content_frame=None):
    """
    Simple temporal consistency loss for video style transfer.

    Penalizes temporal flickering by encouraging stylized frames
    to follow the same motion as the input frames.

    Args:
        output_t: Stylized frame at time t [B, C, H, W]
        output_t1: Stylized frame at time t+1 [B, C, H, W]
        flow_t_to_t1: Optical flow from t to t+1 [B, 2, H, W]
        mask: Optional occlusion mask [B, 1, H, W]
        content_frame: Unused, kept for API compatibility

    Returns:
        Temporal consistency loss scalar
    """
    # Warp output_t to time t+1 using optical flow
    warped_output = warp_image(output_t, flow_t_to_t1)

    # Compute difference
    diff = output_t1 - warped_output

    if mask is not None:
        diff = diff * mask

    # L2 loss
    loss = torch.mean(diff ** 2)

    return loss


class TemporalLoss(nn.Module):
    """
    Simple temporal loss module (pre-research version)
    """

    def __init__(self, use_occlusion_mask=False):
        super(TemporalLoss, self).__init__()
        self.use_occlusion_mask = use_occlusion_mask

    def forward(self, content_frames, stylized_frames, precomputed_flows=None):
        n_frames = len(content_frames)
        device = content_frames[0].device

        if n_frames < 2:
            return {'short_term': torch.tensor(0.0, device=device)}

        if precomputed_flows is None:
            flows = []
            for i in range(n_frames - 1):
                flow = compute_optical_flow(content_frames[i], content_frames[i + 1])
                flows.append(flow)
        else:
            flows = precomputed_flows

        total_loss = 0.0
        for i in range(n_frames - 1):
            loss = temporal_consistency_loss(stylized_frames[i], stylized_frames[i + 1], flows[i])
            total_loss += loss

        return {'short_term': total_loss / (n_frames - 1)}


if __name__ == "__main__":
    print("Testing loss functions...")

    perceptual_loss = PerceptualLoss()

    output = torch.randn(2, 3, 256, 256)
    target = torch.randn(2, 3, 256, 256)

    c_loss = perceptual_loss.content_loss(output, target)
    print(f"Content loss: {c_loss.item():.4f}")

    s_loss = perceptual_loss.style_loss(output, target)
    print(f"Style loss: {s_loss.item():.4f}")

    tv_loss = total_variation_loss(output)
    print(f"TV loss: {tv_loss.item():.4f}")

    print("\nLoss function tests passed!")
