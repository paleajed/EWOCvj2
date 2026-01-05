"""
loss_functions.py

Perceptual loss functions for ReCoNet style transfer training
Based on the original ReCoNet paper: https://arxiv.org/abs/1807.01197

Loss components:
- VGG16 perceptual loss (content + style via Gram matrices)
  Using original ReCoNet layers: relu1_2, relu2_2, relu3_3, relu4_3
- Total variation loss (smoothness)
- Output-level temporal loss with luminance warping constraint
- Feature-level temporal loss (at encoder output)
- Occlusion mask support for temporal losses

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


class VGG16FeatureExtractor(nn.Module):
    """
    VGG16 Feature Extractor for Perceptual Loss (matching original ReCoNet)

    Uses VGG16 as in the original ReCoNet paper.
    Extracts features from 4 layers for style loss:
    - relu1_2: Low-level features
    - relu2_2: Low-mid level
    - relu3_3: Mid level
    - relu4_3: High level (also used for content)

    VGG16 layer indices after each ReLU:
    relu1_1: 1, relu1_2: 3
    relu2_1: 6, relu2_2: 8
    relu3_1: 11, relu3_2: 13, relu3_3: 15
    relu4_1: 18, relu4_2: 20, relu4_3: 22
    relu5_1: 25, relu5_2: 27, relu5_3: 29
    """

    def __init__(self):
        super(VGG16FeatureExtractor, self).__init__()

        # Load pretrained VGG16 (as used in original ReCoNet)
        vgg16 = models.vgg16(weights=models.VGG16_Weights.IMAGENET1K_V1)
        vgg_features = vgg16.features

        # Extract layers matching original ReCoNet
        # Using relu*_2 and relu*_3 layers as in original implementation
        self.slice1 = nn.Sequential(*list(vgg_features[:4]))   # up to relu1_2 (index 3)
        self.slice2 = nn.Sequential(*list(vgg_features[4:9]))  # up to relu2_2 (index 8)
        self.slice3 = nn.Sequential(*list(vgg_features[9:16])) # up to relu3_3 (index 15)
        self.slice4 = nn.Sequential(*list(vgg_features[16:23]))# up to relu4_3 (index 22)

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
        relu1_2 = self.slice1(x)
        relu2_2 = self.slice2(relu1_2)
        relu3_3 = self.slice3(relu2_2)
        relu4_3 = self.slice4(relu3_3)

        return {
            'relu1_2': relu1_2,
            'relu2_2': relu2_2,
            'relu3_3': relu3_3,
            'relu4_3': relu4_3,
        }


# Backward compatibility alias
VGG19FeatureExtractor = VGG16FeatureExtractor


class PerceptualLoss(nn.Module):
    """
    Perceptual Loss combining content and style losses

    Based on original ReCoNet implementation and Gatys et al. paper.

    Content Loss: MSE between relu2_2 features (layer 2, as in original ReCoNet)
    Style Loss: Weighted Gram matrix loss across 4 VGG16 layers

    Original ReCoNet uses VGG16 with layers: relu1_2, relu2_2, relu3_3, relu4_3
    """

    # Style layer weights - matching original ReCoNet STYLE_WEIGHT array
    # Original code: STYLE_WEIGHT = [1e-1, 1e0, 1e1, 5e0] for 4 layers
    STYLE_WEIGHTS = {
        'relu1_2': 1e-1,   # 0.1 - Fine texture
        'relu2_2': 1e0,    # 1.0 - Low-mid patterns
        'relu3_3': 1e1,    # 10.0 - Mid-level patterns
        'relu4_3': 5e0,    # 5.0 - High-level style
    }

    def __init__(self):
        super(PerceptualLoss, self).__init__()
        self.vgg = VGG16FeatureExtractor()

    def gram_matrix(self, features):
        """
        Compute Gram matrix for style loss

        Uses normalization from Gatys et al. paper

        Args:
            features: Feature tensor [B, C, H, W]

        Returns:
            Gram matrix [B, C, C]
        """
        B, C, H, W = features.shape
        features = features.view(B, C, H * W)
        gram = torch.bmm(features, features.transpose(1, 2))
        # Normalize by number of elements (standard Gatys normalization)
        return gram / (C * H * W)

    def content_loss(self, output, target, layer='relu2_2'):
        """
        Content loss: MSE between mid-level features

        Uses relu2_2 by default (layer 2, as in original ReCoNet).
        This layer captures mid-level structure while allowing stylistic freedom.

        Args:
            output: Generated image
            target: Content target image
            layer: Which VGG layer to use

        Returns:
            Content loss scalar
        """
        output_features = self.vgg(output)
        target_features = self.vgg(target)

        loss = F.mse_loss(output_features[layer], target_features[layer])
        return loss

    def style_loss(self, output, target, layers=None, debug=False):
        """
        Style loss: Weighted Gram matrix loss across multiple layers

        Uses all 4 VGG16 layers with weights matching original ReCoNet
        to capture style at multiple scales (from textures to abstract patterns)

        Args:
            output: Generated image
            target: Style target image
            layers: Which VGG layers to use (default: all 4)
            debug: Print debug info

        Returns:
            Style loss scalar
        """
        if layers is None:
            layers = ['relu1_2', 'relu2_2', 'relu3_3', 'relu4_3']

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

    Encourages spatial smoothness by penalizing differences
    between adjacent pixels

    Args:
        image: Image tensor [B, C, H, W]

    Returns:
        TV loss scalar
    """
    diff_h = torch.abs(image[:, :, 1:, :] - image[:, :, :-1, :])
    diff_w = torch.abs(image[:, :, :, 1:] - image[:, :, :, :-1])

    loss = torch.mean(diff_h) + torch.mean(diff_w)
    return loss


def compute_optical_flow(frame1, frame2):
    """
    Compute optical flow between two frames using Farneback algorithm

    Args:
        frame1: First frame tensor [B, 3, H, W] in [-1, 1] range
        frame2: Second frame tensor [B, 3, H, W] in [-1, 1] range

    Returns:
        flow: Optical flow tensor [B, 2, H, W] (dx, dy per pixel)
    """
    if not HAS_OPENCV:
        # Fallback: return zero flow (will use simple frame difference)
        B, C, H, W = frame1.shape
        return torch.zeros(B, 2, H, W, device=frame1.device)

    device = frame1.device
    B, C, H, W = frame1.shape
    flows = []

    for b in range(B):
        # Convert to numpy grayscale for OpenCV
        # [-1, 1] -> [0, 255] uint8
        f1 = ((frame1[b].permute(1, 2, 0).cpu().numpy() + 1.0) * 127.5).astype(np.uint8)
        f2 = ((frame2[b].permute(1, 2, 0).cpu().numpy() + 1.0) * 127.5).astype(np.uint8)

        # Convert to grayscale
        gray1 = cv2.cvtColor(f1, cv2.COLOR_RGB2GRAY)
        gray2 = cv2.cvtColor(f2, cv2.COLOR_RGB2GRAY)

        # Compute optical flow (Farneback)
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

        # Convert to tensor [2, H, W]
        flow_tensor = torch.from_numpy(flow).permute(2, 0, 1).float()
        flows.append(flow_tensor)

    # Stack batch
    flow_batch = torch.stack(flows, dim=0).to(device)
    return flow_batch


def warp_image(image, flow):
    """
    Warp an image using optical flow (backward warping)

    Args:
        image: Image tensor [B, C, H, W]
        flow: Optical flow [B, 2, H, W] (dx, dy)

    Returns:
        Warped image [B, C, H, W]
    """
    B, C, H, W = image.shape
    device = image.device

    # Create normalized grid coordinates
    grid_y, grid_x = torch.meshgrid(
        torch.linspace(-1, 1, H, device=device),
        torch.linspace(-1, 1, W, device=device),
        indexing='ij'
    )
    grid = torch.stack([grid_x, grid_y], dim=-1)  # [H, W, 2]
    grid = grid.unsqueeze(0).expand(B, -1, -1, -1)  # [B, H, W, 2]

    # Normalize flow to [-1, 1] range
    # flow is in pixels, convert to normalized coordinates
    flow_normalized = flow.permute(0, 2, 3, 1)  # [B, H, W, 2]
    flow_normalized = flow_normalized.clone()
    flow_normalized[..., 0] = flow_normalized[..., 0] / (W / 2)
    flow_normalized[..., 1] = flow_normalized[..., 1] / (H / 2)

    # Add flow to grid (backward warping)
    sample_grid = grid + flow_normalized

    # Warp image
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
    Output-level temporal loss with luminance warping constraint

    Original ReCoNet formula:
        L_o = ||( Ŝ₂ - warp(Ŝ₁) ) - ( I₂ - warp(I₁) )||²

    This compares the temporal change in stylized output with the temporal
    change in input. The stylized output should change in the same way as
    the input changes (allowing for legitimate motion/lighting changes).

    The input difference is converted to luminance to focus on structural
    motion rather than color changes.

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
    # Warp previous frames to current time
    warped_styled = warp_image(styled_t, flow)
    warped_input = warp_image(input_t, flow)

    # Compute temporal differences
    styled_diff = styled_t1 - warped_styled  # How stylized output changed
    input_diff = input_t1 - warped_input     # How input changed

    # Convert input difference to luminance (original ReCoNet luminance constraint)
    # This focuses on structural motion rather than color changes
    input_diff_lum = rgb_to_luminance(input_diff)

    # Also convert styled diff to luminance for fair comparison
    styled_diff_lum = rgb_to_luminance(styled_diff)

    # The loss: stylized temporal change should match input temporal change
    diff = styled_diff_lum - input_diff_lum

    if mask is not None:
        # Apply occlusion mask (only penalize visible regions)
        diff = diff * mask

    # L2 loss normalized by image size
    B, _, H, W = diff.shape
    loss = torch.sum(diff ** 2) / (B * H * W)

    return loss


def feature_temporal_loss(features_t, features_t1, flow, mask=None):
    """
    Feature-level temporal loss

    Original ReCoNet uses temporal loss at both output level AND feature level.
    This enforces temporal consistency in the learned representation (at 1/4 resolution),
    helping the encoder learn temporally stable features.

    Formula:
        L_f = ||F₂ - warp(F₁, flow)||²

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
    Simple temporal consistency loss (backward compatibility)

    For use when input frames are not available. Uses simple warping loss.

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
        # Apply occlusion mask (only penalize visible regions)
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
    # Test loss functions
    print("Testing loss functions...")

    perceptual_loss = PerceptualLoss()

    # Create dummy images in [-1, 1] range
    output = torch.rand(2, 3, 256, 256) * 2.0 - 1.0  # Batch of 2, 256x256
    target = torch.rand(2, 3, 256, 256) * 2.0 - 1.0

    # Test content loss
    c_loss = perceptual_loss.content_loss(output, target)
    print(f"Content loss: {c_loss.item():.4f}")

    # Test style loss
    s_loss = perceptual_loss.style_loss(output, target)
    print(f"Style loss: {s_loss.item():.4f}")

    # Test TV loss
    tv_loss = total_variation_loss(output)
    print(f"TV loss: {tv_loss.item():.4f}")

    # Test temporal loss
    print("\nTesting temporal loss functions...")
    if HAS_OPENCV:
        # Frames in [-1, 1] range
        frame1 = torch.rand(1, 3, 256, 256) * 2.0 - 1.0
        frame2 = frame1 + torch.randn_like(frame1) * 0.1  # Similar frame with small noise
        frame2 = frame2.clamp(-1, 1)

        flow = compute_optical_flow(frame1, frame2)
        print(f"Optical flow shape: {flow.shape}")
        print(f"Optical flow range: [{flow.min().item():.2f}, {flow.max().item():.2f}]")

        # Test output temporal loss
        styled1 = torch.rand(1, 3, 256, 256) * 2.0 - 1.0
        styled2 = styled1 + torch.randn_like(styled1) * 0.1
        styled2 = styled2.clamp(-1, 1)

        o_t_loss = output_temporal_loss(styled1, styled2, frame1, frame2, flow)
        print(f"Output temporal loss: {o_t_loss.item():.4f}")

        # Test feature temporal loss
        features1 = torch.randn(1, 128, 64, 64)  # 1/4 resolution (features can be any range)
        features2 = features1 + torch.randn_like(features1) * 0.1

        f_t_loss = feature_temporal_loss(features1, features2, flow)
        print(f"Feature temporal loss: {f_t_loss.item():.4f}")

        # Test combined temporal loss
        temporal_loss_fn = TemporalLoss(lambda_o=1.0, lambda_f=0.01)
        t_losses = temporal_loss_fn(styled1, styled2, features1, features2, frame1, frame2)
        print(f"Combined temporal loss: {t_losses}")

    else:
        print("OpenCV not available, skipping optical flow tests")

    print("\nLoss function tests passed!")
