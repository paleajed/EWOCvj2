"""Test Gram matrix computation and loss values"""
import torch
import sys
sys.path.insert(0, 'C:/ProgramData/EWOCvj2/scripts')

from loss_functions import PerceptualLoss

# Create loss function
loss_fn = PerceptualLoss()

# Create two different images (random noise - should have HIGH style loss)
img1 = torch.randn(1, 3, 256, 256)  # Random image 1
img2 = torch.randn(1, 3, 256, 256)  # Random image 2 (completely different)

# Compute style loss between two random images
print("=== Style loss between TWO RANDOM images (should be HIGH) ===")
s_loss = loss_fn.style_loss(img1, img2, debug=True)
print(f"Total style loss: {s_loss.item():.6f}")

print("\n=== Style loss between SAME image (should be ZERO) ===")
s_loss_same = loss_fn.style_loss(img1, img1, debug=True)
print(f"Total style loss: {s_loss_same.item():.6f}")

print("\n=== Style loss between similar images (small noise added) ===")
img1_noisy = img1 + torch.randn_like(img1) * 0.1
s_loss_noisy = loss_fn.style_loss(img1, img1_noisy, debug=True)
print(f"Total style loss: {s_loss_noisy.item():.6f}")

print("\n=== Content loss comparison ===")
c_loss_random = loss_fn.content_loss(img1, img2)
c_loss_same = loss_fn.content_loss(img1, img1)
print(f"Content loss (random): {c_loss_random.item():.6f}")
print(f"Content loss (same): {c_loss_same.item():.6f}")
