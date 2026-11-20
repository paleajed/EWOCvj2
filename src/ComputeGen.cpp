// ComputeGen.cpp - compute-shader generators (see ComputeGen.h)
#ifndef USE_GLES

#include "ComputeGen.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cctype>
#include <functional>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------------------------
// GLSL helpers available to every generator (after the host prelude, before the generator code)
// ---------------------------------------------------------------------------------------------
static const char* CG_COMMON = R"GLSL(
uint cg_hash(uint x) {
    x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16;
    return x;
}
float cg_rand(uint x) { return float(cg_hash(x)) * (1.0 / 4294967295.0); }
vec3 cg_hsv2rgb(vec3 c) {
    vec3 p = abs(fract(c.xxx + vec3(0.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0);
    return c.z * mix(vec3(1.0), clamp(p - 1.0, 0.0, 1.0), c.y);
}
// rotate the hue of an RGB colour (angle in degrees) via YIQ
vec3 cg_hue(vec3 c, float degrees) {
    const mat3 toYIQ = mat3(0.299, 0.596, 0.211, 0.587, -0.274, -0.523, 0.114, -0.322, 0.312);
    const mat3 fromYIQ = mat3(1.0, 1.0, 1.0, 0.956, -0.272, -1.106, 0.621, -0.647, 1.703);
    vec3 yiq = toYIQ * c;
    float a = radians(degrees), cs = cos(a), sn = sin(a);
    yiq.yz = vec2(yiq.y * cs - yiq.z * sn, yiq.y * sn + yiq.z * cs);
    return clamp(fromYIQ * yiq, 0.0, 1.0);
}
// 8 palettes of 5 colour stops; index 6 is a rainbow, 7 is greyscale
const vec3 CG_PAL[40] = vec3[40](
    vec3(0.0, 0.0, 0.02), vec3(0.34, 0.06, 0.43), vec3(0.74, 0.22, 0.33), vec3(0.97, 0.55, 0.04), vec3(0.99, 1.0, 0.65),
    vec3(0.0, 0.01, 0.05), vec3(0.0, 0.15, 0.4), vec3(0.0, 0.5, 0.7), vec3(0.4, 0.9, 0.9), vec3(1.0, 1.0, 1.0),
    vec3(0.0, 0.02, 0.0), vec3(0.0, 0.25, 0.1), vec3(0.2, 0.7, 0.1), vec3(0.8, 0.95, 0.2), vec3(1.0, 1.0, 0.85),
    vec3(0.0, 0.0, 0.0), vec3(0.2, 0.0, 0.3), vec3(0.7, 0.1, 0.6), vec3(1.0, 0.5, 0.7), vec3(1.0, 0.95, 0.95),
    vec3(0.0, 0.0, 0.03), vec3(0.05, 0.1, 0.4), vec3(0.3, 0.5, 0.8), vec3(0.75, 0.9, 1.0), vec3(1.0, 1.0, 1.0),
    vec3(0.02, 0.01, 0.0), vec3(0.3, 0.1, 0.0), vec3(0.7, 0.4, 0.05), vec3(1.0, 0.8, 0.3), vec3(1.0, 1.0, 0.9),
    vec3(0.0), vec3(0.0), vec3(0.0), vec3(0.0), vec3(0.0),
    vec3(0.0), vec3(0.0), vec3(0.0), vec3(0.0), vec3(0.0));
vec3 cg_palette(int idx, float t) {
    t = clamp(t, 0.0, 1.0);
    if (idx == 6) return cg_hsv2rgb(vec3(fract(t * 0.8), 0.85, smoothstep(0.0, 0.12, t)));
    if (idx == 7) return vec3(t);
    idx = clamp(idx, 0, 5);
    float f = t * 4.0;
    int i = min(int(f), 3);
    return mix(CG_PAL[idx * 5 + i], CG_PAL[idx * 5 + i + 1], f - float(i));
}
)GLSL";

// ---------------------------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------------------------
static std::string upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::toupper(c); });
    return s;
}

static bool format_info(const std::string& f, GLenum& internal, bool& isUint) {
    isUint = false;
    if (f == "r32f") internal = GL_R32F;
    else if (f == "rg32f") internal = GL_RG32F;
    else if (f == "rgba32f") internal = GL_RGBA32F;
    else if (f == "r16f") internal = GL_R16F;
    else if (f == "rg16f") internal = GL_RG16F;
    else if (f == "rgba16f") internal = GL_RGBA16F;
    else if (f == "rgba8") internal = GL_RGBA8;
    else if (f == "r32ui") { internal = GL_R32UI; isUint = true; }
    else return false;
    return true;
}

static std::string json_expr(const json& j, const std::string& dflt) {
    if (j.is_string()) return j.get<std::string>();
    if (j.is_number()) return std::to_string(j.get<double>());
    return dflt;
}

static ISFLoader::ParamType string_to_param_type(const std::string& t, bool& ok) {
    ok = true;
    if (t == "float") return ISFLoader::PARAM_FLOAT;
    if (t == "long") return ISFLoader::PARAM_LONG;
    if (t == "bool") return ISFLoader::PARAM_BOOL;
    if (t == "event") return ISFLoader::PARAM_EVENT;
    if (t == "color") return ISFLoader::PARAM_COLOR;
    if (t == "point2D") return ISFLoader::PARAM_POINT2D;
    ok = false;
    return ISFLoader::PARAM_FLOAT;
}

// ---------------------------------------------------------------------------------------------
// ComputeGenLoader
// ---------------------------------------------------------------------------------------------
ComputeGenLoader::~ComputeGenLoader() {}

bool ComputeGenLoader::supported() {
    static int cached = -1;
    if (cached != -1) return cached == 1;
    cached = 0;
    if (!GLEW_VERSION_4_3) {
        std::cerr << "[ComputeGen] OpenGL 4.3 not available - compute generators disabled" << std::endl;
        return false;
    }
    GLint inv = 0;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &inv);
    if (inv < 256) {
        std::cerr << "[ComputeGen] max compute work group invocations " << inv << " < 256 - disabled" << std::endl;
        return false;
    }
    cached = 1;
    return true;
}

int ComputeGenLoader::loadDirectory(const std::string& dir) {
    if (!supported()) return 0;
    std::error_code ec;
    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) return 0;
    int before = (int)shaders_.size();
    std::vector<fs::path> files;
    for (auto it = fs::recursive_directory_iterator(dir, ec); !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (it->is_regular_file(ec) && it->path().extension() == ".comp") files.push_back(it->path());
    }
    std::sort(files.begin(), files.end());
    for (auto& p : files) loadFile(p.string());
    int added = (int)shaders_.size() - before;
    std::cout << "[ComputeGen] loaded " << added << " generator(s) from " << dir << std::endl;
    return added;
}

bool ComputeGenLoader::loadFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "[ComputeGen] cannot open " << path << std::endl;
        return false;
    }
    std::stringstream ss;
    ss << in.rdbuf();
    auto shader = std::make_unique<ComputeGenShader>();
    shader->name_ = fs::path(path).stem().string();
    if (findShader(shader->name_)) {
        std::cerr << "[ComputeGen] duplicate generator name " << shader->name_ << " (" << path << ") skipped" << std::endl;
        return false;
    }
    if (!shader->parse(ss.str())) {
        std::cerr << "[ComputeGen] " << path << ": header parse failed" << std::endl;
        return false;
    }
    if (!shader->compile()) {
        std::cerr << "[ComputeGen] " << path << ": compile failed" << std::endl;
        return false;
    }
    shaders_.push_back(std::move(shader));
    return true;
}

ComputeGenShader* ComputeGenLoader::getShader(int i) const {
    if (i < 0 || i >= (int)shaders_.size()) return nullptr;
    return shaders_[i].get();
}

int ComputeGenLoader::findIndex(const std::string& name) const {
    std::string u = upper(name);
    for (size_t i = 0; i < shaders_.size(); i++) {
        if (upper(shaders_[i]->name_) == u) return (int)i;
    }
    return -1;
}

ComputeGenShader* ComputeGenLoader::findShader(const std::string& name) const {
    return getShader(findIndex(name));
}

// ---------------------------------------------------------------------------------------------
// ComputeGenShader
// ---------------------------------------------------------------------------------------------
ComputeGenShader::~ComputeGenShader() {
    for (auto& p : passes_) {
        if (p.program) glDeleteProgram(p.program);
    }
}

std::shared_ptr<ComputeGenInstance> ComputeGenShader::createInstance() {
    return std::make_shared<ComputeGenInstance>(this);
}

bool ComputeGenShader::parse(const std::string& content) {
    size_t cs = content.find("/*");
    size_t ce = (cs == std::string::npos) ? std::string::npos : content.find("*/", cs);
    if (cs == std::string::npos || ce == std::string::npos) {
        std::cerr << "[ComputeGen] " << name_ << ": no /* { json } */ header" << std::endl;
        return false;
    }
    std::string jsonText = content.substr(cs + 2, ce - cs - 2);
    source_ = content.substr(ce + 2);

    json root;
    try {
        root = json::parse(jsonText);

        description_ = root.value("DESCRIPTION", std::string());

        // INPUTS -> ISFLoader::ParamInfo (same syntax as ISF so Param::isfset_parameter_to can be reused)
        if (root.contains("INPUTS") && root["INPUTS"].is_array()) {
            for (const auto& in : root["INPUTS"]) {
                if (!in.contains("TYPE") || !in.contains("NAME")) continue;
                bool ok;
                ISFLoader::ParamInfo p;
                p.name = in["NAME"].get<std::string>();
                p.label = in.value("LABEL", p.name);
                p.type = string_to_param_type(in["TYPE"].get<std::string>(), ok);
                if (!ok) {
                    std::cerr << "[ComputeGen] " << name_ << ": unknown input type for " << p.name << std::endl;
                    continue;
                }
                switch (p.type) {
                    case ISFLoader::PARAM_FLOAT:
                        p.minVal = in.value("MIN", 0.0f);
                        p.maxVal = in.value("MAX", 1.0f);
                        p.defaultVal = in.value("DEFAULT", 0.0f);
                        break;
                    case ISFLoader::PARAM_LONG:
                        p.minInt = in.value("MIN", 0);
                        p.maxInt = in.value("MAX", 100);
                        p.defaultInt = in.value("DEFAULT", 0);
                        if (in.contains("VALUES") && in["VALUES"].is_array()) {
                            p.hasDiscreteValues = true;
                            for (const auto& v : in["VALUES"]) {
                                if (v.is_number_integer()) p.values.push_back(v.get<int>());
                            }
                            if (in.contains("LABELS") && in["LABELS"].is_array()) {
                                for (const auto& l : in["LABELS"]) {
                                    if (l.is_string()) p.labels.push_back(l.get<std::string>());
                                }
                            }
                            while (p.labels.size() < p.values.size()) p.labels.push_back(std::to_string(p.values[p.labels.size()]));
                            p.labels.resize(p.values.size());
                            if (!p.isValidValue(p.defaultInt) && !p.values.empty()) p.defaultInt = p.values[0];
                        } else {
                            // no VALUES: offer every integer in MIN..MAX
                            p.hasDiscreteValues = true;
                            for (int v = p.minInt; v <= p.maxInt && v - p.minInt < 256; v++) {
                                p.values.push_back(v);
                                p.labels.push_back(std::to_string(v));
                            }
                        }
                        break;
                    case ISFLoader::PARAM_BOOL:
                    case ISFLoader::PARAM_EVENT:
                        if (in.contains("DEFAULT")) {
                            if (in["DEFAULT"].is_boolean()) p.defaultInt = in["DEFAULT"].get<bool>() ? 1 : 0;
                            else if (in["DEFAULT"].is_number()) p.defaultInt = in["DEFAULT"].get<int>() != 0 ? 1 : 0;
                        }
                        break;
                    case ISFLoader::PARAM_COLOR:
                        if (in.contains("DEFAULT") && in["DEFAULT"].is_array()) {
                            for (int i = 0; i < std::min(4, (int)in["DEFAULT"].size()); i++) p.defaultColor[i] = in["DEFAULT"][i].get<float>();
                        }
                        break;
                    case ISFLoader::PARAM_POINT2D:
                        if (in.contains("DEFAULT") && in["DEFAULT"].is_array() && in["DEFAULT"].size() >= 2) {
                            p.defaultPoint[0] = in["DEFAULT"][0].get<float>();
                            p.defaultPoint[1] = in["DEFAULT"][1].get<float>();
                        }
                        if (in.contains("MIN") && in["MIN"].is_array() && in["MIN"].size() >= 2) {
                            p.minPoint[0] = in["MIN"][0].get<float>();
                            p.minPoint[1] = in["MIN"][1].get<float>();
                        }
                        if (in.contains("MAX") && in["MAX"].is_array() && in["MAX"].size() >= 2) {
                            p.maxPoint[0] = in["MAX"][0].get<float>();
                            p.maxPoint[1] = in["MAX"][1].get<float>();
                        }
                        break;
                    default:
                        break;
                }
                params_.push_back(p);
            }
        }
        // every generator gets a RESET event
        bool hasReset = false;
        for (auto& p : params_) if (p.name == "RESET") hasReset = true;
        if (!hasReset) {
            ISFLoader::ParamInfo p;
            p.name = "RESET";
            p.label = "RESET";
            p.type = ISFLoader::PARAM_EVENT;
            p.defaultInt = 0;
            params_.push_back(p);
        }

        // RESOURCES
        if (root.contains("RESOURCES") && root["RESOURCES"].is_array()) {
            for (const auto& r : root["RESOURCES"]) {
                CGResource res;
                res.name = r.value("NAME", std::string());
                if (res.name.empty()) continue;
                std::string type = r.value("TYPE", std::string("texture"));
                if (type == "buffer") {
                    res.kind = CGResource::BUFFER;
                    res.countExpr = r.contains("COUNT") ? json_expr(r["COUNT"], "1024") : "1024";
                    res.stride = std::max(4, r.value("STRIDE", 16));
                    res.stride = (res.stride + 3) & ~3;
                } else {
                    res.kind = CGResource::TEXTURE;
                    res.format = r.value("FORMAT", std::string("rgba16f"));
                    if (!format_info(res.format, res.internalFormat, res.isUint)) {
                        std::cerr << "[ComputeGen] " << name_ << ": unknown FORMAT " << res.format << std::endl;
                        return false;
                    }
                    res.widthExpr = r.contains("WIDTH") ? json_expr(r["WIDTH"], "RENDERSIZE_X") : "RENDERSIZE_X";
                    res.heightExpr = r.contains("HEIGHT") ? json_expr(r["HEIGHT"], "RENDERSIZE_Y") : "RENDERSIZE_Y";
                    res.persistent = r.value("PERSISTENT", false);
                    res.repeat = (r.value("WRAP", std::string("repeat")) != "clamp");
                    res.linear = (r.value("FILTER", std::string("linear")) != "nearest") && !res.isUint;
                }
                resources_.push_back(res);
            }
        }

        // PASSES
        if (!root.contains("PASSES") || !root["PASSES"].is_array() || root["PASSES"].empty()) {
            std::cerr << "[ComputeGen] " << name_ << ": no PASSES" << std::endl;
            return false;
        }
        for (const auto& pj : root["PASSES"]) {
            CGPass pass;
            pass.kernel = pj.value("KERNEL", std::string());
            if (pass.kernel.empty()) continue;
            if (pj.contains("SIZE")) {
                if (pj["SIZE"].is_string()) pass.sizeName = pj["SIZE"].get<std::string>();
                else if (pj["SIZE"].is_array()) {
                    for (const auto& e : pj["SIZE"]) pass.sizeExprs.push_back(json_expr(e, "1"));
                }
            } else {
                pass.sizeName = "OUTPUT";
            }
            if (pj.contains("REPEAT")) pass.repeatExpr = json_expr(pj["REPEAT"], "1");
            if (pj.contains("SWAP") && pj["SWAP"].is_array()) {
                for (const auto& s : pj["SWAP"]) pass.swap.push_back(s.get<std::string>());
            }
            if (pj.contains("USES") && pj["USES"].is_array()) {
                for (const auto& s : pj["USES"]) pass.uses.push_back(s.get<std::string>());
            }
            pass.initOnly = (pj.value("WHEN", std::string("always")) == "init");
            passes_.push_back(pass);
        }
    } catch (const std::exception& e) {
        std::cerr << "[ComputeGen] " << name_ << ": header error: " << e.what() << std::endl;
        return false;
    }
    return !passes_.empty();
}

std::string ComputeGenShader::buildPrelude(CGPass& pass) const {
    pass.imgIdx.clear();
    pass.smpIdx.clear();
    pass.bufIdx.clear();

    auto used = [&](const std::string& n) {
        if (pass.uses.empty()) return true;
        return std::find(pass.uses.begin(), pass.uses.end(), n) != pass.uses.end();
    };
    pass.usesOutput = used("OUTPUT");

    std::ostringstream o;
    o << "#version 430 core\n";
    if (pass.dims == 1) o << "layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;\n";
    else o << "layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;\n";
    o << "#define KERNEL_" << pass.kernel << " 1\n";
    o << "uniform float TIME;\nuniform float TIMEDELTA;\nuniform int FRAMEINDEX;\n"
         "uniform vec2 RENDERSIZE;\nuniform ivec2 TARGETSIZE;\nuniform int REPEATINDEX;\n";

    for (auto& p : params_) {
        switch (p.type) {
            case ISFLoader::PARAM_FLOAT: o << "uniform float " << p.name << ";\n"; break;
            case ISFLoader::PARAM_LONG:
            case ISFLoader::PARAM_BOOL:
            case ISFLoader::PARAM_EVENT: o << "uniform int " << p.name << ";\n"; break;
            case ISFLoader::PARAM_COLOR: o << "uniform vec4 " << p.name << ";\n"; break;
            case ISFLoader::PARAM_POINT2D: o << "uniform vec2 " << p.name << ";\n"; break;
            default: break;
        }
    }

    int imgUnit = 0, smpUnit = 0, bufBind = 0;
    for (size_t i = 0; i < resources_.size(); i++) {
        const CGResource& r = resources_[i];
        if (!used(r.name)) continue;
        if (r.kind == CGResource::BUFFER) {
            o << "#define BIND_" << r.name << " " << bufBind << "\n";
            pass.bufIdx.push_back((int)i);
            bufBind++;
        } else {
            o << "layout(binding = " << imgUnit << ", " << r.format << ") coherent uniform "
              << (r.isUint ? "uimage2D " : "image2D ") << r.name << ";\n";
            pass.imgIdx.push_back((int)i);
            imgUnit++;
            if (r.persistent && !r.isUint) {
                o << "layout(binding = " << smpUnit << ") uniform sampler2D " << r.name << "_prev;\n";
                pass.smpIdx.push_back((int)i);
                smpUnit++;
            }
        }
    }
    if (pass.usesOutput) {
        o << "layout(binding = " << imgUnit << ", rgba8) writeonly uniform image2D outimg;\n";
    }
    o << CG_COMMON;
    o << "#line 1\n";
    return o.str();
}

bool ComputeGenShader::compile() {
    GLint maxImg = 8;
    glGetIntegerv(GL_MAX_IMAGE_UNITS, &maxImg);

    auto findRes = [&](const std::string& n) -> int {
        for (size_t i = 0; i < resources_.size(); i++) if (resources_[i].name == n) return (int)i;
        return -1;
    };

    for (auto& pass : passes_) {
        // resolve size
        pass.sizeIdx = -1;
        if (!pass.sizeExprs.empty()) {
            pass.dims = pass.sizeExprs.size() == 1 ? 1 : 2;
        } else if (pass.sizeName == "OUTPUT" || pass.sizeName.empty()) {
            pass.dims = 2;
        } else {
            pass.sizeIdx = findRes(pass.sizeName);
            if (pass.sizeIdx < 0) {
                std::cerr << "[ComputeGen] " << name_ << ": pass " << pass.kernel << ": unknown SIZE " << pass.sizeName << std::endl;
                return false;
            }
            pass.dims = resources_[pass.sizeIdx].kind == CGResource::BUFFER ? 1 : 2;
        }
        for (auto& s : pass.swap) {
            int idx = findRes(s);
            if (idx < 0 || resources_[idx].kind != CGResource::TEXTURE || !resources_[idx].persistent) {
                std::cerr << "[ComputeGen] " << name_ << ": pass " << pass.kernel << ": SWAP " << s << " is not a persistent texture" << std::endl;
                return false;
            }
            pass.swapIdx.push_back(idx);
        }
        for (auto& u : pass.uses) {
            if (u != "OUTPUT" && findRes(u) < 0) {
                std::cerr << "[ComputeGen] " << name_ << ": pass " << pass.kernel << ": unknown USES " << u << std::endl;
                return false;
            }
        }

        std::string src = buildPrelude(pass) + source_;
        if ((int)pass.imgIdx.size() + (pass.usesOutput ? 1 : 0) > maxImg) {
            std::cerr << "[ComputeGen] " << name_ << ": pass " << pass.kernel << " needs more than " << maxImg
                      << " image units - list fewer resources in USES" << std::endl;
            return false;
        }

        GLuint sh = glCreateShader(GL_COMPUTE_SHADER);
        const char* csrc = src.c_str();
        glShaderSource(sh, 1, &csrc, nullptr);
        glCompileShader(sh);
        GLint ok = 0;
        glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[4096];
            glGetShaderInfoLog(sh, sizeof(log), nullptr, log);
            std::cerr << "[ComputeGen] " << name_ << " kernel '" << pass.kernel << "' compile error:\n" << log << std::endl;
            glDeleteShader(sh);
            return false;
        }
        GLuint prog = glCreateProgram();
        glAttachShader(prog, sh);
        glLinkProgram(prog);
        glGetProgramiv(prog, GL_LINK_STATUS, &ok);
        glDeleteShader(sh);
        if (!ok) {
            char log[4096];
            glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
            std::cerr << "[ComputeGen] " << name_ << " kernel '" << pass.kernel << "' link error:\n" << log << std::endl;
            glDeleteProgram(prog);
            return false;
        }
        pass.program = prog;
    }
    return true;
}

// ---------------------------------------------------------------------------------------------
// ComputeGenInstance
// ---------------------------------------------------------------------------------------------
ComputeGenInstance::ComputeGenInstance(ComputeGenShader* shader) : shader_(shader) {
    for (auto& p : shader_->params_) {
        std::array<float, 4> v = {0, 0, 0, 0};
        switch (p.type) {
            case ISFLoader::PARAM_FLOAT: v[0] = p.defaultVal; break;
            case ISFLoader::PARAM_LONG:
            case ISFLoader::PARAM_BOOL:
            case ISFLoader::PARAM_EVENT: v[0] = (float)p.defaultInt; break;
            case ISFLoader::PARAM_COLOR: for (int i = 0; i < 4; i++) v[i] = p.defaultColor[i]; break;
            case ISFLoader::PARAM_POINT2D: v[0] = p.defaultPoint[0]; v[1] = p.defaultPoint[1]; break;
            default: break;
        }
        values_[p.name] = v;
    }
    res_.resize(shader_->resources_.size());
    resW_.assign(res_.size(), 0);
    resH_.assign(res_.size(), 0);
}

ComputeGenInstance::~ComputeGenInstance() {
    release();
}

void ComputeGenInstance::setParameter(const std::string& name, float x, float y, float z, float w) {
    auto it = values_.find(name);
    if (it == values_.end()) return;
    it->second = {x, y, z, w};
}

void ComputeGenInstance::reset() {
    resetRequested_ = true;
}

// --- tiny expression evaluator: numbers, $name / name variables, + - * / ( ) and unary minus ---
namespace {
struct Expr {
    const std::string& s;
    size_t p = 0;
    const std::function<double(const std::string&)>& var;
    Expr(const std::string& str, const std::function<double(const std::string&)>& v) : s(str), var(v) {}
    void ws() { while (p < s.size() && std::isspace((unsigned char)s[p])) p++; }
    double factor() {
        ws();
        if (p >= s.size()) return 0;
        if (s[p] == '-') { p++; return -factor(); }
        if (s[p] == '+') { p++; return factor(); }
        if (s[p] == '(') {
            p++;
            double v = expr();
            ws();
            if (p < s.size() && s[p] == ')') p++;
            return v;
        }
        if (std::isdigit((unsigned char)s[p]) || s[p] == '.') {
            size_t e = p;
            while (e < s.size() && (std::isdigit((unsigned char)s[e]) || s[e] == '.')) e++;
            double v = std::atof(s.substr(p, e - p).c_str());
            p = e;
            return v;
        }
        if (s[p] == '$') p++;
        size_t e = p;
        while (e < s.size() && (std::isalnum((unsigned char)s[e]) || s[e] == '_')) e++;
        std::string name = s.substr(p, e - p);
        p = e;
        return var(name);
    }
    double term() {
        double v = factor();
        for (;;) {
            ws();
            if (p < s.size() && s[p] == '*') { p++; v *= factor(); }
            else if (p < s.size() && s[p] == '/') { p++; double d = factor(); v = d != 0 ? v / d : 0; }
            else return v;
        }
    }
    double expr() {
        double v = term();
        for (;;) {
            ws();
            if (p < s.size() && s[p] == '+') { p++; v += term(); }
            else if (p < s.size() && s[p] == '-') { p++; v -= term(); }
            else return v;
        }
    }
};
}

double ComputeGenInstance::eval(const std::string& expr, int rw, int rh) const {
    std::function<double(const std::string&)> var = [&](const std::string& n) -> double {
        if (n == "RENDERSIZE_X") return rw;
        if (n == "RENDERSIZE_Y") return rh;
        auto it = values_.find(n);
        if (it != values_.end()) return it->second[0];
        return 0;
    };
    Expr e(expr, var);
    return e.expr();
}

int ComputeGenInstance::evalInt(const std::string& expr, int rw, int rh, int lo, int hi) const {
    double v = eval(expr, rw, rh);
    if (!(v == v)) v = lo;
    long long i = (long long)std::floor(v + 0.5);
    if (i < lo) i = lo;
    if (i > hi) i = hi;
    return (int)i;
}

void ComputeGenInstance::release() {
    for (auto& r : res_) {
        if (r.tex[0]) glDeleteTextures(1, &r.tex[0]);
        if (r.tex[1]) glDeleteTextures(1, &r.tex[1]);
        if (r.buf) glDeleteBuffers(1, &r.buf);
        r = ResState();
    }
    if (outTex_) glDeleteTextures(1, &outTex_);
    outTex_ = 0;
    outW_ = outH_ = 0;
    allocated_ = false;
}

bool ComputeGenInstance::allocate(int rw, int rh) {
    release();
    GLint maxTex = 8192;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTex);

    for (size_t i = 0; i < shader_->resources_.size(); i++) {
        const CGResource& r = shader_->resources_[i];
        ResState& s = res_[i];
        if (r.kind == CGResource::TEXTURE) {
            s.w = evalInt(r.widthExpr, rw, rh, 1, maxTex);
            s.h = evalInt(r.heightExpr, rw, rh, 1, maxTex);
            int n = r.persistent ? 2 : 1;
            for (int k = 0; k < n; k++) {
                glCreateTextures(GL_TEXTURE_2D, 1, &s.tex[k]);
                glTextureStorage2D(s.tex[k], 1, r.internalFormat, s.w, s.h);
                glTextureParameteri(s.tex[k], GL_TEXTURE_MIN_FILTER, r.linear ? GL_LINEAR : GL_NEAREST);
                glTextureParameteri(s.tex[k], GL_TEXTURE_MAG_FILTER, r.linear ? GL_LINEAR : GL_NEAREST);
                glTextureParameteri(s.tex[k], GL_TEXTURE_WRAP_S, r.repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
                glTextureParameteri(s.tex[k], GL_TEXTURE_WRAP_T, r.repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
            }
            s.cur = 0;
            resW_[i] = s.w;
            resH_[i] = s.h;
        } else {
            s.w = evalInt(r.countExpr, rw, rh, 1, 1 << 24);
            s.h = 1;
            glCreateBuffers(1, &s.buf);
            glNamedBufferData(s.buf, (GLsizeiptr)s.w * r.stride, nullptr, GL_DYNAMIC_COPY);
            resW_[i] = s.w;
            resH_[i] = 1;
        }
    }
    outW_ = rw;
    outH_ = rh;
    glCreateTextures(GL_TEXTURE_2D, 1, &outTex_);
    glTextureStorage2D(outTex_, 1, GL_RGBA8, outW_, outH_);
    glTextureParameteri(outTex_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(outTex_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(outTex_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(outTex_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    allocated_ = true;
    clearState();
    frameIndex_ = 0;
    return true;
}

void ComputeGenInstance::clearState() {
    for (size_t i = 0; i < shader_->resources_.size(); i++) {
        const CGResource& r = shader_->resources_[i];
        ResState& s = res_[i];
        if (r.kind == CGResource::TEXTURE) {
            for (int k = 0; k < 2; k++) {
                if (!s.tex[k]) continue;
                if (r.isUint) glClearTexImage(s.tex[k], 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
                else glClearTexImage(s.tex[k], 0, GL_RGBA, GL_FLOAT, nullptr);
            }
            s.cur = 0;
        } else if (s.buf) {
            glClearNamedBufferData(s.buf, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
        }
    }
    if (outTex_) glClearTexImage(outTex_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
}

GLint ComputeGenInstance::location(CGPass& pass, const std::string& uniform) {
    auto it = pass.locs.find(uniform);
    if (it != pass.locs.end()) return it->second;
    GLint loc = glGetUniformLocation(pass.program, uniform.c_str());
    pass.locs[uniform] = loc;
    return loc;
}

void ComputeGenInstance::setUniforms(CGPass& pass, int targetW, int targetH, int repeatIndex) {
    GLuint prog = pass.program;
    glProgramUniform1f(prog, location(pass, "TIME"), time_);
    glProgramUniform1f(prog, location(pass, "TIMEDELTA"), dt_);
    glProgramUniform1i(prog, location(pass, "FRAMEINDEX"), frameIndex_);
    glProgramUniform2f(prog, location(pass, "RENDERSIZE"), (float)outW_, (float)outH_);
    glProgramUniform2i(prog, location(pass, "TARGETSIZE"), targetW, targetH);
    glProgramUniform1i(prog, location(pass, "REPEATINDEX"), repeatIndex);
    for (auto& p : shader_->params_) {
        auto it = values_.find(p.name);
        if (it == values_.end()) continue;
        GLint loc = location(pass, p.name);
        if (loc < 0) continue;
        const auto& v = it->second;
        switch (p.type) {
            case ISFLoader::PARAM_FLOAT: glProgramUniform1f(prog, loc, v[0]); break;
            case ISFLoader::PARAM_LONG:
            case ISFLoader::PARAM_BOOL:
            case ISFLoader::PARAM_EVENT: glProgramUniform1i(prog, loc, (int)std::floor(v[0] + 0.5f)); break;
            case ISFLoader::PARAM_COLOR: glProgramUniform4f(prog, loc, v[0], v[1], v[2], v[3]); break;
            case ISFLoader::PARAM_POINT2D: glProgramUniform2f(prog, loc, v[0], v[1]); break;
            default: break;
        }
    }
}

void ComputeGenInstance::runPass(CGPass& pass, int rw, int rh, int repeatIndex) {
    // target size
    int tw = 1, th = 1;
    if (!pass.sizeExprs.empty()) {
        tw = evalInt(pass.sizeExprs[0], rw, rh, 1, 1 << 24);
        if (pass.sizeExprs.size() > 1) th = evalInt(pass.sizeExprs[1], rw, rh, 1, 1 << 16);
    } else if (pass.sizeIdx >= 0) {
        tw = res_[pass.sizeIdx].w;
        th = res_[pass.sizeIdx].h;
    } else {
        tw = outW_;
        th = outH_;
    }

    glUseProgram(pass.program);
    setUniforms(pass, tw, th, repeatIndex);

    // bind resources in the order the prelude declared them
    int unit = 0;
    for (int idx : pass.imgIdx) {
        const CGResource& r = shader_->resources_[idx];
        ResState& s = res_[idx];
        GLuint tex = r.persistent ? s.tex[1 - s.cur] : s.tex[0];
        glBindImageTexture(unit++, tex, 0, GL_FALSE, 0, GL_READ_WRITE, r.internalFormat);
    }
    if (pass.usesOutput) {
        glBindImageTexture(unit, outTex_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
    }
    unit = 0;
    for (int idx : pass.smpIdx) {
        ResState& s = res_[idx];
        glBindTextureUnit(unit++, s.tex[s.cur]);
    }
    int b = 0;
    for (int idx : pass.bufIdx) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, b++, res_[idx].buf);
    }

    GLuint gx, gy = 1;
    if (pass.dims == 1) {
        gx = (GLuint)((tw + 255) / 256);
    } else {
        gx = (GLuint)((tw + 15) / 16);
        gy = (GLuint)((th + 15) / 16);
    }
    glDispatchCompute(gx, gy, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    for (int idx : pass.swapIdx) {
        res_[idx].cur = 1 - res_[idx].cur;
    }
}

void ComputeGenInstance::render(float time, int width, int height) {
    if (width < 1 || height < 1) return;

    // clones share one instance: step the simulation only once per frame (equal time stamp) unless a RESET is pending
    auto rst = values_.find("RESET");
    bool resetPending = resetRequested_ || (rst != values_.end() && rst->second[0] > 0.5f);
    if (allocated_ && !resetPending && lastTime_ >= 0.0f && time == lastTime_ && width == outW_ && height == outH_) return;

    // time step
    if (lastTime_ < 0.0f || time < lastTime_) dt_ = 1.0f / 60.0f;
    else dt_ = std::min(0.1f, std::max(0.0005f, time - lastTime_));
    lastTime_ = time;

    // the RESET event is consumed here
    auto rit = values_.find("RESET");
    if (rit != values_.end() && rit->second[0] > 0.5f) {
        resetRequested_ = true;
        rit->second[0] = 0.0f;
    }

    // (re)allocate when the output size or any parameter-driven resource size changed
    bool realloc = !allocated_ || outW_ != width || outH_ != height;
    if (!realloc) {
        for (size_t i = 0; i < shader_->resources_.size() && !realloc; i++) {
            const CGResource& r = shader_->resources_[i];
            if (r.kind == CGResource::TEXTURE) {
                GLint maxTex = 8192;
                glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTex);
                if (evalInt(r.widthExpr, width, height, 1, maxTex) != resW_[i] ||
                    evalInt(r.heightExpr, width, height, 1, maxTex) != resH_[i]) realloc = true;
            } else {
                if (evalInt(r.countExpr, width, height, 1, 1 << 24) != resW_[i]) realloc = true;
            }
        }
    }
    if (realloc) {
        allocate(width, height);
        resetRequested_ = false;
        timeBase_ = time;
    } else if (resetRequested_) {
        clearState();
        frameIndex_ = 0;
        resetRequested_ = false;
        timeBase_ = time;                 // TIME restarts too, so orbiting cameras and emitters start over as well
    }
    time_ = time - timeBase_;

    for (auto& pass : shader_->passes_) {
        if (pass.initOnly && frameIndex_ != 0) continue;
        int reps = evalInt(pass.repeatExpr, width, height, 0, 256);
        for (int k = 0; k < reps; k++) runPass(pass, width, height, k);
    }
    frameIndex_++;

    // leave no sampler bindings behind for the main renderer
    for (int u = 0; u < 8; u++) glBindTextureUnit(u, 0);
    glUseProgram(0);
}

#endif // USE_GLES
