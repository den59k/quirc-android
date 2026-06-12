# quirc-android

This project provides an **Android JNI wrapper** around [quirc](https://github.com/dlbeer/quirc) – a fast and lightweight QR code decoder written in C.

The C library has been slightly modified to improve decoding robustness on mobile devices by using **adaptive binarization** instead of a simple global threshold.

---

## Features
- JNI bindings for Android
- Works with raw grayscale byte arrays (`width * height`)
- Lightweight and dependency-free
- Improved QR detection under uneven lighting conditions

---

## Adaptive binarization (Wolf–Jolion)

Instead of a single global threshold, this fork uses a **local adaptive
threshold based on the Wolf–Jolion method** (an evolution of Sauvola), which is
far more robust to uneven lighting, glare and moiré — especially for QR codes
shown on another screen.

The threshold is anchored to the global brightness minimum `M` and normalized by
the maximum local standard deviation `Rmax`:

```
thresh = mean - k * (1 - stddev / Rmax) * (mean - M)
```

This keeps flat dark areas **black** (finder-pattern centers are no longer
"eaten" into white) and flat bright areas **white**. Mean/variance are computed
in O(1) per pixel via integral images. Default `k = 0.5`.

Two extra details improve real-world reads:

- **Multi-pass:** binarization runs with several window/`k` settings
  (`auto/0.5`, `41/0.5`, `auto/0.3`) and stops at the first decode that succeeds.
- **Majority voting:** each data module is sampled over a 3×3 grid in its central
  ~40% (offsets `0.3 / 0.5 / 0.7`) instead of one pixel, averaging out
  moiré/glare at module centers.

---

## Build

```
./gradlew assembleRelease
```

Output file will be in build/outputs/aar/quirc-release.aar

---

## Usage Example

```java
long ctx = QuircNative.nativeInit();
try {
    // grayBytes: byte array of size width*height (1 byte per pixel, grayscale)
    QuircNative.QuircResult result = QuircNative.nativeScan(ctx, grayBytes, width, height);

    if (result != null) {
        System.out.println("Decoded payload: " + result.payload);
    }
} finally {
    QuircNative.nativeDestroy(ctx);
}
