#include <iostream>
#include <vector>
#include <cstring>

#include "quirc.h"

// stb
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int main() {
    const char* filename = "qr.png";

    int width, height, channels;

    unsigned char* img = stbi_load(filename, &width, &height, &channels, 0);
    if (!img) {
        std::cerr << "Failed to load image\n";
        return 1;
    }

    std::cout << "Loaded: " << width << "x" << height
              << " channels=" << channels << "\n";

    // Конвертация в grayscale
    std::vector<unsigned char> gray(width * height);

    if (channels == 1) {
        std::memcpy(gray.data(), img, width * height);
    } else {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = (y * width + x) * channels;
                unsigned char r = img[idx + 0];
                unsigned char g = img[idx + 1];
                unsigned char b = img[idx + 2];

                // стандартная luminance формула
                gray[y * width + x] =
                    static_cast<unsigned char>(
                        0.299f * r + 0.587f * g + 0.114f * b
                    );
            }
        }
    }

    stbi_image_free(img);

    // === quirc ===

    struct quirc* q = quirc_new();
    if (!q) {
        std::cerr << "quirc_new failed\n";
        return 1;
    }

    if (quirc_resize(q, width, height) < 0) {
        std::cerr << "quirc_resize failed\n";
        quirc_destroy(q);
        return 1;
    }

    unsigned char* image = quirc_begin(q, nullptr, nullptr);
    std::memcpy(image, gray.data(), width * height);
    quirc_end(q);

    int count = quirc_count(q);
    std::cout << "Detected QR regions: " << count << "\n";

    for (int i = 0; i < count; ++i) {
        struct quirc_code code;
        struct quirc_data data;

        quirc_extract(q, i, &code);

        auto err = quirc_decode(&code, &data);
        if (err) {
            std::cout << "Decode error: " << err << "\n";
            continue;
        }

        std::cout << "Payload: "
                  << std::string(reinterpret_cast<char*>(data.payload),
                                 data.payload_len)
                  << "\n";

        std::cout << "ECC level: " << data.ecc_level << "\n";

        std::cout << "Corners:\n";
        for (int c = 0; c < 4; ++c) {
            std::cout << "  (" << code.corners[c].x
                      << ", " << code.corners[c].y << ")\n";
        }
    }

    // === Сохраняем бинаризованное изображение ===
    // После quirc_end q->image содержит 0/1 карту.
    // Преобразуем в 0/255 для сохранения.
    std::vector<unsigned char> binary(width * height);

    get_binary(q, binary.data(), width, height);

    stbi_write_png("binary.png", width, height, 1,
                   binary.data(), width);

    std::cout << "Saved binary.png\n";

    quirc_destroy(q);

    return 0;
}