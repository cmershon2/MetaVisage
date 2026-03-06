#include "core/Project.h"
#include "io/ProjectSerializer.h"

namespace MetaVisage {

Project::Project()
    : projectName_("Untitled Project"),
      projectPath_(""),
      created_(QDateTime::currentDateTime()),
      lastModified_(QDateTime::currentDateTime()),
      currentStage_(WorkflowStage::Alignment) {
}

Project::~Project() {
}

bool Project::Save(const QString& filepath) {
    ProjectSerializer serializer;
    SerializationResult result = serializer.Save(*this, filepath);
    if (result.success) {
        projectPath_ = filepath;
        lastModified_ = QDateTime::currentDateTime();
    }
    return result.success;
}

bool Project::Load(const QString& filepath) {
    ProjectSerializer serializer;
    SerializationResult result = serializer.Load(*this, filepath);
    if (result.success) {
        projectPath_ = filepath;
    }
    return result.success;
}

bool Project::CanProceedToNextStage() const {
    switch (currentStage_) {
        case WorkflowStage::Alignment:
            // Both meshes must be loaded
            return morphMesh_.isLoaded && targetMesh_.isLoaded;

        case WorkflowStage::PointReference:
            // Point counts must match
            {
                int morphPointCount = 0;
                int targetPointCount = 0;
                for (const auto& corr : pointReference_.correspondences) {
                    if (corr.morphMeshVertexIndex >= 0) morphPointCount++;
                    if (corr.targetMeshVertexIndex >= 0) targetPointCount++;
                }
                return morphPointCount > 0 && morphPointCount == targetPointCount;
            }

        case WorkflowStage::Morph:
            // Morph must be processed and accepted
            return morph_.isProcessed && morph_.isAccepted;

        case WorkflowStage::TouchUp:
            // Always can proceed (to export)
            return true;

        default:
            return false;
    }
}

} // namespace MetaVisage
