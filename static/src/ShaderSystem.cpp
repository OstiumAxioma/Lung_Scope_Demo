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

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace BronchoscopyLib {
    
    class ShaderSystem::Impl {
    public:
        // Shader路径
        std::string shaderRootPath;
        
        // 缓存的shader源码
        std::map<std::string, std::string> shaderCache;
        
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
        
        // 获取基础shader路径
        std::string GetBaseShaderPath(BaseShader base, bool isVertex) {
            std::string name;
            switch (base) {
                case SURFACE: name = "surface"; break;
                case LINE: name = "line"; break;
                case POINT: name = "point"; break;
            }
            return "basic/" + name + (isVertex ? ".vert" : ".frag");
        }
        
        // 获取效果shader路径
        std::string GetEffectShaderPath(EffectShader effect) {
            switch (effect) {
                case PHONG: return "effect/phong.frag";
                case FRESNEL: return "effect/fresnel.frag";
                case FOG: return "effect/fog.frag";
                default: return "";
            }
        }
        
        // 获取视图shader路径
        std::string GetViewShaderPath(ViewShader view, bool isVertex) {
            std::string name;
            switch (view) {
                case VIEW_ENDOSCOPE: name = "endoscope"; break;
                case VIEW_OVERVIEW: name = "overview"; break;
                default: return "";
            }
            return "view/" + name + (isVertex ? ".vert" : ".frag");
        }
        
        // 组合shader
        std::string CombineShaders(const ShaderConfig& config, bool isVertex) {
            std::string shader;
            
            if (isVertex) {
                // 顶点着色器：基础 + 视图特定（如果有）
                shader = LoadShaderFile(GetBaseShaderPath(config.base, true));
                
                if (config.view != VIEW_NONE) {
                    std::string viewShader = GetViewShaderPath(config.view, true);
                    if (!viewShader.empty()) {
                        // 这里可以添加shader组合逻辑
                        // 目前简单返回视图特定的shader
                        shader = LoadShaderFile(viewShader);
                    }
                }
            } else {
                // 片段着色器：基础 + 效果 + 视图特定
                shader = LoadShaderFile(GetBaseShaderPath(config.base, false));
                
                // TODO: 实现shader组合逻辑
                // 目前简单返回基础shader
            }
            
            return shader;
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
        
        // 根据视图类型应用不同的shader效果
        if (config.view == VIEW_OVERVIEW) {
            // Overview视图 - 增强边缘光照
            mapper->AddShaderReplacement(
                vtkShader::Fragment,
                "//VTK::Light::Impl",  // 替换光照实现
                false,  // 在标准替换之后
                "//VTK::Light::Impl\n"  // 保留默认光照
                "  // 增强边缘光照\n"
                "  vec3 viewDir = normalize(-vertexVC.xyz);\n"
                "  float rim = 1.0 - max(dot(viewDir, normalVCVSOutput), 0.0);\n"
                "  rim = smoothstep(0.6, 1.0, rim);\n"
                "  vec3 rimLight = vec3(0.2, 0.2, 0.25) * rim;\n"
                "  fragOutput0.rgb += rimLight;\n",
                false  // 只做一次
            );
            
            // 调整基础颜色
            mapper->AddShaderReplacement(
                vtkShader::Fragment,
                "//VTK::Color::Impl",
                false,
                "//VTK::Color::Impl\n"
                "  // 微调颜色\n"
                "  diffuseColor = vec3(0.85, 0.85, 0.8);\n"
                "  ambientColor = vec3(0.3, 0.3, 0.3);\n",
                false
            );
        }
        else if (config.view == VIEW_ENDOSCOPE) {
            // Endoscope视图 - 内窥镜效果，微红色调
            mapper->AddShaderReplacement(
                vtkShader::Fragment,
                "//VTK::Color::Impl",
                false,
                "//VTK::Color::Impl\n"
                "  // 内窥镜视图颜色调整\n"
                "  diffuseColor = vec3(0.95, 0.75, 0.7);\n"
                "  ambientColor = vec3(0.35, 0.25, 0.25);\n",
                false
            );
            
            // 调整镜面反射模拟湿润表面
            mapper->AddShaderReplacement(
                vtkShader::Fragment,
                "//VTK::Light::Impl",
                false,
                "//VTK::Light::Impl\n"
                "  // 增强镜面反射\n"
                "  specular *= 1.5;\n",
                false
            );
        }
        
        std::cout << "ShaderSystem: Applied shader replacements for view type " 
                 << config.view << std::endl;
        
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