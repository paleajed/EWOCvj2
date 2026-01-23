"""
reconet_model.py

ReCoNet (Real-time Collection Network) Model Architecture
Lightweight style transfer network for real-time inference

Key features to avoid artifacts:
- Reflection padding throughout (avoids edge/border artifacts)
- Bilinear Upsample+Conv decoder (avoids checkerboard artifacts)

License: GPL3
"""

import torch
import torch.nn as nn


class ResidualBlock(nn.Module):
    """Residual block with instance normalization and reflection padding"""
    def __init__(self, channels):
        super(ResidualBlock, self).__init__()
        # Use reflection padding to avoid edge artifacts
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
        out = self.relu(self.in1(self.conv1(out)))
        out = self.pad2(out)
        out = self.in2(self.conv2(out))
        out = out + residual
        return out


class UpsampleConv(nn.Module):
    """
    Upsample + Conv layer to avoid checkerboard artifacts.

    Uses bilinear upsampling followed by a regular convolution with reflection padding,
    which produces smoother results than ConvTranspose2d (deconvolution).
    - Bilinear is preferred over nearest-neighbor to avoid block artifacts
    - Reflection padding avoids border artifacts and periodic patterns
    See: "Deconvolution and Checkerboard Artifacts" (Odena et al., 2016)
    """
    def __init__(self, in_channels, out_channels, kernel_size=3, scale_factor=2):
        super(UpsampleConv, self).__init__()
        # Use nearest-neighbor upsampling (bilinear causes blur)
        self.upsample = nn.Upsample(scale_factor=scale_factor, mode='nearest')
        # Use reflection padding instead of zero padding to avoid border artifacts
        padding = kernel_size // 2
        self.pad = nn.ReflectionPad2d(padding)
        self.conv = nn.Conv2d(in_channels, out_channels, kernel_size=kernel_size,
                              stride=1, padding=0)  # padding handled by ReflectionPad2d
        self.norm = nn.InstanceNorm2d(out_channels, affine=True)
        self.relu = nn.ReLU(inplace=True)

    def forward(self, x):
        x = self.upsample(x)
        x = self.pad(x)
        x = self.conv(x)
        x = self.norm(x)
        x = self.relu(x)
        return x


class ReCoNet(nn.Module):
    """
    ReCoNet Style Transfer Network

    Architecture:
    - Encoder: 3 conv layers (downsampling)
    - Residual blocks: 5 blocks (feature transformation)
    - Decoder: 2 upsample+conv layers (upsampling without checkerboard)

    Input/Output: RGB images, any resolution (fully convolutional)
    """

    def __init__(self):
        super(ReCoNet, self).__init__()

        # Encoder (Downsampling) - uses reflection padding to avoid edge artifacts
        # Initial conv (preserves resolution)
        self.enc_pad1 = nn.ReflectionPad2d(4)
        self.enc_conv1 = nn.Conv2d(3, 32, kernel_size=9, stride=1, padding=0)
        self.enc_norm1 = nn.InstanceNorm2d(32, affine=True)
        self.enc_relu1 = nn.ReLU(inplace=True)

        # Downsample 1 (1/2 resolution)
        self.enc_pad2 = nn.ReflectionPad2d(1)
        self.enc_conv2 = nn.Conv2d(32, 64, kernel_size=3, stride=2, padding=0)
        self.enc_norm2 = nn.InstanceNorm2d(64, affine=True)
        self.enc_relu2 = nn.ReLU(inplace=True)

        # Downsample 2 (1/4 resolution)
        self.enc_pad3 = nn.ReflectionPad2d(1)
        self.enc_conv3 = nn.Conv2d(64, 128, kernel_size=3, stride=2, padding=0)
        self.enc_norm3 = nn.InstanceNorm2d(128, affine=True)
        self.enc_relu3 = nn.ReLU(inplace=True)

        # Residual Blocks (Feature Transformation)
        self.res_blocks = nn.Sequential(
            ResidualBlock(128),
            ResidualBlock(128),
            ResidualBlock(128),
            ResidualBlock(128),
            ResidualBlock(128),
        )

        # Decoder (Upsampling) - uses Upsample+Conv to avoid checkerboard artifacts
        self.upsample1 = UpsampleConv(128, 64, kernel_size=3, scale_factor=2)  # 1/4 -> 1/2
        self.upsample2 = UpsampleConv(64, 32, kernel_size=3, scale_factor=2)   # 1/2 -> full

        # Final conv to RGB with reflection padding (avoids border artifacts)
        self.final_pad = nn.ReflectionPad2d(4)
        self.final_conv = nn.Conv2d(32, 3, kernel_size=9, stride=1, padding=0)
        self.tanh = nn.Tanh()  # Output range [-1, 1]

    def forward(self, x, return_features=False):
        """
        Forward pass

        Args:
            x: Input image tensor [B, 3, H, W], range [-1, 1]
            return_features: If True, return (features, output) for temporal loss

        Returns:
            If return_features=True: (features, output) tuple
                - features: [B, 128, H/4, W/4] for feature-level temporal loss
                - output: [B, 3, H, W] stylized image, range [-1, 1]
            If return_features=False: output only (for inference/ONNX export)
        """
        # Encoder (with reflection padding)
        x = self.enc_pad1(x)
        x = self.enc_relu1(self.enc_norm1(self.enc_conv1(x)))
        x = self.enc_pad2(x)
        x = self.enc_relu2(self.enc_norm2(self.enc_conv2(x)))
        x = self.enc_pad3(x)
        encoded = self.enc_relu3(self.enc_norm3(self.enc_conv3(x)))

        # Residual blocks
        features = self.res_blocks(encoded)

        # Decoder (Upsample+Conv to avoid checkerboard)
        x = self.upsample1(features)
        x = self.upsample2(x)
        x = self.final_pad(x)
        output = self.tanh(self.final_conv(x))

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

    print("\n=== Training mode tests (with features) ===")
    # Test forward pass with different resolutions
    test_resolutions = [(256, 256), (512, 512), (1024, 1024), (1920, 1080)]

    for h, w in test_resolutions:
        x = torch.rand(1, 3, h, w) * 2.0 - 1.0  # [-1, 1] range
        features, y = model(x, return_features=True)
        print(f"Input {x.shape} -> Features {features.shape}, Output {y.shape}")

    print("\n=== ONNX export mode tests ===")
    export_model = ReCoNetExport(model)
    x = torch.rand(1, 3, 256, 256) * 2.0 - 1.0
    y = export_model(x)
    print(f"Export wrapper: Input {x.shape} -> Output {y.shape}")

    print("\nModel test passed!")
