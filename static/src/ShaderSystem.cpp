#include "ShaderSystem.h"

// VTK headers
#include <vtkActor.h>
#include <vtkOpenGLPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkShader.h>

// Standard headers
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace BronchoscopyLib {
    
    // Shader替换结构
    struct ShaderReplacement {
        vtkShader::Type shaderType;
        std::string tag;
        bool before;
        std::string code;
    };
    
    class ShaderSystem::Impl {
    public:
        // Shader路径
        std::string shaderRootPath;
        
        // 缓存的shader源码
        std::map<std::string, std::string> shaderCache;
        
        // 缓存的shader替换
        std::map<std::string, std::vector<ShaderReplacement>> replacementCache;
        
        // 是否已初始化
        bool initialized;
        
        Impl() : initialized(false) {}
        
        // 查找shader根目录
        std::string FindShaderRoot() {
            std::vector<std::string> possiblePaths = {
                "shaders/",
                "../shaders/",
                "../../shaders/",
                "../../../shaders/"
            };
            
            for (const auto& path : possiblePaths) {
                if (DirectoryExists(path)) {
                    std::cout << "ShaderSystem: Found shaders at: " << path << std::endl;
                    return path;
                }
            }
            
            std::cerr << "ShaderSystem: Warning - Shaders folder not found, using default shaders" << std::endl;
            return "";
        }
        
        // 检查目录是否存在
        bool DirectoryExists(const std::string& path) {
            #ifdef _WIN32
                DWORD attrib = GetFileAttributesA(path.c_str());
                return (attrib != INVALID_FILE_ATTRIBUTES && 
                       (attrib & FILE_ATTRIBUTE_DIRECTORY));
            #else
                struct stat info;
                return (stat(path.c_str(), &info) == 0 && 
                       (info.st_mode & S_IFDIR));
            #endif
        }
        
        // 读取shader文件
        std::string LoadShaderFile(const std::string& relativePath) {
            // 先检查缓存
            auto it = shaderCache.find(relativePath);
            if (it != shaderCache.end()) {
                return it->second;
            }
            
            if (shaderRootPath.empty()) {
                return GetDefaultShader(relativePath);
            }
            
            std::string fullPath = shaderRootPath + relativePath;
            std::ifstream file(fullPath);
            
            if (!file.is_open()) {
                std::cerr << "ShaderSystem: Failed to load shader: " << fullPath << std::endl;
                return GetDefaultShader(relativePath);
            }
            
            std::string content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            
            // 缓存shader
            shaderCache[relativePath] = content;
            
            return content;
        }
        
        // 获取默认shader（后备方案）
        std::string GetDefaultShader(const std::string& relativePath) {
            if (relativePath.find(".vert") != std::string::npos) {
                // 默认顶点着色器
                return R"glsl(
                    #version 120
                    varying vec3 vertexVC;
                    varying vec3 normalVC;
                    
                    void main() {
                        gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
                        vertexVC = vec3(gl_ModelViewMatrix * gl_Vertex);
                        normalVC = gl_NormalMatrix * gl_Normal;
                        gl_FrontColor = gl_Color;
                    }
                )glsl";
            } else {
                // 默认片段着色器
                return R"glsl(
                    #version 120
                    varying vec3 vertexVC;
                    varying vec3 normalVC;
                    
                    void main() {
                        vec3 lightDir = normalize(vec3(0.0, 0.0, 1.0));
                        float diff = max(dot(normalize(normalVC), lightDir), 0.0);
                        vec3 color = gl_Color.rgb * (0.3 + 0.7 * diff);
                        gl_FragColor = vec4(color, gl_Color.a);
                    }
                )glsl";
            }
        }
        
        
        // 获取视图shader路径
        std::string GetViewShaderPath(ViewShader view) {
            switch (view) {
                case VIEW_ENDOSCOPE: return "view/endoscope.shader";
                case VIEW_OVERVIEW: return "view/overview.shader";
                default: return "";
            }
        }
        
        // 解析shader替换文件
        std::vector<ShaderReplacement> ParseShaderFile(const std::string& filepath) {
            std::vector<ShaderReplacement> replacements;
            
            std::string fullPath = shaderRootPath + filepath;
            std::ifstream file(fullPath);
            
            if (!file.is_open()) {
                std::cerr << "ShaderSystem: Failed to open shader file: " << fullPath << std::endl;
                return replacements;
            }
            
            std::string line;
            while (std::getline(file, line)) {
                // 跳过注释和空行
                if (line.empty() || line[0] == '#') continue;
                
                // 查找替换指令 [Type] Tag BEFORE/AFTER
                if (line[0] == '[') {
                    ShaderReplacement replacement;
                    
                    // 解析shader类型
                    size_t typeEnd = line.find(']');
                    if (typeEnd == std::string::npos) continue;
                    
                    std::string typeStr = line.substr(1, typeEnd - 1);
                    if (typeStr == "Vertex") {
                        replacement.shaderType = vtkShader::Vertex;
                    } else if (typeStr == "Fragment") {
                        replacement.shaderType = vtkShader::Fragment;
                    } else {
                        continue;
                    }
                    
                    // 解析tag和before/after
                    size_t tagStart = line.find(' ', typeEnd) + 1;
                    size_t tagEnd = line.find(' ', tagStart);
                    replacement.tag = line.substr(tagStart, tagEnd - tagStart);
                    
                    std::string timing = line.substr(tagEnd + 1);
                    replacement.before = (timing == "BEFORE");
                    
                    // 读取shader代码直到END_REPLACEMENT
                    std::string code;
                    while (std::getline(file, line)) {
                        if (line == "END_REPLACEMENT") break;
                        code += line + "\n";
                    }
                    replacement.code = code;
                    
                    replacements.push_back(replacement);
                }
            }
            
            return replacements;
        }
    };
    
    ShaderSystem::ShaderSystem() : pImpl(std::make_unique<Impl>()) {
    }
    
    ShaderSystem::~ShaderSystem() = default;
    
    bool ShaderSystem::Initialize() {
        if (pImpl->initialized) {
            return true;
        }
        
        // 查找shader根目录
        pImpl->shaderRootPath = pImpl->FindShaderRoot();
        
        pImpl->initialized = true;
        
        return !pImpl->shaderRootPath.empty();
    }
    
    bool ShaderSystem::ApplyShader(vtkActor* actor, const ShaderConfig& config) {
        if (!actor) {
            std::cerr << "ShaderSystem: Invalid actor" << std::endl;
            return false;
        }
        
        vtkMapper* mapper = actor->GetMapper();
        if (!mapper) {
            std::cerr << "ShaderSystem: Actor has no mapper" << std::endl;
            return false;
        }
        
        vtkOpenGLPolyDataMapper* glMapper = 
            vtkOpenGLPolyDataMapper::SafeDownCast(mapper);
        
        if (!glMapper) {
            std::cerr << "ShaderSystem: Mapper is not OpenGL poly data mapper" << std::endl;
            return false;
        }
        
        return ApplyShaderToMapper(glMapper, config);
    }
    
    bool ShaderSystem::ApplyShaderToMapper(vtkOpenGLPolyDataMapper* mapper, 
                                          const ShaderConfig& config) {
        if (!mapper) {
            return false;
        }
        
        if (!pImpl->initialized) {
            Initialize();
        }
        
        // 清除之前的shader替换
        mapper->ClearAllShaderReplacements();
        
        // 获取shader文件路径
        std::string shaderPath = pImpl->GetViewShaderPath(config.view);
        if (shaderPath.empty() && config.view != VIEW_NONE) {
            std::cerr << "ShaderSystem: No shader file for view type " << config.view << std::endl;
            return false;
        }
        
        // 检查缓存
        std::vector<ShaderReplacement> replacements;
        auto cacheIt = pImpl->replacementCache.find(shaderPath);
        if (cacheIt != pImpl->replacementCache.end()) {
            replacements = cacheIt->second;
        } else {
            // 解析shader文件
            replacements = pImpl->ParseShaderFile(shaderPath);
            if (!replacements.empty()) {
                pImpl->replacementCache[shaderPath] = replacements;
                std::cout << "ShaderSystem: Loaded " << replacements.size() 
                         << " shader replacements from " << shaderPath << std::endl;
            }
        }
        
        // 应用所有替换
        for (const auto& replacement : replacements) {
            mapper->AddShaderReplacement(
                replacement.shaderType,
                replacement.tag.c_str(),
                replacement.before,
                replacement.code.c_str(),
                false  // 只做一次
            );
        }
        
        if (!replacements.empty()) {
            std::cout << "ShaderSystem: Applied " << replacements.size() 
                     << " shader replacements for view type " << config.view << std::endl;
        }
        
        return true;
    }
    
    bool ShaderSystem::ApplyPostProcessing(vtkRenderer* renderer, PostShader postEffect) {
        if (!renderer || postEffect == POST_NONE) {
            return false;
        }
        
        // TODO: 实现FXAA后处理
        // VTK 8.2中需要使用vtkRenderPass系统
        std::cout << "ShaderSystem: Post-processing not yet implemented" << std::endl;
        
        return false;
    }
    
    bool ShaderSystem::ReloadAllShaders() {
        // 清空缓存
        pImpl->shaderCache.clear();
        
        std::cout << "ShaderSystem: All shaders reloaded" << std::endl;
        return true;
    }
    
    bool ShaderSystem::IsInitialized() const {
        return pImpl->initialized;
    }
    
    std::string ShaderSystem::GetShaderRootPath() const {
        return pImpl->shaderRootPath;
    }
    
} // namespace BronchoscopyLib