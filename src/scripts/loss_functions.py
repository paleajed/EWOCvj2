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

    # Default style layer weights - optimized for BRUSH STROKES
    # relu3_1 and relu4_1 capture actual brush stroke patterns
    # Uses 0.1-10 range (100:1 ratio) for noticeable layer differences
    DEFAULT_STYLE_WEIGHTS = {
        'relu1_1': 0.5,   # Fine edges (low)
        'relu2_1': 2.0,   # Small textures (medium-high)
        'relu3_1': 10.0,  # Medium strokes (KEY - MAX)
        'relu4_1': 10.0,  # Brush strokes (KEY - MAX)
        'relu5_1': 0.5,   # Abstract mood (low)
    }

    def __init__(self, style_weights=None):
        """
        Args:
            style_weights: Optional dict with layer weights, e.g.:
                {'relu1_1': 0.1, 'relu2_1': 0.5, 'relu3_1': 2.0, 'relu4_1': 3.0, 'relu5_1': 0.5}
        """
        super(PerceptualLoss, self).__init__()
        self.vgg = VGG19FeatureExtractor()

        # Use provided weights or defaults
        if style_weights:
            self.style_weights = {**self.DEFAULT_STYLE_WEIGHTS, **style_weights}
        else:
            self.style_weights = self.DEFAULT_STYLE_WEIGHTS.copy()

    def get_style_weights(self):
        """Get current style layer weights"""
        return self.style_weights.copy()

    def set_style_weights(self, weights):
        """Set style layer weights (partial update supported)"""
        self.style_weights.update(weights)

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

        Handles batch size mismatch: output can have batch > 1, target is typically batch 1.
        Each output image is compared against the same style target.
        """
        if layers is None:
            layers = ['relu1_1', 'relu2_1', 'relu3_1', 'relu4_1', 'relu5_1']

        output_features = self.vgg(output)
        target_features = self.vgg(target)

        loss = 0.0
        total_weight = 0.0
        batch_size = output.size(0)

        for layer in layers:
            weight = self.style_weights.get(layer, 1.0)
            total_weight += weight

            output_gram = self.gram_matrix(output_features[layer])  # [B, C, C]
            target_gram = self.gram_matrix(target_features[layer])  # [1, C, C]

            # Expand target to match output batch size for proper comparison
            if target_gram.size(0) == 1 and batch_size > 1:
                target_gram = target_gram.expand(batch_size, -1, -1)

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
    Compute optical flow between two frames using Farneback algorithm.

    Uses full resolution and quality parameters for accurate flow estimation.
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

        # Full quality Farneback parameters
        flow = cv2.calcOpticalFlowFarneback(
            gray1, gray2,
            None,
            pyr_scale=0.5,
            levels=3,
            winsize=15,
            iterations=3,
            poly_n=5,
            poly_sigma=1.1,
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


def warp_features(features, flow):
    """
    Warp feature maps using optical flow (for feature-level temporal loss)

    The flow is at full resolution, but features are at 1/4 resolution,
    so we need to resize the flow accordingly.

    Args:
        features: Feature tensor [B, C, H/4, W/4]
        flow: Optical flow at full resolution [B, 2, H, W]

    Returns:
        Warped features [B, C, H/4, W/4]
    """
    B, C, Hf, Wf = features.shape
    _, _, H, W = flow.shape

    # Resize flow to feature resolution
    flow_resized = F.interpolate(flow, size=(Hf, Wf), mode='bilinear', align_corners=True)

    # Scale flow values (flow is in pixels at original resolution)
    flow_resized[:, 0, :, :] = flow_resized[:, 0, :, :] * (Wf / W)
    flow_resized[:, 1, :, :] = flow_resized[:, 1, :, :] * (Hf / H)

    # Warp using the resized flow
    return warp_image(features, flow_resized)


def compute_occlusion_mask(flow_forward, flow_backward, threshold=1.0):
    """
    Compute occlusion mask using forward-backward consistency check

    Args:
        flow_forward: Forward optical flow [B, 2, H, W]
        flow_backward: Backward optical flow [B, 2, H, W]
        threshold: Consistency threshold in pixels

    Returns:
        Occlusion mask [B, 1, H, W] (1 = visible, 0 = occluded)
    """
    # Warp backward flow using forward flow
    warped_backward = warp_image(flow_backward, flow_forward)

    # Check consistency: flow_forward + warped_backward should be ~0
    consistency = flow_forward + warped_backward
    consistency_magnitude = torch.sqrt(
        consistency[:, 0:1] ** 2 + consistency[:, 1:2] ** 2
    )

    # Create mask (1 where consistent, 0 where occluded)
    mask = (consistency_magnitude < threshold).float()

    return mask


def rgb_to_luminance(rgb):
    """
    Convert RGB to luminance using ITU-R BT.709 coefficients

    This is used in the original ReCoNet for the luminance warping constraint.

    Args:
        rgb: RGB tensor [B, 3, H, W]

    Returns:
        Luminance tensor [B, 1, H, W]
    """
    # BT.709 coefficients
    r = rgb[:, 0:1, :, :]
    g = rgb[:, 1:2, :, :]
    b = rgb[:, 2:3, :, :]

    luminance = 0.2126 * r + 0.7152 * g + 0.0722 * b
    return luminance


def output_temporal_loss(styled_t, styled_t1, input_t, input_t1, flow, mask=None):
    """
    Output-level temporal loss with motion-aware weighting (RGB)

    The idea: stylized frames should be temporally consistent WHERE the input
    is static, but allowed to change WHERE the input has motion.

    Note: Feature temporal loss is disabled (lambda_f_ratio=0) because it
    constrains encoder features and prevents input color adaptation.
    This output-level RGB loss only constrains final output, not the encoder.

    Args:
        styled_t: Stylized frame at time t [B, 3, H, W]
        styled_t1: Stylized frame at time t+1 [B, 3, H, W]
        input_t: Input frame at time t [B, 3, H, W]
        input_t1: Input frame at time t+1 [B, 3, H, W]
        flow: Optical flow from t to t+1 [B, 2, H, W]
        mask: Optional occlusion mask [B, 1, H, W] (1=visible, 0=occluded)

    Returns:
        Output temporal loss scalar
    """
    # Warp previous styled frame to current time
    warped_styled = warp_image(styled_t, flow)

    # Compute temporal diff in RGB for full color+brightness stability
    styled_diff = styled_t1 - warped_styled  # [B, 3, H, W]

    # Compute input motion magnitude to find static regions
    # Where input has motion, we allow styled changes
    warped_input = warp_image(input_t, flow)
    input_diff = input_t1 - warped_input
    input_motion = torch.abs(input_diff).mean(dim=1, keepdim=True)  # [B, 1, H, W]

    # Create soft weight: high where input is static, low where input has motion
    # This allows styled frames to change where there's real motion
    motion_threshold = 0.05  # Threshold for "static" (in [-1,1] range)
    static_weight = torch.exp(-input_motion / motion_threshold)  # Soft falloff

    # Apply static weight - only penalize styled changes in static regions
    weighted_diff = styled_diff * static_weight

    if mask is not None:
        # Apply occlusion mask (only penalize visible regions)
        weighted_diff = weighted_diff * mask

    # L2 loss normalized by image size
    B, C, H, W = weighted_diff.shape
    loss = torch.sum(weighted_diff ** 2) / (B * C * H * W)

    return loss


def feature_temporal_loss(features_t, features_t1, flow, mask=None):
    """
    Feature-level temporal loss

    Original ReCoNet uses temporal loss at both output level AND feature level.
    This enforces temporal consistency in the learned representation (at 1/4 resolution),
    helping the encoder learn temporally stable features.

    Formula:
        L_f = ||F2 - warp(F1, flow)||^2

    Args:
        features_t: Features at time t [B, C, H/4, W/4]
        features_t1: Features at time t+1 [B, C, H/4, W/4]
        flow: Optical flow at full resolution [B, 2, H, W]
        mask: Optional occlusion mask at full resolution [B, 1, H, W]

    Returns:
        Feature temporal loss scalar
    """
    # Warp features from t to t+1
    warped_features = warp_features(features_t, flow)

    # Compute difference
    diff = features_t1 - warped_features

    if mask is not None:
        # Resize mask to feature resolution
        B, C, Hf, Wf = features_t.shape
        mask_resized = F.interpolate(mask, size=(Hf, Wf), mode='bilinear', align_corners=True)
        diff = diff * mask_resized

    # L2 loss normalized by feature dimensions
    B, C, H, W = diff.shape
    loss = torch.sum(diff ** 2) / (B * C * H * W)

    return loss


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
    Complete temporal loss module for ReCoNet-style training

    Combines output-level and feature-level temporal losses
    with occlusion mask computation.

    Original ReCoNet weights:
        LAMBDA_O = 1e6 (output temporal)
        LAMBDA_F = 1e4 (feature temporal)
    """

    def __init__(self, lambda_o=1.0, lambda_f=0.01, use_occlusion_mask=True):
        """
        Args:
            lambda_o: Weight for output-level temporal loss (relative)
            lambda_f: Weight for feature-level temporal loss (relative)
            use_occlusion_mask: Whether to compute and use occlusion masks
        """
        super(TemporalLoss, self).__init__()
        self.lambda_o = lambda_o
        self.lambda_f = lambda_f
        self.use_occlusion_mask = use_occlusion_mask

    def forward(self, styled_t, styled_t1, features_t, features_t1,
                input_t, input_t1, flow=None):
        """
        Compute combined temporal loss

        Args:
            styled_t: Stylized output at time t [B, 3, H, W]
            styled_t1: Stylized output at time t+1 [B, 3, H, W]
            features_t: Encoder features at time t [B, C, H/4, W/4]
            features_t1: Encoder features at time t+1 [B, C, H/4, W/4]
            input_t: Input frame at time t [B, 3, H, W]
            input_t1: Input frame at time t+1 [B, 3, H, W]
            flow: Optional precomputed flow [B, 2, H, W]

        Returns:
            Dictionary with 'output', 'feature', and 'total' losses
        """
        device = styled_t.device

        # Compute optical flow if not provided
        if flow is None:
            flow = compute_optical_flow(input_t, input_t1)

        # Compute occlusion mask
        mask = None
        if self.use_occlusion_mask and HAS_OPENCV:
            flow_backward = compute_optical_flow(input_t1, input_t)
            mask = compute_occlusion_mask(flow, flow_backward)

        # Output-level temporal loss (with luminance constraint)
        o_loss = output_temporal_loss(styled_t, styled_t1, input_t, input_t1, flow, mask)

        # Feature-level temporal loss
        f_loss = feature_temporal_loss(features_t, features_t1, flow, mask)

        # Combined loss
        total = self.lambda_o * o_loss + self.lambda_f * f_loss

        return {
            'output': o_loss,
            'feature': f_loss,
            'total': total
        }


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
