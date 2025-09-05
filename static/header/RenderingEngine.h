#ifndef RENDERING_ENGINE_H
#define RENDERING_ENGINE_H

#include <memory>

// 前向声明VTK类
class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkCamera;

namespace BronchoscopyLib {
    
    /**
     * RenderingEngine - 管理渲染器和渲染窗口
     * 负责创建、配置渲染器，管理渲染窗口，执行渲染操作
     */
    class RenderingEngine {
    public:
        RenderingEngine();
        ~RenderingEngine();
        
        // 初始化渲染器
        void Initialize();
        
        // 获取渲染器
        vtkRenderer* GetOverviewRenderer() const;
        vtkRenderer* GetEndoscopeRenderer() const;
        
        // 设置渲染窗口
        void SetOverviewRenderWindow(vtkRenderWindow* window);
        void SetEndoscopeRenderWindow(vtkRenderWindow* window);
        
        // 获取渲染窗口
        vtkRenderWindow* GetOverviewRenderWindow() const;
        vtkRenderWindow* GetEndoscopeRenderWindow() const;
        
        // 设置相机（由CameraController提供）
        void SetOverviewCamera(vtkCamera* camera);
        void SetEndoscopeCamera(vtkCamera* camera);
        
        // 渲染控制
        void Render();
        void RenderOverview();
        void RenderEndoscope();
        
        // 背景颜色设置
        void SetOverviewBackground(double r, double g, double b);
        void SetEndoscopeBackground(double r, double g, double b);
        
        // 交互器设置
        void SetupInteractors();
        
        // 视口设置
        void SetViewport(int view, double xmin, double ymin, double xmax, double ymax);
        
        // 获取渲染状态
        bool IsInitialized() const;
        
    private:
        class Impl;
        std::unique_ptr<Impl> pImpl;
    };
    
} // namespace BronchoscopyLib

#endif // RENDERING_ENGINE_H