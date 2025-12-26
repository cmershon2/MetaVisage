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
    QFile vertFile(vertexPath);
    if (!vertFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open vertex shader:" << vertexPath;
        return false;
    }
    QTextStream vertStream(&vertFile);
    QString vertexSource = vertStream.readAll();
    vertFile.close();

    QFile fragFile(fragmentPath);
    if (!fragFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open fragment shader:" << fragmentPath;
        return false;
    }
    QTextStream fragStream(&fragFile);
    QString fragmentSource = fragStream.readAll();
    fragFile.close();

    unsigned int program = CreateProgram(vertexSource, fragmentSource);
    if (program == 0) {
        qWarning() << "Failed to create shader program:" << name;
        return false;
    }

    shaders_[name] = program;
    qDebug() << "Successfully loaded shader:" << name;
    return true;
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
    unsigned int shader = glCreateShader(type);
    const char* src = source.toUtf8().constData();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        qWarning() << "Shader compilation failed:" << infoLog;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

unsigned int ShaderManager::CreateProgram(const QString& vertexSource, const QString& fragmentSource) {
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
        qWarning() << "Shader program linking failed:" << infoLog;
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
