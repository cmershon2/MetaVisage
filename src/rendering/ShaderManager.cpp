#include "rendering/ShaderManager.h"
#include "utils/Logger.h"

#include <format>
#include <fstream>
#include <sstream>

namespace MetaVisage {

namespace {

bool ReadFile(const std::string& path, std::string& out) {
    std::ifstream in(path);
    if (!in.is_open()) return false;
    std::stringstream buf;
    buf << in.rdbuf();
    out = buf.str();
    return true;
}

} // namespace

ShaderManager::ShaderManager() {
}

ShaderManager::~ShaderManager() {
    for (auto& pair : shaders_) {
        glDeleteProgram(pair.second);
    }
}

bool ShaderManager::LoadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexSource;
    if (!ReadFile(vertexPath, vertexSource)) {
        MV_LOG_WARNING(std::format("Failed to open vertex shader: {}", vertexPath));
        return false;
    }

    std::string fragmentSource;
    if (!ReadFile(fragmentPath, fragmentSource)) {
        MV_LOG_WARNING(std::format("Failed to open fragment shader: {}", fragmentPath));
        return false;
    }

    unsigned int program = CreateProgram(vertexSource, fragmentSource);
    if (program == 0) {
        MV_LOG_WARNING(std::format("Failed to create shader program: {}", name));
        return false;
    }

    shaders_[name] = program;
    MV_LOG_DEBUG(std::format("Successfully loaded shader: {}", name));
    return true;
}

unsigned int ShaderManager::GetShader(const std::string& name) const {
    auto it = shaders_.find(name);
    if (it != shaders_.end()) {
        return it->second;
    }
    return 0;
}

void ShaderManager::UseShader(const std::string& name) {
    unsigned int program = GetShader(name);
    if (program != 0) {
        glUseProgram(program);
    }
}

unsigned int ShaderManager::CompileShader(unsigned int type, const std::string& source) {
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        MV_LOG_WARNING(std::format("Shader compilation failed: {}", infoLog));
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

unsigned int ShaderManager::CreateProgram(const std::string& vertexSource, const std::string& fragmentSource) {
    unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
    if (vertexShader == 0) return 0;

    unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return 0;
    }

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        MV_LOG_WARNING(std::format("Shader program linking failed: {}", infoLog));
        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

} // namespace MetaVisage
