#ifndef SHADERMANAGER_H
#define SHADERMANAGER_H

#include <GL/glew.h>
#include <map>
#include <string>

namespace MetaVisage {

class ShaderManager {
public:
    ShaderManager();
    ~ShaderManager();

    bool LoadShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath);
    unsigned int GetShader(const std::string& name) const;
    void UseShader(const std::string& name);

private:
    unsigned int CompileShader(unsigned int type, const std::string& source);
    unsigned int CreateProgram(const std::string& vertexSource, const std::string& fragmentSource);

    std::map<std::string, unsigned int> shaders_;
};

} // namespace MetaVisage

#endif // SHADERMANAGER_H
