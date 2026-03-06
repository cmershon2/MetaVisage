#ifndef SCULPTBRUSH_H
#define SCULPTBRUSH_H

#include "core/Types.h"
#include "core/Mesh.h"
#include "core/Transform.h"
#include <vector>

namespace MetaVisage {

struct BrushSettings {
    float radius = 0.5f;
    float strength = 0.5f;
    FalloffType falloff = FalloffType::Smooth;
};

struct AffectedVertex {
    int index;
    float weight; // 0.0 to 1.0 based on distance and falloff
};

class SculptBrush {
public:
    SculptBrush();
    virtual ~SculptBrush();

    void SetSettings(const BrushSettings& settings) { settings_ = settings; }
    const BrushSettings& GetSettings() const { return settings_; }

    void SetRadius(float radius) { settings_.radius = radius; }
    void SetStrength(float strength) { settings_.strength = strength; }
    void SetFalloff(FalloffType falloff) { settings_.falloff = falloff; }

    float GetRadius() const { return settings_.radius; }
    float GetStrength() const { return settings_.strength; }
    FalloffType GetFalloff() const { return settings_.falloff; }

    // Calculate falloff weight for a given distance within radius
    float CalculateFalloff(float distance, float radius) const;

    // Find vertices affected by the brush at the given world position
    std::vector<AffectedVertex> GetAffectedVertices(
        const Mesh& mesh, const Transform& transform,
        const Vector3& worldCenter, float radius) const;

    // Apply the brush effect - implemented by subclasses
    // worldCenter: brush position in world space
    // worldNormal: surface normal at brush position in world space
    // mouseDelta: world-space movement delta
    // Returns true if mesh was modified
    virtual bool Apply(Mesh& mesh, const Transform& transform,
                       const Vector3& worldCenter, const Vector3& worldNormal,
                       const Vector3& mouseDelta, float deltaTime) = 0;

    // Called when stroke begins (mouse down)
    virtual void BeginStroke(Mesh& mesh, const Transform& transform,
                             const Vector3& worldCenter);

    // Called when stroke ends (mouse up)
    virtual void EndStroke();

    virtual BrushType GetType() const = 0;

protected:
    BrushSettings settings_;
};

} // namespace MetaVisage

#endif // SCULPTBRUSH_H
