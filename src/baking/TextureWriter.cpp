#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "thirdparty/stb_image_write.h"

#include "baking/TextureWriter.h"
#include "core/TextureData.h"
#include "utils/Logger.h"

namespace MetaVisage {

bool TextureWriter::WritePNG(const TextureData& texture, const QString& path) {
    if (!texture.IsLoaded()) {
        MV_LOG_ERROR("TextureWriter::WritePNG: texture not loaded");
        return false;
    }

    int result = stbi_write_png(
        path.toStdString().c_str(),
        texture.GetWidth(),
        texture.GetHeight(),
        4, // RGBA
        texture.GetPixels().data(),
        texture.GetWidth() * 4 // stride
    );

    if (result) {
        MV_LOG_INFO(QString("Wrote PNG texture: %1 (%2x%3)")
            .arg(path).arg(texture.GetWidth()).arg(texture.GetHeight()));
    } else {
        MV_LOG_ERROR(QString("Failed to write PNG texture: %1").arg(path));
    }

    return result != 0;
}

bool TextureWriter::WriteTGA(const TextureData& texture, const QString& path) {
    if (!texture.IsLoaded()) {
        MV_LOG_ERROR("TextureWriter::WriteTGA: texture not loaded");
        return false;
    }

    int result = stbi_write_tga(
        path.toStdString().c_str(),
        texture.GetWidth(),
        texture.GetHeight(),
        4, // RGBA
        texture.GetPixels().data()
    );

    if (result) {
        MV_LOG_INFO(QString("Wrote TGA texture: %1 (%2x%3)")
            .arg(path).arg(texture.GetWidth()).arg(texture.GetHeight()));
    } else {
        MV_LOG_ERROR(QString("Failed to write TGA texture: %1").arg(path));
    }

    return result != 0;
}

} // namespace MetaVisage
