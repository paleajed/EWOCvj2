"""
reconet_model.py

ReCoNet (Real-time Coherent Video Style Transfer Network) Model Architecture
Based on the original paper: https://arxiv.org/abs/1807.01197

Key architectural features matching the original:
- Reflection padding throughout (avoids border artifacts)
- Upsample + Conv decoder (avoids checkerboard artifacts from ConvTranspose2d)
- Returns (features, output) tuple for feature-level temporal loss

License: GPL3
"""

import torch
import torch.nn as nn


class ConvInstRelu(nn.Module):
    """Convolution + InstanceNorm + ReLU with reflection padding"""
    def __init__(self, in_channels, out_channels, kernel_size, stride=1):
        super(ConvInstRelu, self).__init__()
        # Reflection padding to avoid border artifacts
        padding = kernel_size // 2
        self.pad = nn.ReflectionPad2d(padding)
        self.conv = nn.Conv2d(in_channels, out_channels, kernel_size, stride=stride, padding=0)
        self.norm = nn.InstanceNorm2d(out_channels, affine=True)
        self.relu = nn.ReLU(inplace=True)

    def forward(self, x):
        x = self.pad(x)
        x = self.conv(x)
        x = self.norm(x)
        x = self.relu(x)
        return x


class UpsampleConvInstRelu(nn.Module):
    """Upsample + Conv + InstanceNorm + ReLU (avoids checkerboard artifacts)"""
    def __init__(self, in_channels, out_channels, kernel_size, scale_factor=2):
        super(UpsampleConvInstRelu, self).__init__()
        # Upsample then conv (better than ConvTranspose2d)
        self.upsample = nn.Upsample(scale_factor=scale_factor, mode='nearest')
        padding = kernel_size // 2
        self.pad = nn.ReflectionPad2d(padding)
        self.conv = nn.Conv2d(in_channels, out_channels, kernel_size, stride=1, padding=0)
        self.norm = nn.InstanceNorm2d(out_channels, affine=True)
        self.relu = nn.ReLU(inplace=True)

    def forward(self, x):
        x = self.upsample(x)
        x = self.pad(x)
        x = self.conv(x)
        x = self.norm(x)
        x = self.relu(x)
        return x


class ResidualBlock(nn.Module):
    """Residual block with instance normalization and reflection padding"""
    def __init__(self, channels):
        super(ResidualBlock, self).__init__()
        # Reflection padding for both convolutions
        self.pad1 = nn.ReflectionPad2d(1)
        self.conv1 = nn.Conv2d(channels, channels, kernel_size=3, padding=0)
        self.in1 = nn.InstanceNorm2d(channels, affine=True)
        self.pad2 = nn.ReflectionPad2d(1)
        self.conv2 = nn.Conv2d(channels, channels, kernel_size=3, padding=0)
        self.in2 = nn.InstanceNorm2d(channels, affine=True)
        self.relu = nn.ReLU(inplace=True)

    def forward(self, x):
        residual = x
        out = self.pad1(x)
        out = self.conv1(out)
        out = self.in1(out)
        out = self.relu(out)
        out = self.pad2(out)
        out = self.conv2(out)
        out = self.in2(out)
        out = out + residual
        return out


class ReCoNet(nn.Module):
    """
    ReCoNet Style Transfer Network

    Architecture matching original paper:
    - Encoder: 3 conv layers with reflection padding (downsampling 4x)
    - Residual blocks: 5 blocks (feature transformation at 1/4 resolution)
    - Decoder: 2 upsample+conv layers (upsampling back to full resolution)

    Input/Output: RGB images, any resolution (fully convolutional)

    Returns: (features, output) tuple
        - features: Encoder output at 1/4 resolution [B, 128, H/4, W/4]
        - output: Stylized image [B, 3, H, W], range [-1, 1]
    """

    def __init__(self):
        super(ReCoNet, self).__init__()

        # Encoder (Downsampling with reflection padding)
        # conv1: 3->32, kernel 9x9, stride 1 (preserves resolution)
        self.conv1 = ConvInstRelu(3, 32, kernel_size=9, stride=1)
        # conv2: 32->64, kernel 3x3, stride 2 (1/2 resolution)
        self.conv2 = ConvInstRelu(32, 64, kernel_size=3, stride=2)
        # conv3: 64->128, kernel 3x3, stride 2 (1/4 resolution)
        self.conv3 = ConvInstRelu(64, 128, kernel_size=3, stride=2)

        # Residual Blocks (Feature Transformation at 1/4 resolution)
        self.res_blocks = nn.Sequential(
            ResidualBlock(128),
            ResidualBlock(128),
            ResidualBlock(128),
            ResidualBlock(128),
            ResidualBlock(128),
        )

        # Decoder (Upsampling with Upsample+Conv to avoid checkerboard)
        # deconv1: 128->64, upsample 2x (1/2 resolution)
        self.deconv1 = UpsampleConvInstRelu(128, 64, kernel_size=3, scale_factor=2)
        # deconv2: 64->32, upsample 2x (full resolution)
        self.deconv2 = UpsampleConvInstRelu(64, 32, kernel_size=3, scale_factor=2)

        # Final output layer (no normalization, tanh activation for [-1, 1] output)
        self.final_pad = nn.ReflectionPad2d(4)
        self.final_conv = nn.Conv2d(32, 3, kernel_size=9, padding=0)
        self.tanh = nn.Tanh()

    def forward(self, x, return_features=True):
        """
        Forward pass

        Args:
            x: Input image tensor [B, 3, H, W], range [-1, 1]
            return_features: If True, return (features, output). If False, return output only.

        Returns:
            If return_features=True: (features, output) tuple
                - features: [B, 128, H/4, W/4] for feature-level temporal loss
                - output: [B, 3, H, W] stylized image, range [-1, 1]
            If return_features=False: output only (for inference/ONNX export)
        """
        # Encoder
        x = self.conv1(x)
        x = self.conv2(x)
        x = self.conv3(x)

        # Residual blocks
        features = self.res_blocks(x)

        # Decoder
        x = self.deconv1(features)
        x = self.deconv2(x)

        # Final output
        x = self.final_pad(x)
        x = self.final_conv(x)
        output = self.tanh(x)

        if return_features:
            return features, output
        else:
            return output

    def get_param_count(self):
        """Get total number of parameters"""
        return sum(p.numel() for p in self.parameters())


class ReCoNetExport(nn.Module):
    """
    Wrapper for ONNX export that:
    1. Accepts [-1, 1] input (C++ normalizes to this range)
    2. Returns only output (no features tuple)
    3. Outputs [-1, 1] range (C++ converts to [0, 1] for display)
    """
    def __init__(self, reconet):
        super(ReCoNetExport, self).__init__()
        self.reconet = reconet

    def forward(self, x):
        return self.reconet(x, return_features=False)


if __name__ == "__main__":
    # Test model
    model = ReCoNet()
    print(f"ReCoNet model created")
    print(f"Parameters: {model.get_param_count():,}")

    print("\n=== Training mode tests (input [-1, 1]) ===")
    # Test forward pass with different resolutions
    test_resolutions = [(256, 256), (512, 512), (1024, 1024), (1920, 1080)]

    for h, w in test_resolutions:
        x = torch.rand(1, 3, h, w) * 2.0 - 1.0  # [-1, 1] range
        features, y = model(x, return_features=True)
        print(f"Input {x.shape} [-1,1] -> Features {features.shape}, Output {y.shape} [{y.min():.2f}, {y.max():.2f}]")

    print("\n=== ONNX export mode tests ===")
    export_model = ReCoNetExport(model)
    x = torch.rand(1, 3, 256, 256) * 2.0 - 1.0  # [-1, 1] range
    y = export_model(x)
    print(f"Export wrapper: Input {x.shape} -> Output {y.shape} [{y.min():.2f}, {y.max():.2f}]")

    # Verify output range is in [-1, 1]
    assert y.min() >= -1.1 and y.max() <= 1.1, f"Output range unexpected: [{y.min():.2f}, {y.max():.2f}]"
    print(f"Output range check passed: model outputs [-1, 1] range")

    print("\nModel test passed!")
