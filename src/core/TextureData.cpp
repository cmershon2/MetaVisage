#define STB_IMAGE_IMPLEMENTATION
#include "thirdparty/stb_image.h"

#include "core/TextureData.h"
#include "utils/Logger.h"
#include <algorithm>
#include <cmath>

namespace MetaVisage {

TextureData::TextureData()
    : width_(0), height_(0), channels_(4) {
}

TextureData::~TextureData() {
}

std::shared_ptr<TextureData> TextureData::LoadFromFile(const QString& path) {
    auto texture = std::make_shared<TextureData>();
    texture->filePath_ = path;

    int reqChannels = 4; // Always load as RGBA
    int w, h, c;
    unsigned char* data = stbi_load(path.toStdString().c_str(), &w, &h, &c, reqChannels);
    if (!data) {
        MV_LOG_ERROR(QString("Failed to load texture '%1': %2")
            .arg(path, stbi_failure_reason()));
        return nullptr;
    }

    texture->width_ = w;
    texture->height_ = h;
    texture->channels_ = 4;
    texture->pixels_.assign(data, data + (w * h * 4));

    stbi_image_free(data);

    MV_LOG_INFO(QString("Loaded texture '%1': %2x%3 (%4 channels in source)")
        .arg(path).arg(w).arg(h).arg(c));

    return texture;
}

std::shared_ptr<TextureData> TextureData::Create(int width, int height) {
    auto texture = std::make_shared<TextureData>();
    texture->width_ = width;
    texture->height_ = height;
    texture->channels_ = 4;
    texture->pixels_.resize(width * height * 4, 0);
    return texture;
}

void TextureData::UVToPixel(float u, float v, float& px, float& py) const {
    // Wrap UVs to [0, 1)
    u = u - std::floor(u);
    v = v - std::floor(v);

    // Convert to pixel coordinates (center of pixel)
    px = u * static_cast<float>(width_) - 0.5f;
    py = v * static_cast<float>(height_) - 0.5f;
}

Vector3 TextureData::SampleBilinear(float u, float v) const {
    if (pixels_.empty() || width_ <= 0 || height_ <= 0) {
        return Vector3(0.0f, 0.0f, 0.0f);
    }

    float px, py;
    UVToPixel(u, v, px, py);

    int x0 = static_cast<int>(std::floor(px));
    int y0 = static_cast<int>(std::floor(py));
    float fx = px - x0;
    float fy = py - y0;

    // Clamp to valid range
    auto clampX = [this](int x) { return std::max(0, std::min(x, width_ - 1)); };
    auto clampY = [this](int y) { return std::max(0, std::min(y, height_ - 1)); };

    int x0c = clampX(x0);
    int y0c = clampY(y0);
    int x1c = clampX(x0 + 1);
    int y1c = clampY(y0 + 1);

    auto getColor = [this](int x, int y) -> Vector3 {
        int idx = (y * width_ + x) * 4;
        return Vector3(
            pixels_[idx + 0] / 255.0f,
            pixels_[idx + 1] / 255.0f,
            pixels_[idx + 2] / 255.0f
        );
    };

    Vector3 c00 = getColor(x0c, y0c);
    Vector3 c10 = getColor(x1c, y0c);
    Vector3 c01 = getColor(x0c, y1c);
    Vector3 c11 = getColor(x1c, y1c);

    // Bilinear interpolation
    Vector3 top = c00 * (1.0f - fx) + c10 * fx;
    Vector3 bottom = c01 * (1.0f - fx) + c11 * fx;
    return top * (1.0f - fy) + bottom * fy;
}

Vector3 TextureData::SampleBilinearRGB(float u, float v, float& outAlpha) const {
    if (pixels_.empty() || width_ <= 0 || height_ <= 0) {
        outAlpha = 0.0f;
        return Vector3(0.0f, 0.0f, 0.0f);
    }

    float px, py;
    UVToPixel(u, v, px, py);

    int x0 = static_cast<int>(std::floor(px));
    int y0 = static_cast<int>(std::floor(py));
    float fx = px - x0;
    float fy = py - y0;

    auto clampX = [this](int x) { return std::max(0, std::min(x, width_ - 1)); };
    auto clampY = [this](int y) { return std::max(0, std::min(y, height_ - 1)); };

    int x0c = clampX(x0);
    int y0c = clampY(y0);
    int x1c = clampX(x0 + 1);
    int y1c = clampY(y0 + 1);

    auto getAlpha = [this](int x, int y) -> float {
        int idx = (y * width_ + x) * 4;
        return pixels_[idx + 3] / 255.0f;
    };

    float a00 = getAlpha(x0c, y0c);
    float a10 = getAlpha(x1c, y0c);
    float a01 = getAlpha(x0c, y1c);
    float a11 = getAlpha(x1c, y1c);

    float topA = a00 * (1.0f - fx) + a10 * fx;
    float botA = a01 * (1.0f - fx) + a11 * fx;
    outAlpha = topA * (1.0f - fy) + botA * fy;

    return SampleBilinear(u, v);
}

void TextureData::SetPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    int idx = (y * width_ + x) * 4;
    pixels_[idx + 0] = r;
    pixels_[idx + 1] = g;
    pixels_[idx + 2] = b;
    pixels_[idx + 3] = a;
}

void TextureData::GetPixel(int x, int y, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        r = g = b = a = 0;
        return;
    }
    int idx = (y * width_ + x) * 4;
    r = pixels_[idx + 0];
    g = pixels_[idx + 1];
    b = pixels_[idx + 2];
    a = pixels_[idx + 3];
}

} // namespace MetaVisage
