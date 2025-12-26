#ifndef SHADERMANAGER_H
#define SHADERMANAGER_H

#include <QOpenGLFunctions_4_3_Core>
#include <QString>
#include <map>

namespace MetaVisage {

class ShaderManager : protected QOpenGLFunctions_4_3_Core {
public:
    ShaderManager();
    ~ShaderManager();

    bool LoadShader(const QString& name, const QString& vertexPath, const QString& fragmentPath);
    unsigned int GetShader(const QString& name) const;
    void UseShader(const QString& name);

private:
    unsigned int CompileShader(unsigned int type, const QString& source);
    unsigned int CreateProgram(const QString& vertexSource, const QString& fragmentSource);

    std::map<QString, unsigned int> shaders_;
};

} // namespace MetaVisage

#endif // SHADERMANAGER_H
