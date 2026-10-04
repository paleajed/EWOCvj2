#include "UniformCache.h"
#include <iostream>
#include <cstring>

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

void UniformCache::invalidateValues() {
    for (auto &kv : locationCache) kv.second.hasValue = false;
}

UniformCache::Entry& UniformCache::getEntry(std::string_view name) {
    auto it = locationCache.find(name);
    if (it != locationCache.end()) {
        return it->second;
    }

    std::string key(name);
    Entry e;
    e.location = glGetUniformLocation(shaderProgram, key.c_str());

    if (e.location == -1) {
        std::cerr << "Warning: Uniform '" << key << "' not found in shader program" << std::endl;
    }

    return locationCache.emplace(std::move(key), e).first->second;
}

bool UniformCache::changed(Entry& e, uint8_t kind, const uint32_t* v, int n) {
    uint8_t k = (uint8_t)(kind | (n << 4));
    if (e.hasValue && e.kind == k && std::memcmp(e.value, v, n * sizeof(uint32_t)) == 0) {
        return false;
    }
    e.hasValue = true;
    e.kind = k;
    std::memcpy(e.value, v, n * sizeof(uint32_t));
    return true;
}

static inline uint32_t fbits(GLfloat f) {
    uint32_t u;
    std::memcpy(&u, &f, sizeof(u));
    return u;
}

// Float uniforms
void UniformCache::setFloat(std::string_view name, GLfloat value) {
    Entry& e = getEntry(name);
    if (e.location == -1) return;
    uint32_t v[1] = {fbits(value)};
    if (changed(e, 1, v, 1)) glUniform1f(e.location, value);
}

void UniformCache::setFloat2(std::string_view name, GLfloat x, GLfloat y) {
    Entry& e = getEntry(name);
    if (e.location == -1) return;
    uint32_t v[2] = {fbits(x), fbits(y)};
    if (changed(e, 1, v, 2)) glUniform2f(e.location, x, y);
}

void UniformCache::setFloat3(std::string_view name, GLfloat x, GLfloat y, GLfloat z) {
    Entry& e = getEntry(name);
    if (e.location == -1) return;
    uint32_t v[3] = {fbits(x), fbits(y), fbits(z)};
    if (changed(e, 1, v, 3)) glUniform3f(e.location, x, y, z);
}

void UniformCache::setFloat4(std::string_view name, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    Entry& e = getEntry(name);
    if (e.location == -1) return;
    uint32_t v[4] = {fbits(x), fbits(y), fbits(z), fbits(w)};
    if (changed(e, 1, v, 4)) glUniform4f(e.location, x, y, z, w);
}

void UniformCache::setFloat4v(std::string_view name, const GLfloat* values) {
    Entry& e = getEntry(name);
    if (e.location == -1) return;
    uint32_t v[4] = {fbits(values[0]), fbits(values[1]), fbits(values[2]), fbits(values[3])};
    if (changed(e, 1, v, 4)) glUniform4fv(e.location, 1, values);
}

// Integer uniforms
void UniformCache::setInt(std::string_view name, GLint value) {
    Entry& e = getEntry(name);
    if (e.location == -1) return;
    uint32_t v[1] = {(uint32_t)value};
    if (changed(e, 2, v, 1)) glUniform1i(e.location, value);
}

void UniformCache::setInt2(std::string_view name, GLint x, GLint y) {
    Entry& e = getEntry(name);
    if (e.location == -1) return;
    uint32_t v[2] = {(uint32_t)x, (uint32_t)y};
    if (changed(e, 2, v, 2)) glUniform2i(e.location, x, y);
}

void UniformCache::setInt3(std::string_view name, GLint x, GLint y, GLint z) {
    Entry& e = getEntry(name);
    if (e.location == -1) return;
    uint32_t v[3] = {(uint32_t)x, (uint32_t)y, (uint32_t)z};
    if (changed(e, 2, v, 3)) glUniform3i(e.location, x, y, z);
}

void UniformCache::setInt4(std::string_view name, GLint x, GLint y, GLint z, GLint w) {
    Entry& e = getEntry(name);
    if (e.location == -1) return;
    uint32_t v[4] = {(uint32_t)x, (uint32_t)y, (uint32_t)z, (uint32_t)w};
    if (changed(e, 2, v, 4)) glUniform4i(e.location, x, y, z, w);
}

// Boolean uniforms (using int)
void UniformCache::setBool(std::string_view name, bool value) {
    setInt(name, value ? 1 : 0);
}

// Matrix uniforms (not value-cached)
void UniformCache::setMatrix3(std::string_view name, const GLfloat* matrix) {
    Entry& e = getEntry(name);
    if (e.location != -1) {
        e.hasValue = false;
        glUniformMatrix3fv(e.location, 1, GL_FALSE, matrix);
    }
}

void UniformCache::setMatrix4(std::string_view name, const GLfloat* matrix) {
    Entry& e = getEntry(name);
    if (e.location != -1) {
        e.hasValue = false;
        glUniformMatrix4fv(e.location, 1, GL_FALSE, matrix);
    }
}

// Sampler uniforms
void UniformCache::setSampler(std::string_view name, GLint textureUnit) {
    setInt(name, textureUnit);
}

// Array uniforms (not value-cached)
void UniformCache::setFloat1v(std::string_view name, GLsizei count, const GLfloat* values) {
    Entry& e = getEntry(name);
    if (e.location != -1) {
        e.hasValue = false;
        glUniform1fv(e.location, count, values);
    }
}

void UniformCache::setInt1v(std::string_view name, GLsizei count, const GLint* values) {
    Entry& e = getEntry(name);
    if (e.location != -1) {
        e.hasValue = false;
        glUniform1iv(e.location, count, values);
    }
}

void UniformCache::setSamplerArray(std::string_view name, const GLint* textureUnits, GLsizei count) {
    Entry& e = getEntry(name);
    if (e.location != -1) {
        e.hasValue = false;
        glUniform1iv(e.location, count, textureUnits);
    }
}
