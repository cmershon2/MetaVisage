#include "rendering/GizmoRenderer.h"
#include "utils/Paths.h"
#include <QDebug>
#include <cmath>
#include <algorithm>
#include <limits>

#ifdef HAVE_ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#endif

namespace MetaVisage {

GizmoRenderer::GizmoRenderer() {
}

GizmoRenderer::~GizmoRenderer() {
    auto cleanup = [this](GizmoModel& model) {
        for (auto& sub : model.subMeshes) {
            if (sub.vao) glDeleteVertexArrays(1, &sub.vao);
            if (sub.vbo) glDeleteBuffers(1, &sub.vbo);
            if (sub.ibo) glDeleteBuffers(1, &sub.ibo);
        }
        model.subMeshes.clear();
    };
    cleanup(moveGizmo_);
    cleanup(rotateGizmo_);
    cleanup(scaleGizmo_);
}

bool GizmoRenderer::Initialize() {
    // Resolve gizmo asset paths relative to the executable so installed builds
    // (where CWD is not the project root) load assets correctly.
    const std::string exeDir = Paths::ExecutableDir().toStdString();
    const std::string base = exeDir.empty() ? std::string("assets/models/gizmos")
                                            : exeDir + "/assets/models/gizmos";
    bool ok = true;
    ok &= LoadGizmoModel(base + "/move_gizmo.obj", moveGizmo_);
    ok &= LoadGizmoModel(base + "/rotate_gizmo.obj", rotateGizmo_);
    ok &= LoadGizmoModel(base + "/scale_gizmo.obj", scaleGizmo_);

    if (ok) {
        initialized_ = true;
        qDebug() << "GizmoRenderer: All gizmo models loaded successfully";
    } else {
        qWarning() << "GizmoRenderer: Failed to load one or more gizmo models";
    }

    return ok;
}

bool GizmoRenderer::LoadGizmoModel(const std::string& filepath, GizmoModel& model) {
#ifdef HAVE_ASSIMP
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        filepath,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        qWarning() << "GizmoRenderer: Assimp error loading" << filepath.c_str()
                   << ":" << importer.GetErrorString();
        return false;
    }

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        const aiMesh* aiMesh = scene->mMeshes[i];
        GizmoSubMesh sub;
        sub.name = aiMesh->mName.C_Str();
        sub.axis = NameToAxis(sub.name);

        // Extract vertices and normals
        sub.vertices.resize(aiMesh->mNumVertices);
        std::vector<float> vertexData;
        vertexData.reserve(aiMesh->mNumVertices * 6); // pos(3) + normal(3)

        for (unsigned int v = 0; v < aiMesh->mNumVertices; ++v) {
            const aiVector3D& pos = aiMesh->mVertices[v];
            sub.vertices[v] = Vector3(pos.x, pos.y, pos.z);
            vertexData.push_back(pos.x);
            vertexData.push_back(pos.y);
            vertexData.push_back(pos.z);

            if (aiMesh->HasNormals()) {
                const aiVector3D& norm = aiMesh->mNormals[v];
                vertexData.push_back(norm.x);
                vertexData.push_back(norm.y);
                vertexData.push_back(norm.z);
            } else {
                vertexData.push_back(0.0f);
                vertexData.push_back(1.0f);
                vertexData.push_back(0.0f);
            }
        }

        // Extract indices
        for (unsigned int f = 0; f < aiMesh->mNumFaces; ++f) {
            const aiFace& face = aiMesh->mFaces[f];
            for (unsigned int idx = 0; idx < face.mNumIndices; ++idx) {
                sub.indices.push_back(face.mIndices[idx]);
            }
        }
        sub.indexCount = static_cast<GLsizei>(sub.indices.size());

        // Upload to GPU
        glGenVertexArrays(1, &sub.vao);
        glGenBuffers(1, &sub.vbo);
        glGenBuffers(1, &sub.ibo);

        glBindVertexArray(sub.vao);

        glBindBuffer(GL_ARRAY_BUFFER, sub.vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     vertexData.size() * sizeof(float),
                     vertexData.data(), GL_STATIC_DRAW);

        // Position attribute (location 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Normal attribute (location 1)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sub.ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     sub.indices.size() * sizeof(unsigned int),
                     sub.indices.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);

        model.subMeshes.push_back(std::move(sub));

        qDebug() << "  Loaded gizmo sub-mesh:" << sub.name.c_str()
                 << "verts:" << aiMesh->mNumVertices
                 << "tris:" << sub.indexCount / 3;
    }

    return !model.subMeshes.empty();
#else
    (void)filepath;
    (void)model;
    qWarning() << "GizmoRenderer: Assimp not available, cannot load gizmo models";
    return false;
#endif
}

GizmoAxis GizmoRenderer::NameToAxis(const std::string& name) const {
    if (name == "axis_x") return GizmoAxis::X;
    if (name == "axis_y") return GizmoAxis::Y;
    if (name == "axis_z") return GizmoAxis::Z;
    if (name == "center") return GizmoAxis::Center;
    return GizmoAxis::None;
}

Vector3 GizmoRenderer::GetAxisColor(GizmoAxis axis, GizmoAxis hoveredAxis, GizmoAxis activeAxis) const {
    // Active axis gets a bright white-yellow highlight
    if (activeAxis != GizmoAxis::None && axis == activeAxis) {
        return Vector3(1.0f, 1.0f, 0.4f);
    }

    // Hovered axis gets a lighter highlight
    if (hoveredAxis != GizmoAxis::None && axis == hoveredAxis) {
        return Vector3(1.0f, 0.95f, 0.6f);
    }

    // Default axis colors
    switch (axis) {
        case GizmoAxis::X:      return Vector3(0.9f, 0.2f, 0.2f);   // Red
        case GizmoAxis::Y:      return Vector3(0.2f, 0.8f, 0.2f);   // Green
        case GizmoAxis::Z:      return Vector3(0.3f, 0.5f, 0.9f);   // Blue
        case GizmoAxis::Center: return Vector3(0.85f, 0.85f, 0.85f); // White/gray
        default:                return Vector3(0.5f, 0.5f, 0.5f);
    }
}

Matrix4x4 GizmoRenderer::ComputeGizmoModelMatrix(const Vector3& position,
                                                   const Camera& camera,
                                                   int /*viewportWidth*/,
                                                   int viewportHeight) const {
    // Compute scale so the gizmo is always a fixed screen size
    // regardless of camera distance
    const float gizmoScreenFraction = 0.10f; // 10% of viewport height

    Vector3 camPos = camera.GetPosition();
    float dx = position.x - camPos.x;
    float dy = position.y - camPos.y;
    float dz = position.z - camPos.z;
    float distance = std::sqrt(dx * dx + dy * dy + dz * dz);

    float scale;
    if (camera.GetProjectionMode() == ProjectionMode::Perspective) {
        const float PI = 3.14159265359f;
        float fovRad = camera.GetFOV() * PI / 180.0f;
        float halfHeight = distance * std::tan(fovRad * 0.5f);
        scale = (halfHeight * 2.0f * gizmoScreenFraction) / static_cast<float>(viewportHeight)
                * static_cast<float>(viewportHeight);
        // Simplifies to: halfHeight * 2.0f * gizmoScreenFraction
        scale = halfHeight * 2.0f * gizmoScreenFraction;
    } else {
        // Orthographic: use camera distance as proxy for zoom
        float orthoSize = camera.GetDistance() * 0.5f;
        scale = orthoSize * 2.0f * gizmoScreenFraction;
    }

    // Clamp to a reasonable range
    scale = std::max(0.001f, std::min(scale, 1000.0f));

    // Build model matrix: translate to position, then uniform scale
    Matrix4x4 model = Matrix4x4::Translation(position) * Matrix4x4::Scale(Vector3(scale, scale, scale));
    return model;
}

void GizmoRenderer::Render(TransformMode mode, const Vector3& position,
                            GizmoAxis hoveredAxis, GizmoAxis activeAxis,
                            const Camera& camera, int viewportWidth, int viewportHeight,
                            unsigned int shaderProgram) {
    if (!initialized_ || mode == TransformMode::None) return;

    const GizmoModel* gizmo = nullptr;
    switch (mode) {
        case TransformMode::Move:   gizmo = &moveGizmo_; break;
        case TransformMode::Rotate: gizmo = &rotateGizmo_; break;
        case TransformMode::Scale:  gizmo = &scaleGizmo_; break;
        default: return;
    }

    Matrix4x4 modelMatrix = ComputeGizmoModelMatrix(position, camera, viewportWidth, viewportHeight);

    // Compute normal matrix (inverse transpose of upper-left 3x3)
    Matrix4x4 invModel = modelMatrix.Inverse();
    Matrix4x4 normalMat4 = invModel.Transpose();
    float normalMatrix[9] = {
        normalMat4.m[0], normalMat4.m[1], normalMat4.m[2],
        normalMat4.m[4], normalMat4.m[5], normalMat4.m[6],
        normalMat4.m[8], normalMat4.m[9], normalMat4.m[10]
    };

    glUseProgram(shaderProgram);

    int modelLoc = glGetUniformLocation(shaderProgram, "uModel");
    int vpLoc = glGetUniformLocation(shaderProgram, "uViewProjection");
    int normalLoc = glGetUniformLocation(shaderProgram, "uNormalMatrix");
    int colorLoc = glGetUniformLocation(shaderProgram, "uColor");
    int viewPosLoc = glGetUniformLocation(shaderProgram, "uViewPos");

    float aspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
    Matrix4x4 view = camera.GetViewMatrix();
    Matrix4x4 proj = camera.GetProjectionMatrix(aspectRatio);
    Matrix4x4 vp = proj * view;

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, modelMatrix.Data());
    glUniformMatrix4fv(vpLoc, 1, GL_FALSE, vp.Data());
    glUniformMatrix3fv(normalLoc, 1, GL_FALSE, normalMatrix);

    Vector3 viewPos = camera.GetPosition();
    glUniform3f(viewPosLoc, viewPos.x, viewPos.y, viewPos.z);

    // Render on top of scene geometry
    glClear(GL_DEPTH_BUFFER_BIT);

    for (const auto& sub : gizmo->subMeshes) {
        if (sub.vao == 0 || sub.indexCount == 0) continue;

        Vector3 color = GetAxisColor(sub.axis, hoveredAxis, activeAxis);
        glUniform3f(colorLoc, color.x, color.y, color.z);

        glBindVertexArray(sub.vao);
        glDrawElements(GL_TRIANGLES, sub.indexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }
}

GizmoAxis GizmoRenderer::HitTest(TransformMode mode, const Vector3& gizmoPosition,
                                  const Ray& ray, const Camera& camera,
                                  int viewportWidth, int viewportHeight) const {
    if (!initialized_ || mode == TransformMode::None) return GizmoAxis::None;

    const GizmoModel* gizmo = nullptr;
    switch (mode) {
        case TransformMode::Move:   gizmo = &moveGizmo_; break;
        case TransformMode::Rotate: gizmo = &rotateGizmo_; break;
        case TransformMode::Scale:  gizmo = &scaleGizmo_; break;
        default: return GizmoAxis::None;
    }

    Matrix4x4 modelMatrix = ComputeGizmoModelMatrix(gizmoPosition, camera, viewportWidth, viewportHeight);

    GizmoAxis closestAxis = GizmoAxis::None;
    float closestT = std::numeric_limits<float>::max();

    for (const auto& sub : gizmo->subMeshes) {
        if (sub.indices.empty() || sub.vertices.empty()) continue;

        // Test ray against each triangle of this sub-mesh (in world space)
        for (size_t i = 0; i + 2 < sub.indices.size(); i += 3) {
            // Transform vertices from model space to world space
            auto transformVert = [&](const Vector3& v) -> Vector3 {
                float wx = modelMatrix.m[0]*v.x + modelMatrix.m[4]*v.y + modelMatrix.m[8]*v.z + modelMatrix.m[12];
                float wy = modelMatrix.m[1]*v.x + modelMatrix.m[5]*v.y + modelMatrix.m[9]*v.z + modelMatrix.m[13];
                float wz = modelMatrix.m[2]*v.x + modelMatrix.m[6]*v.y + modelMatrix.m[10]*v.z + modelMatrix.m[14];
                return Vector3(wx, wy, wz);
            };

            Vector3 v0 = transformVert(sub.vertices[sub.indices[i]]);
            Vector3 v1 = transformVert(sub.vertices[sub.indices[i + 1]]);
            Vector3 v2 = transformVert(sub.vertices[sub.indices[i + 2]]);

            // Moller-Trumbore ray-triangle intersection
            Vector3 edge1 = v1 - v0;
            Vector3 edge2 = v2 - v0;
            Vector3 h = ray.direction.Cross(edge2);
            float a = edge1.Dot(h);

            if (std::abs(a) < 1e-7f) continue;

            float f = 1.0f / a;
            Vector3 s = ray.origin - v0;
            float u = f * s.Dot(h);
            if (u < 0.0f || u > 1.0f) continue;

            Vector3 q = s.Cross(edge1);
            float v = f * ray.direction.Dot(q);
            if (v < 0.0f || u + v > 1.0f) continue;

            float t = f * edge2.Dot(q);
            if (t > 0.001f && t < closestT) {
                closestT = t;
                closestAxis = sub.axis;
            }
        }
    }

    return closestAxis;
}

} // namespace MetaVisage
