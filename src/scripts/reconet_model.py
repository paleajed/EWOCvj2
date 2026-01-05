"""
reconet_model.py.bak

ReCoNet (Real-time Collection Network) Model Architecture
Lightweight style transfer network for real-time inference

PRE-RESEARCH VERSION - before Upsample+Conv and reflection padding changes

License: GPL3
"""

import torch
import torch.nn as nn


class ResidualBlock(nn.Module):
    """Residual block with instance normalization"""
    def __init__(self, channels):
        super(ResidualBlock, self).__init__()
        self.conv1 = nn.Conv2d(channels, channels, kernel_size=3, padding=1)
        self.in1 = nn.InstanceNorm2d(channels, affine=True)
        self.conv2 = nn.Conv2d(channels, channels, kernel_size=3, padding=1)
        self.in2 = nn.InstanceNorm2d(channels, affine=True)
        self.relu = nn.ReLU(inplace=True)

    def forward(self, x):
        residual = x
        out = self.relu(self.in1(self.conv1(x)))
        out = self.in2(self.conv2(out))
        out = out + residual
        return out


class ReCoNet(nn.Module):
    """
    ReCoNet Style Transfer Network

    Architecture:
    - Encoder: 3 conv layers (downsampling)
    - Residual blocks: 5 blocks (feature transformation)
    - Decoder: 2 transposed conv layers (upsampling)

    Input/Output: RGB images, any resolution (fully convolutional)
    """

    def __init__(self):
        super(ReCoNet, self).__init__()

        # Encoder (Downsampling)
        self.encoder = nn.Sequential(
            # Initial conv (preserves resolution)
            nn.Conv2d(3, 32, kernel_size=9, stride=1, padding=4),
            nn.InstanceNorm2d(32, affine=True),
            nn.ReLU(inplace=True),

            # Downsample 1 (1/2 resolution)
            nn.Conv2d(32, 64, kernel_size=3, stride=2, padding=1),
            nn.InstanceNorm2d(64, affine=True),
            nn.ReLU(inplace=True),

            # Downsample 2 (1/4 resolution)
            nn.Conv2d(64, 128, kernel_size=3, stride=2, padding=1),
            nn.InstanceNorm2d(128, affine=True),
            nn.ReLU(inplace=True),
        )

        # Residual Blocks (Feature Transformation)
        self.res_blocks = nn.Sequential(
            ResidualBlock(128),
            ResidualBlock(128),
            ResidualBlock(128),
            ResidualBlock(128),
            ResidualBlock(128),
        )

        # Decoder (Upsampling)
        self.decoder = nn.Sequential(
            # Upsample 1 (1/2 resolution)
            nn.ConvTranspose2d(128, 64, kernel_size=3, stride=2,
                             padding=1, output_padding=1),
            nn.InstanceNorm2d(64, affine=True),
            nn.ReLU(inplace=True),

            # Upsample 2 (full resolution)
            nn.ConvTranspose2d(64, 32, kernel_size=3, stride=2,
                             padding=1, output_padding=1),
            nn.InstanceNorm2d(32, affine=True),
            nn.ReLU(inplace=True),

            # Final conv to RGB
            nn.Conv2d(32, 3, kernel_size=9, stride=1, padding=4),
            nn.Tanh()  # Output range [-1, 1]
        )

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
        # Encoder
        encoded = self.encoder(x)

        # Residual blocks
        features = self.res_blocks(encoded)

        # Decoder
        output = self.decoder(features)

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
