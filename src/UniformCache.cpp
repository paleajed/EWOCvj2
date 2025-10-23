#include "UniformCache.h"
#include <iostream>

UniformCache::UniformCache(GLuint program) : shaderProgram(program) {
}

UniformCache::~UniformCache() {
    clearCache();
}

void UniformCache::setProgram(GLuint program) {
    if (shaderProgram != program) {
        shaderProgram = program;
        clearCache();
    }
}

void UniformCache::clearCache() {
    locationCache.clear();
}

GLint UniformCache::getUniformLocation(const std::string& name) {
    auto it = locationCache.find(name);
    if (it != locationCache.end()) {
        return it->second;
    }
    
    GLint location = glGetUniformLocation(shaderProgram, name.c_str());
    locationCache[name] = location;
    
    if (location == -1) {
        if (name == "" ) {
            bool dummy = false;
        }
        std::cerr << "Warning: Uniform '" << name << "' not found in shader program" << std::endl;
    }
    
    return location;
}

// Float uniforms
void UniformCache::setFloat(const std::string& name, GLfloat value) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniform1f(location, value);
    }
}

void UniformCache::setFloat2(const std::string& name, GLfloat x, GLfloat y) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniform2f(location, x, y);
    }
}

void UniformCache::setFloat3(const std::string& name, GLfloat x, GLfloat y, GLfloat z) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniform3f(location, x, y, z);
    }
}

void UniformCache::setFloat4(const std::string& name, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniform4f(location, x, y, z, w);
    }
}

void UniformCache::setFloat4v(const std::string& name, const GLfloat* values) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniform4fv(location, 1, values);
    }
}

// Integer uniforms
void UniformCache::setInt(const std::string& name, GLint value) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniform1i(location, value);
    }
}

void UniformCache::setInt2(const std::string& name, GLint x, GLint y) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniform2i(location, x, y);
    }
}

void UniformCache::setInt3(const std::string& name, GLint x, GLint y, GLint z) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniform3i(location, x, y, z);
    }
}

void UniformCache::setInt4(const std::string& name, GLint x, GLint y, GLint z, GLint w) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniform4i(location, x, y, z, w);
    }
}

// Boolean uniforms (using int)
void UniformCache::setBool(const std::string& name, bool value) {
    setInt(name, value ? 1 : 0);
}

// Matrix uniforms
void UniformCache::setMatrix3(const std::string& name, const GLfloat* matrix) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniformMatrix3fv(location, 1, GL_FALSE, matrix);
    }
}

void UniformCache::setMatrix4(const std::string& name, const GLfloat* matrix) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, matrix);
    }
}

// Sampler uniforms
void UniformCache::setSampler(const std::string& name, GLint textureUnit) {
    setInt(name, textureUnit);
}

// Array uniforms
void UniformCache::setFloat1v(const std::string& name, GLsizei count, const GLfloat* values) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniform1fv(location, count, values);
    }
}

void UniformCache::setInt1v(const std::string& name, GLsizei count, const GLint* values) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniform1iv(location, count, values);
    }
}

void UniformCache::setSamplerArray(const std::string& name, const GLint* textureUnits, GLsizei count) {
    GLint location = getUniformLocation(name);
    if (location != -1) {
        glUniform1iv(location, count, textureUnits);
    }
}