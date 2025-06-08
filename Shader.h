#ifndef AA5DADE4_03A9_4920_A259_04AC8699AB9C
#define AA5DADE4_03A9_4920_A259_04AC8699AB9C
#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h>    
#include <GLFW/glfw3.h> 
#include <string>

class Shader{
public:
    Shader(const char* vertexPath, const char* fragmentPath);
    std::string vertexString;
    std::string fragmentString;
    const char* vertexSource;
    const char* fragmentSource;
    unsigned int ID;
    void use();
private:
    void checkCompileErrors(unsigned int ID, std::string type);
};

#endif


#endif /* AA5DADE4_03A9_4920_A259_04AC8699AB9C */
