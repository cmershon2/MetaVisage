#ifndef MESHEXPORTER_H
#define MESHEXPORTER_H

#include "core/Mesh.h"
#include "core/Transform.h"
#include <QString>
#include <functional>

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
    QString ueNamingPrefix;     // e.g. "SK_" for skeletal mesh naming
    bool metaHumanCompatible;   // Reconstruct original vertex layout for MetaHuman Conform
    bool undoUVFlip;            // Undo aiProcess_FlipUVs Y-flip from import

    ExportOptions()
        : format(ExportFormat::FBX),
          triangulate(true),
          includeMaterials(true),
          applyTransform(true),
          convertToYUp(true),
          scaleFactor(1.0f),
          ueNamingPrefix(""),
          metaHumanCompatible(false),
          undoUVFlip(false) {}
};

struct ExportResult {
    bool success;
    QString errorMessage;
    QString exportedFilePath;
    int vertexCount;
    int faceCount;

    ExportResult() : success(false), vertexCount(0), faceCount(0) {}
};

using ExportProgressCallback = std::function<void(float progress, const QString& message)>;

class MeshExporter {
public:
    MeshExporter();
    ~MeshExporter();

    ExportResult Export(const Mesh& mesh, const QString& filepath,
                        const ExportOptions& options,
                        const Transform* transform = nullptr,
                        const Mesh* originalMorphMesh = nullptr);

    void SetProgressCallback(ExportProgressCallback callback) { progressCallback_ = callback; }

    static QString GetFormatExtension(ExportFormat format);
    static QString GetFormatFilter(ExportFormat format);
    static QString GetAllFormatsFilter();

private:
    ExportProgressCallback progressCallback_;

    void ReportProgress(float progress, const QString& message);

    // Direct OBJ writer: writes with separate v/vt/vn indices to preserve topology
    ExportResult ExportOBJDirect(const Mesh& mesh, const QString& filepath,
                                  const ExportOptions& options,
                                  const Transform* transform);

#ifdef HAVE_ASSIMP
    ExportResult ExportWithAssimp(const Mesh& mesh, const QString& filepath,
                                   const ExportOptions& options,
                                   const Transform* transform);

    ExportResult ExportFBXMetaHuman(const Mesh& mesh, const QString& filepath,
                                     const ExportOptions& options,
                                     const Transform* transform);
#endif

    // MetaHuman OBJ export: rewrites the original OBJ file with deformed positions
    ExportResult ExportOBJMetaHuman(const Mesh& deformedMesh,
                                     const Mesh& originalMesh,
                                     const QString& filepath,
                                     const ExportOptions& options,
                                     const Transform* transform);
};

} // namespace MetaVisage

#endif // MESHEXPORTER_H
