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
        
        
        // 获取视图shader路径（分别返回顶点和片段）
        std::pair<std::string, std::string> GetViewShaderPaths(ViewShader view) {
            switch (view) {
                case VIEW_ENDOSCOPE: 
                    return std::make_pair("view/endoscope.vert", "view/endoscope.frag");
                case VIEW_OVERVIEW: 
                    return std::make_pair("view/overview.vert", "view/overview.frag");
                default: 
                    return std::make_pair("", "");
            }
        }
        
        // 获取材质shader路径
        std::pair<std::string, std::string> GetMaterialShaderPaths(MaterialShader material) {
            switch (material) {
                case MATERIAL_TISSUE:
                    return std::make_pair("material/tissue.vert", "material/tissue.frag");
                case MATERIAL_NONE:
                default:
                    return std::make_pair("", "");
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
                // 跳过空行和#注释
                if (line.empty() || line[0] == '#') continue;
                
                // 检查是否为VTK替换标签
                if (line.find("//VTK::") == 0) {
                    ShaderReplacement replacement;
                    
                    // 查找空格分隔标签和BEFORE/AFTER
                    size_t spacePos = line.find(' ');
                    if (spacePos != std::string::npos) {
                        replacement.tag = line.substr(0, spacePos);
                        std::string timing = line.substr(spacePos + 1);
                        replacement.before = (timing == "BEFORE");
                        
                        // 读取下一行（应该是相同的标签作为替换点）
                        if (std::getline(file, line)) {
                            // 继续读取shader代码直到END_REPLACEMENT
                            std::string code = line + "\n";  // 包含标签行本身
                            while (std::getline(file, line)) {
                                if (line == "END_REPLACEMENT") break;
                                code += line + "\n";
                            }
                            replacement.code = code;
                            replacements.push_back(replacement);
                        }
                    }
                }
                // 其他//开头的行视为普通注释，忽略
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
        
        // 获取shader文件路径（顶点和片段分开）
        auto shaderPaths = pImpl->GetViewShaderPaths(config.view);
        if (shaderPaths.first.empty() && config.view != VIEW_NONE) {
            std::cerr << "ShaderSystem: No shader files for view type " << config.view << std::endl;
            return false;
        }
        
        int totalReplacements = 0;
        
        // 加载顶点shader替换
        if (!shaderPaths.first.empty()) {
            std::vector<ShaderReplacement> vertReplacements;
            auto cacheIt = pImpl->replacementCache.find(shaderPaths.first);
            if (cacheIt != pImpl->replacementCache.end()) {
                vertReplacements = cacheIt->second;
            } else {
                vertReplacements = pImpl->ParseShaderFile(shaderPaths.first);
                if (!vertReplacements.empty()) {
                    pImpl->replacementCache[shaderPaths.first] = vertReplacements;
                }
            }
            
            // 应用顶点shader替换
            for (auto& replacement : vertReplacements) {
                replacement.shaderType = vtkShader::Vertex;  // 设置为顶点shader
                mapper->AddShaderReplacement(
                    replacement.shaderType,
                    replacement.tag.c_str(),
                    replacement.before,
                    replacement.code.c_str(),
                    false
                );
                totalReplacements++;
            }
        }
        
        // 加载片段shader替换
        if (!shaderPaths.second.empty()) {
            std::vector<ShaderReplacement> fragReplacements;
            auto cacheIt = pImpl->replacementCache.find(shaderPaths.second);
            if (cacheIt != pImpl->replacementCache.end()) {
                fragReplacements = cacheIt->second;
            } else {
                fragReplacements = pImpl->ParseShaderFile(shaderPaths.second);
                if (!fragReplacements.empty()) {
                    pImpl->replacementCache[shaderPaths.second] = fragReplacements;
                }
            }
            
            // 应用片段shader替换
            for (auto& replacement : fragReplacements) {
                replacement.shaderType = vtkShader::Fragment;  // 设置为片段shader
                mapper->AddShaderReplacement(
                    replacement.shaderType,
                    replacement.tag.c_str(),
                    replacement.before,
                    replacement.code.c_str(),
                    false
                );
                totalReplacements++;
            }
        }
        
        if (totalReplacements > 0) {
            std::cout << "ShaderSystem: Applied " << totalReplacements 
                     << " shader replacements for view type " << config.view << std::endl;
        }
        
        return true;
    }
    
    bool ShaderSystem::ApplyMaterialShader(vtkActor* actor, MaterialShader material) {
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
        
        return ApplyMaterialShaderToMapper(glMapper, material);
    }
    
    bool ShaderSystem::ApplyMaterialShaderToMapper(vtkOpenGLPolyDataMapper* mapper, 
                                                   MaterialShader material) {
        if (!mapper) {
            return false;
        }
        
        if (!pImpl->initialized) {
            Initialize();
        }
        
        // 不清除之前的替换，因为材质和视图shader是叠加的
        // mapper->ClearAllShaderReplacements();
        
        // 获取材质shader文件路径
        auto shaderPaths = pImpl->GetMaterialShaderPaths(material);
        if (shaderPaths.first.empty() && material != MATERIAL_NONE) {
            std::cerr << "ShaderSystem: No shader files for material type " << material << std::endl;
            return false;
        }
        
        int totalReplacements = 0;
        
        // 加载顶点shader替换
        if (!shaderPaths.first.empty()) {
            std::vector<ShaderReplacement> vertReplacements;
            auto cacheIt = pImpl->replacementCache.find(shaderPaths.first);
            if (cacheIt != pImpl->replacementCache.end()) {
                vertReplacements = cacheIt->second;
            } else {
                vertReplacements = pImpl->ParseShaderFile(shaderPaths.first);
                if (!vertReplacements.empty()) {
                    pImpl->replacementCache[shaderPaths.first] = vertReplacements;
                }
            }
            
            for (auto& replacement : vertReplacements) {
                replacement.shaderType = vtkShader::Vertex;
                mapper->AddShaderReplacement(
                    replacement.shaderType,
                    replacement.tag.c_str(),
                    replacement.before,
                    replacement.code.c_str(),
                    false
                );
                totalReplacements++;
            }
        }
        
        // 加载片段shader替换
        if (!shaderPaths.second.empty()) {
            std::vector<ShaderReplacement> fragReplacements;
            auto cacheIt = pImpl->replacementCache.find(shaderPaths.second);
            if (cacheIt != pImpl->replacementCache.end()) {
                fragReplacements = cacheIt->second;
            } else {
                fragReplacements = pImpl->ParseShaderFile(shaderPaths.second);
                if (!fragReplacements.empty()) {
                    pImpl->replacementCache[shaderPaths.second] = fragReplacements;
                }
            }
            
            for (auto& replacement : fragReplacements) {
                replacement.shaderType = vtkShader::Fragment;
                mapper->AddShaderReplacement(
                    replacement.shaderType,
                    replacement.tag.c_str(),
                    replacement.before,
                    replacement.code.c_str(),
                    false
                );
                totalReplacements++;
            }
        }
        
        if (totalReplacements > 0) {
            std::cout << "ShaderSystem: Applied " << totalReplacements 
                     << " material shader replacements" << std::endl;
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