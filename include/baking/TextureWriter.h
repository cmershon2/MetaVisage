#ifndef TEXTUREWRITER_H
#define TEXTUREWRITER_H

#include <QString>

namespace MetaVisage {

class TextureData;

class TextureWriter {
public:
    // Write texture data to a PNG file
    static bool WritePNG(const TextureData& texture, const QString& path);

    // Write texture data to a TGA file
    static bool WriteTGA(const TextureData& texture, const QString& path);
};

} // namespace MetaVisage

#endif // TEXTUREWRITER_H
