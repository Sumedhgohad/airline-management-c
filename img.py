import cv2
import numpy as np
import matplotlib.pyplot as plt

# ------------------------------
# 1. Load image (grayscale)
# ------------------------------
img = cv2.imread(r"D:\SY\IMAGE_PROCESSING\Sample_image.jpg", cv2.IMREAD_GRAYSCALE)
if img is None:
    raise FileNotFoundError("Image not found. Please place 'image.jpg' in the working directory.")

# Function to display images
def show_images(images, titles, cmap='gray'):
    plt.figure(figsize=(15, 10))
    for i in range(len(images)):
        plt.subplot(2, 4, i + 1)
        plt.imshow(images[i], cmap=cmap)
        plt.title(titles[i])
        plt.axis('off')
    plt.tight_layout()
    plt.show()

# ------------------------------
# 2. Add Noise Functions
# ------------------------------
def add_gaussian_noise(image, mean=0, sigma=20):
    gauss = np.random.normal(mean, sigma, image.shape)
    noisy = image + gauss
    return np.clip(noisy, 0, 255).astype(np.uint8)

def add_salt_pepper_noise(image, salt_prob=0.02, pepper_prob=0.02):
    noisy = np.copy(image)
    total_pixels = image.size
    num_salt = np.ceil(salt_prob * total_pixels)
    num_pepper = np.ceil(pepper_prob * total_pixels)
    # Add salt (white pixels)
    coords = [np.random.randint(0, i - 1, int(num_salt)) for i in image.shape]
    noisy[coords[0], coords[1]] = 255
    # Add pepper (black pixels)
    coords = [np.random.randint(0, i - 1, int(num_pepper)) for i in image.shape]
    noisy[coords[0], coords[1]] = 0
    return noisy

def add_speckle_noise(image):
    gauss = np.random.randn(*image.shape)
    noisy = image + image * gauss * 0.1
    return np.clip(noisy, 0, 255).astype(np.uint8)

# ------------------------------
# 3. Filtering / Restoration
# ------------------------------

# Mean filter (Averaging)
def mean_filter(image, kernel_size=3):
    return cv2.blur(image, (kernel_size, kernel_size))

# Median filter
def median_filter(image, kernel_size=3):
    return cv2.medianBlur(image, kernel_size)

# Inverse filtering (basic)
def inverse_filter(image, degradation_kernel_size=5):
    # Simulate degradation (blurring)
    h = np.ones((degradation_kernel_size, degradation_kernel_size)) / (degradation_kernel_size ** 2)
    H = np.fft.fft2(h, s=image.shape)
    G = np.fft.fft2(image)
    # Avoid division by zero
    H_inv = np.where(H == 0, 1e-5, H)
    F_hat = G / H_inv
    restored = np.abs(np.fft.ifft2(F_hat))
    return np.clip(restored, 0, 255).astype(np.uint8)

# ------------------------------
# 4. Apply Noise and Restoration
# ------------------------------
gaussian_noisy = add_gaussian_noise(img)
sp_noisy = add_salt_pepper_noise(img)
speckle_noisy = add_speckle_noise(img)

# Apply filters
gaussian_mean = mean_filter(gaussian_noisy)
gaussian_median = median_filter(gaussian_noisy)
gaussian_inverse = inverse_filter(gaussian_noisy)

sp_mean = mean_filter(sp_noisy)
sp_median = median_filter(sp_noisy)
sp_inverse = inverse_filter(sp_noisy)

# ------------------------------
# 5. Display Results
# ------------------------------
show_images(
    [img, gaussian_noisy, gaussian_mean, gaussian_median, gaussian_inverse,
          sp_noisy, sp_mean, sp_median],
    ['Original',
     'Gaussian Noise', 'Mean Filter (Gaussian)', 'Median Filter (Gaussian)', 'Inverse Filter (Gaussian)',
     'Salt & Pepper Noise', 'Mean Filter (S&P)', 'Median Filter (S&P)']
)








