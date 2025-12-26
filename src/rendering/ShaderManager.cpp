#include "rendering/ShaderManager.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

namespace MetaVisage {

ShaderManager::ShaderManager() {
    initializeOpenGLFunctions();
}

ShaderManager::~ShaderManager() {
    for (auto& pair : shaders_) {
        glDeleteProgram(pair.second);
    }
}

bool ShaderManager::LoadShader(const QString& name, const QString& vertexPath, const QString& fragmentPath) {
    // TODO: Implement shader loading in Sprint 2
    return false;
}

unsigned int ShaderManager::GetShader(const QString& name) const {
    auto it = shaders_.find(name);
    if (it != shaders_.end()) {
        return it->second;
    }
    return 0;
}

void ShaderManager::UseShader(const QString& name) {
    unsigned int program = GetShader(name);
    if (program != 0) {
        glUseProgram(program);
    }
}

unsigned int ShaderManager::CompileShader(unsigned int type, const QString& source) {
    // TODO: Implement shader compilation in Sprint 2
    return 0;
}

unsigned int ShaderManager::CreateProgram(const QString& vertexSource, const QString& fragmentSource) {
    // TODO: Implement program linking in Sprint 2
    return 0;
}

} // namespace MetaVisage
