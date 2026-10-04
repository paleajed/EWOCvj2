#pragma once

#ifdef USE_GLES
#include <GLES3/gl3.h>
#else
#include "GL/glew.h"
#include "GL/gl.h"
#endif
#include <unordered_map>
#include <string>
#include <string_view>
#include <cstdint>

class UniformCache {
private:
    GLuint shaderProgram;

    // Transparent hashing so lookups with a literal / string_view don't construct a std::string
    struct SvHash {
        using is_transparent = void;
        size_t operator()(std::string_view sv) const { return std::hash<std::string_view>{}(sv); }
        size_t operator()(const std::string& s) const { return std::hash<std::string_view>{}(s); }
        size_t operator()(const char* s) const { return std::hash<std::string_view>{}(s); }
    };

    // Location plus the last value sent (bit pattern), so identical re-sets can be skipped.
    // Only valid as long as every write to this program goes through the cache.
    struct Entry {
        GLint location = -1;
        bool hasValue = false;
        uint8_t kind = 0;           // 1 = float, 2 = int; includes component count in bits 4-7
        uint32_t value[4] = {0, 0, 0, 0};
    };
    std::unordered_map<std::string, Entry, SvHash, std::equal_to<>> locationCache;

    Entry& getEntry(std::string_view name);
    // returns true if the value differs from what was last sent (and records it)
    bool changed(Entry& e, uint8_t kind, const uint32_t* v, int n);

public:
    UniformCache(GLuint program);
    ~UniformCache();

    void setProgram(GLuint program);
    void clearCache();
    // Forget the remembered values but keep locations (call if something wrote uniforms behind the cache's back)
    void invalidateValues();

    // Float uniforms
    void setFloat(std::string_view name, GLfloat value);
    void setFloat2(std::string_view name, GLfloat x, GLfloat y);
    void setFloat3(std::string_view name, GLfloat x, GLfloat y, GLfloat z);
    void setFloat4(std::string_view name, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void setFloat4v(std::string_view name, const GLfloat* values);

    // Integer uniforms
    void setInt(std::string_view name, GLint value);
    void setInt2(std::string_view name, GLint x, GLint y);
    void setInt3(std::string_view name, GLint x, GLint y, GLint z);
    void setInt4(std::string_view name, GLint x, GLint y, GLint z, GLint w);

    // Boolean uniforms (using int)
    void setBool(std::string_view name, bool value);

    // Matrix uniforms
    void setMatrix3(std::string_view name, const GLfloat* matrix);
    void setMatrix4(std::string_view name, const GLfloat* matrix);

    // Sampler uniforms
    void setSampler(std::string_view name, GLint textureUnit);

    // Array uniforms
    void setFloat1v(std::string_view name, GLsizei count, const GLfloat* values);
    void setInt1v(std::string_view name, GLsizei count, const GLint* values);
    void setSamplerArray(std::string_view name, const GLint* textureUnits, GLsizei count);
};
