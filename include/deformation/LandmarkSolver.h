#ifndef LANDMARKSOLVER_H
#define LANDMARKSOLVER_H

#include "core/Mesh.h"
#include "core/SourceProfile.h"
#include "core/Transform.h"
#include "core/Types.h"

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace MetaVisage {

// Sprint 12 — auto-map. Places a source profile's landmarks on a target mesh
// that the user has already aligned in Stage 1. Everything the solver needs is
// copied or shared in, never read from Project, so a worker thread can run it
// while the UI keeps drawing. Solvers never change the input transforms
// (design decision 3): any alignment refinement stays internal.
struct LandmarkSolveInput {
    std::shared_ptr<const Mesh> source;
    Transform sourceTransform;
    std::shared_ptr<const Mesh> target;
    Transform targetTransform;

    std::vector<ProfileLandmark> landmarks;   // from the source profile
    Axis symmetryAxis = Axis::X;              // in source mesh space
    float symmetryPlaneOffset = 0.0f;
    std::vector<bool> mask;                   // true = ignore when fitting; empty = none
};

struct SolvedLandmark {
    std::string landmarkId;
    int sourceVertex = -1;
    Vector3 targetPosition;   // world space, on the target surface
    int targetVertex = -1;    // nearest vertex of the hit triangle, like manual placement
    float confidence = 0.0f;  // 0..1
};

struct LandmarkSolveResult {
    bool success = false;
    std::string errorMessage;
    std::vector<SolvedLandmark> landmarks;   // same order as the input landmarks

    // Diagnostics for tests and the benchmark tool.
    double seconds = 0.0;
    float alignmentScale = 1.0f;       // internal similarity fit, relative to Stage 1
    float alignmentRotationDeg = 0.0f;
    float alignmentResidual = 0.0f;    // trimmed RMS after the fit, fraction of target diagonal
    float targetMirrorError = 0.0f;    // median mirror distance, fraction of target diagonal
    bool symmetryApplied = false;
};

using LandmarkProgressCallback = std::function<void(float progress, const std::string& message)>;

class LandmarkSolver {
public:
    virtual ~LandmarkSolver() = default;

    // Runs on the caller's thread. Setting `cancel` makes it return early with
    // success = false and no landmarks.
    virtual LandmarkSolveResult Solve(const LandmarkSolveInput& input,
                                      const LandmarkProgressCallback& progress,
                                      std::atomic<bool>& cancel) = 0;
};

} // namespace MetaVisage

#endif // LANDMARKSOLVER_H
