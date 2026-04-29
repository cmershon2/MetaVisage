#include "core/WrapPreset.h"

namespace MetaVisage {

void WrapPreset::ApplyTo(MorphData& morph) const {
    morph.algorithm                      = params.algorithm;
    morph.stiffness                      = params.stiffness;
    morph.smoothness                     = params.smoothness;
    morph.nricpStiffnessSteps            = params.nricpStiffnessSteps;
    morph.nricpAlphaInitial              = params.nricpAlphaInitial;
    morph.nricpAlphaFinal                = params.nricpAlphaFinal;
    morph.nricpIcpIterations             = params.nricpIcpIterations;
    morph.nricpNormalThreshold           = params.nricpNormalThreshold;
    morph.nricpLandmarkWeight            = params.nricpLandmarkWeight;
    morph.nricpEpsilon                   = params.nricpEpsilon;
    morph.nricpEnableBoundaryExclusion   = params.nricpEnableBoundaryExclusion;
    morph.nricpBoundaryExclusionHops     = params.nricpBoundaryExclusionHops;
    morph.nricpOptimizationIterations    = params.nricpOptimizationIterations;
    morph.nricpDpInitial                 = params.nricpDpInitial;
    morph.nricpDpFinal                   = params.nricpDpFinal;
    morph.nricpGammaInitial              = params.nricpGammaInitial;
    morph.nricpGammaFinal                = params.nricpGammaFinal;
    morph.nricpSamplingInitial           = params.nricpSamplingInitial;
    morph.nricpSamplingFinal             = params.nricpSamplingFinal;
    morph.nricpNormalizeSampling         = params.nricpNormalizeSampling;
}

bool WrapPreset::MatchesParamsOf(const MorphData& morph) const {
    // Exact equality — params copied via ApplyTo are bit-identical, so == is
    // the right comparator. If the user drags a slider to the precise preset
    // value the card re-highlights, which matches user intent.
    return morph.algorithm                     == params.algorithm
        && morph.stiffness                     == params.stiffness
        && morph.smoothness                    == params.smoothness
        && morph.nricpStiffnessSteps           == params.nricpStiffnessSteps
        && morph.nricpAlphaInitial             == params.nricpAlphaInitial
        && morph.nricpAlphaFinal               == params.nricpAlphaFinal
        && morph.nricpIcpIterations            == params.nricpIcpIterations
        && morph.nricpNormalThreshold          == params.nricpNormalThreshold
        && morph.nricpLandmarkWeight           == params.nricpLandmarkWeight
        && morph.nricpEpsilon                  == params.nricpEpsilon
        && morph.nricpEnableBoundaryExclusion  == params.nricpEnableBoundaryExclusion
        && morph.nricpBoundaryExclusionHops    == params.nricpBoundaryExclusionHops
        && morph.nricpOptimizationIterations   == params.nricpOptimizationIterations
        && morph.nricpDpInitial                == params.nricpDpInitial
        && morph.nricpDpFinal                  == params.nricpDpFinal
        && morph.nricpGammaInitial             == params.nricpGammaInitial
        && morph.nricpGammaFinal               == params.nricpGammaFinal
        && morph.nricpSamplingInitial          == params.nricpSamplingInitial
        && morph.nricpSamplingFinal            == params.nricpSamplingFinal
        && morph.nricpNormalizeSampling        == params.nricpNormalizeSampling;
}

// Lucide glyph UTF-8 byte literals (mirrors MV_ICON_TARGET / MV_ICON_SPARKLES
// in include/ui/MvIcons.h). Hard-coded here because core/ must not depend on
// ui/ — if Lucide ever renumbers, both files have to update in lockstep.
namespace {

constexpr const char* kIconGlyphTarget   = "\xEE\x86\x80"; // U+E180 — target
constexpr const char* kIconGlyphSparkles = "\xEE\x90\x92"; // U+E412 — sparkles

WrapPreset MakeQuickWrap() {
    WrapPreset preset;
    preset.id          = "quick";
    preset.name        = "Quick Wrap";
    preset.description = "Tight wrap for human-like targets";
    preset.iconGlyph   = kIconGlyphTarget;
    preset.isBuiltIn   = true;
    // Default params (= Fantasy Wrap baseline) plus rigidity (gamma) locked
    // to 1.0 on both ends of the schedule. Higher rigidity preserves the
    // source mesh's shape, which is what indie devs want for realistic
    // human heads. CLAUDE.md and product-vision.md pin these semantics.
    preset.params.nricpGammaInitial = 1.0f;
    preset.params.nricpGammaFinal   = 1.0f;
    return preset;
}

WrapPreset MakeFantasyWrap() {
    WrapPreset preset;
    preset.id          = "fantasy";
    preset.name        = "Fantasy Wrap";
    preset.description = "Looser fit for dramatic features";
    preset.iconGlyph   = kIconGlyphSparkles;
    preset.isBuiltIn   = true;
    // params left at WrapParams{} defaults — the canonical baseline the
    // NRICP pipeline was tuned against. Don't second-guess the design
    // mockups: the Sprint 3.8.9 retro caught a regression where the
    // stage3.jsx values were baked verbatim and visibly broke the wrap.
    return preset;
}

} // namespace

const WrapPreset& WrapPreset::QuickWrap() {
    static const WrapPreset preset = MakeQuickWrap();
    return preset;
}

const WrapPreset& WrapPreset::FantasyWrap() {
    static const WrapPreset preset = MakeFantasyWrap();
    return preset;
}

std::vector<WrapPreset> WrapPreset::BuiltIns() {
    return { QuickWrap(), FantasyWrap() };
}

std::string WrapPreset::FindMatchingBuiltInId(const MorphData& morph) {
    for (const auto& preset : BuiltIns()) {
        if (preset.MatchesParamsOf(morph)) return preset.id;
    }
    return {};
}

} // namespace MetaVisage
