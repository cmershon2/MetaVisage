#ifndef TEXTUREWRITER_H
#define TEXTUREWRITER_H

#include <string>

namespace MetaVisage {

class TextureData;

class TextureWriter {
public:
    // Write texture data to a PNG file
    static bool WritePNG(const TextureData& texture, const std::string& path);

    // Write texture data to a TGA file
    static bool WriteTGA(const TextureData& texture, const std::string& path);
};

} // namespace MetaVisage

#endif // TEXTUREWRITER_H
