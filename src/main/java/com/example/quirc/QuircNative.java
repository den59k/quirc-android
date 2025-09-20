package com.example.quirc;

public class QuircNative {
    static {
        System.loadLibrary("quirc_jni");
    }

    // Внутреннее представление результата
    public static class QuircResult {
        public final String payload;
        public final int eccLevel;
        public final int[] corners; // x0,y0,x1,y1,x2,y2,x3,y3

        public QuircResult(String payload, int eccLevel, int[] corners) {
            this.payload = payload;
            this.eccLevel = eccLevel;
            this.corners = corners;
        }
    }

    // native API
    public static native long nativeInit();
    public static native void nativeDestroy(long ctx);
    // grayBytes: width*height grayscale bytes (one byte per pixel, row-major)
    public static native QuircResult nativeScan(long ctx, byte[] grayBytes, int width, int height);
}