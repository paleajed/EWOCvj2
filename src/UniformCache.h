#pragma once

#include "GL/glew.h"
#include "GL/gl.h"
#include <unordered_map>
#include <string>

class UniformCache {
private:
    GLuint shaderProgram;
    std::unordered_map<std::string, GLint> locationCache;
    
    GLint getUniformLocation(const std::string& name);
    
public:
    UniformCache(GLuint program);
    ~UniformCache();
    
    void setProgram(GLuint program);
    void clearCache();
    
    // Float uniforms
    void setFloat(const std::string& name, GLfloat value);
    void setFloat2(const std::string& name, GLfloat x, GLfloat y);
    void setFloat3(const std::string& name, GLfloat x, GLfloat y, GLfloat z);
    void setFloat4(const std::string& name, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void setFloat4v(const std::string& name, const GLfloat* values);
    
    // Integer uniforms
    void setInt(const std::string& name, GLint value);
    void setInt2(const std::string& name, GLint x, GLint y);
    void setInt3(const std::string& name, GLint x, GLint y, GLint z);
    void setInt4(const std::string& name, GLint x, GLint y, GLint z, GLint w);
    
    // Boolean uniforms (using int)
    void setBool(const std::string& name, bool value);
    
    // Matrix uniforms
    void setMatrix3(const std::string& name, const GLfloat* matrix);
    void setMatrix4(const std::string& name, const GLfloat* matrix);
    
    // Sampler uniforms
    void setSampler(const std::string& name, GLint textureUnit);
    
    // Array uniforms
    void setFloat1v(const std::string& name, GLsizei count, const GLfloat* values);
    void setInt1v(const std::string& name, GLsizei count, const GLint* values);
    void setSamplerArray(const std::string& name, const GLint* textureUnits, GLsizei count);
};