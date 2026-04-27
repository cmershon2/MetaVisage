#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "thirdparty/stb_image_write.h"

#include "baking/TextureWriter.h"
#include "core/TextureData.h"
#include "utils/Logger.h"

#include <format>

namespace MetaVisage {

bool TextureWriter::WritePNG(const TextureData& texture, const std::string& path) {
    if (!texture.IsLoaded()) {
        MV_LOG_ERROR("TextureWriter::WritePNG: texture not loaded");
        return false;
    }

    int result = stbi_write_png(
        path.c_str(),
        texture.GetWidth(),
        texture.GetHeight(),
        4, // RGBA
        texture.GetPixels().data(),
        texture.GetWidth() * 4 // stride
    );

    if (result) {
        MV_LOG_INFO(std::format("Wrote PNG texture: {} ({}x{})",
            path, texture.GetWidth(), texture.GetHeight()));
    } else {
        MV_LOG_ERROR(std::format("Failed to write PNG texture: {}", path));
    }

    return result != 0;
}

bool TextureWriter::WriteTGA(const TextureData& texture, const std::string& path) {
    if (!texture.IsLoaded()) {
        MV_LOG_ERROR("TextureWriter::WriteTGA: texture not loaded");
        return false;
    }

    int result = stbi_write_tga(
        path.c_str(),
        texture.GetWidth(),
        texture.GetHeight(),
        4, // RGBA
        texture.GetPixels().data()
    );

    if (result) {
        MV_LOG_INFO(std::format("Wrote TGA texture: {} ({}x{})",
            path, texture.GetWidth(), texture.GetHeight()));
    } else {
        MV_LOG_ERROR(std::format("Failed to write TGA texture: {}", path));
    }

    return result != 0;
}

} // namespace MetaVisage
