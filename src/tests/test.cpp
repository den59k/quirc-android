#include <iostream>
#include <vector>
#include <cstring>
#include <filesystem>
#include <string>
#include <algorithm>

#include "quirc.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace fs = std::filesystem;

static const std::string IMAGES_DIR  = "images";
static const std::string BINARY_DIR  = "binary_out";

struct TestResult {
    std::string filename;
    bool        loaded   = false;
    int         detected = 0;
    int         decoded  = 0;
    std::string payload;
    std::string error;
};

static bool is_image(const fs::path& p) {
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg"
        || ext == ".bmp" || ext == ".tga";
}

static TestResult test_image(const fs::path& path) {
    TestResult r;
    r.filename = path.filename().string();

    int width, height, channels;
    unsigned char* img = stbi_load(path.string().c_str(), &width, &height, &channels, 0);
    if (!img) {
        r.error = "failed to load image";
        return r;
    }
    r.loaded = true;

    std::vector<unsigned char> gray(width * height);
    if (channels == 1) {
        std::memcpy(gray.data(), img, width * height);
    } else {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = (y * width + x) * channels;
                unsigned char rv = img[idx + 0];
                unsigned char gv = img[idx + 1];
                unsigned char bv = img[idx + 2];
                gray[y * width + x] =
                    static_cast<unsigned char>(0.299f * rv + 0.587f * gv + 0.114f * bv);
            }
        }
    }
    stbi_image_free(img);

    struct quirc* q = quirc_new();
    if (!q) { r.error = "quirc_new failed"; return r; }

    if (quirc_resize(q, width, height) < 0) {
        r.error = "quirc_resize failed";
        quirc_destroy(q);
        return r;
    }

    unsigned char* buf = quirc_begin(q, nullptr, nullptr);
    std::memcpy(buf, gray.data(), width * height);
    quirc_end(q);

    r.detected = quirc_count(q);

    for (int i = 0; i < r.detected; ++i) {
        struct quirc_code code;
        struct quirc_data data;
        quirc_extract(q, i, &code);
        if (quirc_decode(&code, &data) == 0) {
            ++r.decoded;
            if (r.payload.empty())
                r.payload = std::string(reinterpret_cast<char*>(data.payload),
                                        data.payload_len);
        }
    }

    // Сохраняем бинаризованное изображение для отладки
    fs::create_directories(BINARY_DIR);
    std::vector<unsigned char> binary(width * height);
    get_binary(q, binary.data(), width, height);
    std::string out_name = BINARY_DIR + "/" + r.filename;
    // Всегда сохраняем как PNG
    auto dot = out_name.rfind('.');
    if (dot != std::string::npos) out_name.replace(dot, std::string::npos, ".png");
    stbi_write_png(out_name.c_str(), width, height, 1, binary.data(), width);

    quirc_destroy(q);
    return r;
}

int main(int argc, char* argv[]) {
    std::string dir = (argc > 1) ? argv[1] : IMAGES_DIR;

    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        std::cerr << "Images directory not found: " << dir << "\n";
        std::cerr << "Create '" << dir << "/' and put QR-code images inside.\n";
        return 1;
    }

    std::vector<fs::path> files;
    for (auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_regular_file() && is_image(entry.path()))
            files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());

    if (files.empty()) {
        std::cerr << "No image files found in: " << dir << "\n";
        return 1;
    }

    std::cout << "Testing " << files.size() << " image(s) from '" << dir << "'...\n";
    std::cout << "Binarized images saved to '" << BINARY_DIR << "/'\n\n";

    int passed = 0, failed = 0;

    for (auto& path : files) {
        TestResult r = test_image(path);

        bool ok = (r.decoded > 0);
        std::cout << (ok ? "[PASS]" : "[FAIL]") << " " << r.filename;

        if (!r.loaded) {
            std::cout << " — " << r.error;
        } else if (!r.error.empty()) {
            std::cout << " — " << r.error;
        } else if (r.detected == 0) {
            std::cout << " — no QR regions detected";
        } else if (r.decoded == 0) {
            std::cout << " — detected " << r.detected << " region(s), but decode failed";
        } else {
            std::cout << " — \"" << r.payload << "\"";
            if (r.decoded > 1)
                std::cout << " (+" << (r.decoded - 1) << " more)";
        }

        std::cout << "\n";

        if (ok) ++passed; else ++failed;
    }

    std::cout << "\n--- Results: " << passed << " passed, " << failed << " failed"
              << " out of " << files.size() << " ---\n";

    return (failed == 0) ? 0 : 1;
}
