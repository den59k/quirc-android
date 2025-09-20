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
