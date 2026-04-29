#ifndef WRAP_PRESET_H
#define WRAP_PRESET_H

#include "core/Project.h"

#include <string>
#include <vector>

namespace MetaVisage {

// Parameter snapshot for the Wrap Engine. Mirrors the parameter subset of
// MorphData (everything that the Wrap Engine consumes) without the runtime
// state — no displacement magnitudes, no deformedMorphMesh, no vertex mask,
// no isProcessed/isAccepted flags. A WrapPreset wraps one of these, plus
// the bookkeeping the UI needs (id / name / icon).
//
// The default constructor matches the MorphData() constructor defaults
// byte-for-byte, so WrapParams{} *is* the canonical Fantasy Wrap baseline
// — see WrapPreset::FantasyWrap().
struct WrapParams {
    DeformationAlgorithm algorithm        = DeformationAlgorithm::NRICP;
    float stiffness                       = 0.5f;
    float smoothness                      = 0.5f;

    int   nricpStiffnessSteps             = 5;
    float nricpAlphaInitial               = 100.0f;
    float nricpAlphaFinal                 = 50.0f;
    int   nricpIcpIterations              = 7;
    float nricpNormalThreshold            = 60.0f;
    float nricpLandmarkWeight             = 10.0f;
    float nricpEpsilon                    = 1e-4f;

    bool  nricpEnableBoundaryExclusion    = true;
    int   nricpBoundaryExclusionHops      = 3;

    int   nricpOptimizationIterations     = 1;
    float nricpDpInitial                  = 1.0f;
    float nricpDpFinal                    = 1.0f;

    float nricpGammaInitial               = 0.0f;
    float nricpGammaFinal                 = 0.0f;

    float nricpSamplingInitial            = 0.1f;
    float nricpSamplingFinal              = 0.01f;
    bool  nricpNormalizeSampling          = true;
};

// A named Wrap Engine parameter snapshot. Built-in presets are read-only
// (isBuiltIn = true) — the UI's save/rename/delete paths refuse to mutate
// them. User-saved presets (Story 4.4) will set isBuiltIn = false.
//
// id is the stable key the inspector card row tracks active selection by;
// it must be unique across the running set of (built-in ∪ user) presets.
// name / description / iconGlyph are display-only.
struct WrapPreset {
    std::string id;
    std::string name;
    std::string description;
    std::string iconGlyph;   // Lucide PUA codepoint string (see ui/MvIcons.h)
    bool        isBuiltIn = false;
    WrapParams  params;

    // Copy this preset's params onto the project's morph data. Touches only
    // the parameter fields — runtime state (displacementMagnitudes,
    // deformedMorphMesh, vertexMask, isProcessed/isAccepted, etc.) is left
    // untouched, so applying a preset mid-project doesn't blow away an
    // already-computed deformed mesh.
    void ApplyTo(MorphData& morph) const;

    // Returns true if every parameter field in `morph` exactly equals this
    // preset's params snapshot. Used by the inspector to figure out which
    // card to highlight after a project load or after the user edits a
    // slider back to a preset's exact value.
    bool MatchesParamsOf(const MorphData& morph) const;

    // Canonical built-ins. Quick Wrap is the project default.
    static const WrapPreset& QuickWrap();
    static const WrapPreset& FantasyWrap();

    // Returns Quick → Fantasy in card-row display order. Story 4.4's user
    // presets will be rendered after this list in the same row.
    static std::vector<WrapPreset> BuiltIns();

    // Finds a built-in whose params exactly match `morph`. Returns the
    // matching id, or empty string if no built-in matches (the user has
    // tweaked parameters since the last preset was applied).
    static std::string FindMatchingBuiltInId(const MorphData& morph);
};

} // namespace MetaVisage

#endif // WRAP_PRESET_H
