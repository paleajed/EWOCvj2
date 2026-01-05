"""Calculate proper style weight based on loss magnitudes"""
import torch
import sys
sys.path.insert(0, 'C:/ProgramData/EWOCvj2/scripts')

from loss_functions import PerceptualLoss

loss_fn = PerceptualLoss()

# Test with random images to get baseline loss magnitudes
img1 = torch.randn(1, 3, 512, 512)  # Use 512x512 like training
img2 = torch.randn(1, 3, 512, 512)

content_loss = loss_fn.content_loss(img1, img2).item()
style_loss = loss_fn.style_loss(img1, img2).item()

print(f"Content loss (random 512x512): {content_loss:.6f}")
print(f"Style loss (random 512x512): {style_loss:.10f}")
print()

# Calculate weight needed to equalize
if style_loss > 0:
    weight_to_equalize = content_loss / style_loss
    print(f"Style weight to EQUALIZE losses: {weight_to_equalize:.0f}")
    print(f"Style weight for style to be 10x content: {weight_to_equalize * 10:.0f}")
    print(f"Style weight for style to be 100x content: {weight_to_equalize * 100:.0f}")
else:
    print("Style loss is zero - something is wrong")

print()
print("Recommended: Use style_weight = 1e7 or 1e8")
