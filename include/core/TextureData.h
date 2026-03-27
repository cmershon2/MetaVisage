#ifndef TEXTUREDATA_H
#define TEXTUREDATA_H

#include "core/Types.h"
#include <QString>
#include <vector>
#include <cstdint>
#include <memory>

namespace MetaVisage {

// CPU-side texture storage for loading, sampling, and baking
class TextureData {
public:
    TextureData();
    ~TextureData();

    // Load an image from disk (PNG, TGA, JPG, BMP supported)
    static std::shared_ptr<TextureData> LoadFromFile(const QString& path);

    // Create an empty texture with given dimensions (RGBA, initialized to zero)
    static std::shared_ptr<TextureData> Create(int width, int height);

    // Bilinear sample at UV coordinates [0,1]. Returns RGB in [0,1].
    Vector3 SampleBilinear(float u, float v) const;

    // Bilinear sample returning RGBA in [0,1] as a Vector3 (RGB) + separate alpha
    Vector3 SampleBilinearRGB(float u, float v, float& outAlpha) const;

    // Direct pixel access (x, y in pixel coordinates)
    void SetPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    void GetPixel(int x, int y, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const;

    // Accessors
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    int GetChannels() const { return channels_; }
    bool IsLoaded() const { return !pixels_.empty() && width_ > 0 && height_ > 0; }
    const QString& GetFilePath() const { return filePath_; }
    const std::vector<uint8_t>& GetPixels() const { return pixels_; }
    std::vector<uint8_t>& GetPixels() { return pixels_; }

private:
    std::vector<uint8_t> pixels_;  // RGBA pixel data
    int width_;
    int height_;
    int channels_;                 // Always stored as 4 (RGBA) internally
    QString filePath_;

    // Helper: clamp and wrap UV, return pixel coords
    void UVToPixel(float u, float v, float& px, float& py) const;
};

// Texture set for a mesh (albedo + optional normal map)
struct TextureSet {
    std::shared_ptr<TextureData> albedo;
    std::shared_ptr<TextureData> normalMap;

    bool HasAlbedo() const { return albedo && albedo->IsLoaded(); }
    bool HasNormalMap() const { return normalMap && normalMap->IsLoaded(); }
};

} // namespace MetaVisage

#endif // TEXTUREDATA_H
