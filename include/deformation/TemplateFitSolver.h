#ifndef TEMPLATEFITSOLVER_H
#define TEMPLATEFITSOLVER_H

#include "deformation/LandmarkSolver.h"
#include "deformation/NRICPDeformer.h"

#include <string>
#include <vector>

namespace MetaVisage {

// Stiff, short wrap schedule for the coarse fit: it only has to land each
// landmark in the right neighbourhood, so it trades detail for speed.
NRICPParams TemplateFitCoarseParams();

// Sprint 12 — geometric template-fit landmark solver (design decision 1).
//
// 1. Similarity refinement: trimmed ICP with uniform scale (closed-form
//    Umeyama per step) of the source's landmark region onto the target,
//    starting from the Stage 1 transforms, then a bounded affine refinement
//    for differing proportions. Internal only.
// 1c. Feature refit: each eye and ear gets a bounded affine fit of its own,
//    matched both ways so an oversized feature pulls the patch outward.
// 2. Coarse fit: a stiff, short NRICP run with no landmarks that brings each
//    landmark into the right neighbourhood of the target.
// 3. Projection: each fitted landmark snaps to the closest target surface
//    point and that triangle's nearest vertex; crease landmarks (mouth
//    corners) then move to the deepest point of the crease nearby.
// 4. Symmetry: on a near-symmetric target, mirrored pairs are averaged and
//    midline landmarks are pulled onto the symmetry plane.
// 5. Confidence: projection distance, normal agreement, mirror consistency
//    and the similarity residual.
class TemplateFitSolver : public LandmarkSolver {
public:
    struct Params {
        int similaritySamples = 4000;
        int similarityIterations = 40;
        float trimFraction = 0.8f;       // keep the closest 80% of matches per step
        float roiPadding = 0.15f;        // landmark bounding box padding, fraction of its size
        float maxScaleChange = 2.0f;     // refuse fits that scale more than 2x either way
        float maxRotationDeg = 45.0f;    // refuse fits that rotate further than this
        float flipResidualRatio = 0.75f; // a 180-degree pose fitting this much better means "facing wrong way"
        int affineIterations = 20;       // proportion fit after the similarity fit
        float maxAffineStretch = 1.4f;   // per-axis stretch allowed beyond the similarity fit
        float symmetryThreshold = 0.002f; // median mirror distance, fraction of target diagonal
        float confidenceRadius = 0.025f;  // neighbourhood checked around each landmark, fraction of diagonal
        float confidenceResidualZero = 0.01f; // neighbourhood residual (fraction of diagonal) scoring zero
        float distinctSpread = 0.02f;     // normal spread at which a landmark counts as well defined
        NRICPParams coarse = TemplateFitCoarseParams();

        // Feature refit: each eye and ear gets its own bounded affine fit
        // before the coarse fit. On the six-head dataset a mouth refit pulled
        // closed lips apart and a nose refit moved nostrils off, so both are
        // left to the coarse fit.
        bool featureRefit = true;
        std::vector<std::string> refitRegions = {"eye", "ear"};
        int regionIterations = 15;
        int regionSamples = 1500;
        float regionPatchScale = 1.6f;    // patch radius, multiple of the region landmarks' spread
        float regionMinRadius = 0.03f;    // fraction of target diagonal
        float maxRegionStretch = 2.2f;    // per-axis size change allowed for one feature
        float maxRegionShift = 0.6f;      // feature centre may move this fraction of the patch radius
        float regionRegularization = 0.05f; // pull toward no change where the patch is too flat to decide

        // Feature snapping: the last nudge of crease landmarks (mouth corners,
        // lip contact) onto the deepest point of the crease.
        bool featureSnap = true;
        float snapRadius = 0.015f;        // search radius, fraction of target diagonal
        float snapScale = 0.006f;         // neighbourhood used to measure crease depth, fraction of diagonal
        float snapMinDepth = 0.05f;       // crease depth (relative to snapScale) needed to move
        float snapDistancePenalty = 0.1f; // depth traded per search radius moved
    };

    TemplateFitSolver() = default;
    explicit TemplateFitSolver(const Params& params) : params_(params) {}

    LandmarkSolveResult Solve(const LandmarkSolveInput& input,
                              const LandmarkProgressCallback& progress,
                              std::atomic<bool>& cancel) override;

    const Params& GetParams() const { return params_; }

private:
    Params params_;
};

} // namespace MetaVisage

#endif // TEMPLATEFITSOLVER_H
