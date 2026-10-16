#ifndef MESHEXPORTER_H
#define MESHEXPORTER_H

#include "core/Mesh.h"
#include "core/Transform.h"
#include <functional>
#include <string>

namespace MetaVisage {

enum class ExportFormat {
    FBX,
    OBJ,
    GLTF
};

struct ExportOptions {
    ExportFormat format;
    bool triangulate;
    bool includeMaterials;
    bool applyTransform;        // Bake transform into vertices
    bool convertToYUp;          // Convert coordinate system to Y-up for UE
    float scaleFactor;
    std::string ueNamingPrefix; // e.g. "SK_" for skeletal mesh naming
    bool metaHumanCompatible;   // Reconstruct original vertex layout for MetaHuman Conform
    bool undoUVFlip;            // Undo aiProcess_FlipUVs Y-flip from import

    // Texture baking options
    bool exportBakedTextures;   // Bake and write textures alongside mesh
    int bakeResolution;         // Texture resolution: 1024, 2048, or 4096
    std::string textureFormat;  // "png" or "tga"

    ExportOptions()
        : format(ExportFormat::FBX),
          // Default OFF — UE5 MetaHuman Conform requires source quad topology
          // preserved end-to-end. The export modal also forces this to false;
          // the default here protects any direct callers that build options
          // outside the modal path.
          triangulate(false),
          includeMaterials(true),
          applyTransform(true),
          convertToYUp(true),
          scaleFactor(1.0f),
          ueNamingPrefix(""),
          metaHumanCompatible(false),
          undoUVFlip(false),
          exportBakedTextures(false),
          bakeResolution(2048),
          textureFormat("png") {}
};

struct ExportResult {
    bool success;
    std::string errorMessage;
    std::string exportedFilePath;
    int vertexCount;
    int faceCount;
    std::string bakedAlbedoPath;
    std::string bakedNormalMapPath;

    ExportResult() : success(false), vertexCount(0), faceCount(0) {}
};

using ExportProgressCallback = std::function<void(float progress, const std::string& message)>;

class MeshExporter {
public:
    MeshExporter();
    ~MeshExporter();

    ExportResult Export(const Mesh& mesh, const std::string& filepath,
                        const ExportOptions& options,
                        const Transform* transform = nullptr,
                        const Mesh* originalMorphMesh = nullptr);

    void SetProgressCallback(ExportProgressCallback callback) { progressCallback_ = callback; }

    static std::string GetFormatExtension(ExportFormat format);
    static std::string GetFormatFilter(ExportFormat format);
    static std::string GetAllFormatsFilter();

private:
    ExportProgressCallback progressCallback_;

    void ReportProgress(float progress, const std::string& message);

    // Direct OBJ writer: writes with separate v/vt/vn indices to preserve topology
    ExportResult ExportOBJDirect(const Mesh& mesh, const std::string& filepath,
                                  const ExportOptions& options,
                                  const Transform* transform);

#ifdef HAVE_ASSIMP
    ExportResult ExportWithAssimp(const Mesh& mesh, const std::string& filepath,
                                   const ExportOptions& options,
                                   const Transform* transform);

    ExportResult ExportFBXMetaHuman(const Mesh& mesh, const std::string& filepath,
                                     const ExportOptions& options,
                                     const Transform* transform);
#endif

    // MetaHuman OBJ export: rewrites the original OBJ file with deformed positions
    ExportResult ExportOBJMetaHuman(const Mesh& deformedMesh,
                                     const Mesh& originalMesh,
                                     const std::string& filepath,
                                     const ExportOptions& options,
                                     const Transform* transform);
};

} // namespace MetaVisage

#endif // MESHEXPORTER_H
