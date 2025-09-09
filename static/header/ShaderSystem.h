#ifndef SHADER_SYSTEM_H
#define SHADER_SYSTEM_H

#include <string>
#include <vector>
#include <memory>

// 前向声明VTK类
class vtkActor;
class vtkOpenGLPolyDataMapper;
class vtkRenderer;

namespace BronchoscopyLib {
    
    /**
     * ShaderSystem - Shader管理系统
     * 从主程序的shaders文件夹加载和管理shader
     */
    class ShaderSystem {
    public:
        // 基础shader类型（物体渲染方式）
        enum BaseShader {
            SURFACE,    // 3D表面（气管模型）
            LINE,       // 线条（路径）
            POINT       // 点（标记球）
        };
        
        // 效果shader（可选的增强效果）
        enum EffectShader {
            EFFECT_NONE,
            PHONG,      // Phong光照
            FRESNEL,    // 菲涅尔效果
            FOG         // 雾效果
        };
        
        // 视图特定shader
        enum ViewShader {
            VIEW_NONE,
            VIEW_ENDOSCOPE,  // 腔镜视图特效
            VIEW_OVERVIEW    // 全局视图特效
        };
        
        // 后处理shader（简化版，只有FXAA）
        enum PostShader {
            POST_NONE,
            POST_FXAA        // FXAA抗锯齿
        };
        
        // Shader配置
        struct ShaderConfig {
            BaseShader base;
            EffectShader effect;
            ViewShader view;
            
            ShaderConfig(BaseShader b = SURFACE, 
                        EffectShader e = EFFECT_NONE, 
                        ViewShader v = VIEW_NONE) 
                : base(b), effect(e), view(v) {}
        };
        
        ShaderSystem();
        ~ShaderSystem();
        
        // 初始化系统，自动查找shader路径
        bool Initialize();
        
        // 应用shader组合到actor
        bool ApplyShader(vtkActor* actor, const ShaderConfig& config);
        
        // 应用shader到mapper（更底层的接口）
        bool ApplyShaderToMapper(vtkOpenGLPolyDataMapper* mapper, 
                                 const ShaderConfig& config);
        
        // 应用后处理到渲染器
        bool ApplyPostProcessing(vtkRenderer* renderer, PostShader postEffect);
        
        // 重新加载所有shader（开发时热重载）
        bool ReloadAllShaders();
        
        // 检查系统是否初始化
        bool IsInitialized() const;
        
        // 获取shader根路径
        std::string GetShaderRootPath() const;
        
    private:
        class Impl;
        std::unique_ptr<Impl> pImpl;
    };
    
} // namespace BronchoscopyLib

#endif // SHADER_SYSTEM_H