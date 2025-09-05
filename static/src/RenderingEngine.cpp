#include "RenderingEngine.h"

// VTK头文件
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkCamera.h>
#include <vtkRendererCollection.h>

#include <iostream>

namespace BronchoscopyLib {
    
    class RenderingEngine::Impl {
    public:
        // 渲染器
        vtkSmartPointer<vtkRenderer> overviewRenderer;
        vtkSmartPointer<vtkRenderer> endoscopeRenderer;
        
        // 渲染窗口（弱引用，不拥有）
        vtkRenderWindow* overviewWindow;
        vtkRenderWindow* endoscopeWindow;
        
        // 背景颜色
        double overviewBgColor[3];
        double endoscopeBgColor[3];
        
        // 初始化标志
        bool initialized;
        
        Impl() : overviewWindow(nullptr), endoscopeWindow(nullptr), initialized(false) {
            // 默认背景颜色
            overviewBgColor[0] = 0.1; overviewBgColor[1] = 0.2; overviewBgColor[2] = 0.4;
            endoscopeBgColor[0] = 0.15; endoscopeBgColor[1] = 0.15; endoscopeBgColor[2] = 0.15;
        }
        
        void CreateRenderers() {
            overviewRenderer = vtkSmartPointer<vtkRenderer>::New();
            endoscopeRenderer = vtkSmartPointer<vtkRenderer>::New();
            
            // 设置背景颜色
            overviewRenderer->SetBackground(overviewBgColor);
            endoscopeRenderer->SetBackground(endoscopeBgColor);
            
            initialized = true;
        }
    };
    
    RenderingEngine::RenderingEngine() : pImpl(std::make_unique<Impl>()) {
    }
    
    RenderingEngine::~RenderingEngine() = default;
    
    void RenderingEngine::Initialize() {
        if (pImpl->initialized) {
            std::cout << "RenderingEngine: Already initialized" << std::endl;
            return;
        }
        
        pImpl->CreateRenderers();
        
        std::cout << "RenderingEngine initialized" << std::endl;
    }
    
    vtkRenderer* RenderingEngine::GetOverviewRenderer() const {
        return pImpl->overviewRenderer;
    }
    
    vtkRenderer* RenderingEngine::GetEndoscopeRenderer() const {
        return pImpl->endoscopeRenderer;
    }
    
    void RenderingEngine::SetOverviewRenderWindow(vtkRenderWindow* window) {
        pImpl->overviewWindow = window;
        
        if (window && pImpl->overviewRenderer) {
            // 先移除可能存在的旧渲染器
            vtkRendererCollection* renderers = window->GetRenderers();
            if (renderers) {
                renderers->InitTraversal();
                vtkRenderer* ren;
                while ((ren = renderers->GetNextItem()) != nullptr) {
                    window->RemoveRenderer(ren);
                }
            }
            
            // 添加新渲染器
            window->AddRenderer(pImpl->overviewRenderer);
            
            std::cout << "Overview renderer attached to window" << std::endl;
        }
    }
    
    void RenderingEngine::SetEndoscopeRenderWindow(vtkRenderWindow* window) {
        pImpl->endoscopeWindow = window;
        
        if (window && pImpl->endoscopeRenderer) {
            // 先移除可能存在的旧渲染器
            vtkRendererCollection* renderers = window->GetRenderers();
            if (renderers) {
                renderers->InitTraversal();
                vtkRenderer* ren;
                while ((ren = renderers->GetNextItem()) != nullptr) {
                    window->RemoveRenderer(ren);
                }
            }
            
            // 添加新渲染器
            window->AddRenderer(pImpl->endoscopeRenderer);
            
            std::cout << "Endoscope renderer attached to window" << std::endl;
        }
    }
    
    vtkRenderWindow* RenderingEngine::GetOverviewRenderWindow() const {
        return pImpl->overviewWindow;
    }
    
    vtkRenderWindow* RenderingEngine::GetEndoscopeRenderWindow() const {
        return pImpl->endoscopeWindow;
    }
    
    void RenderingEngine::SetOverviewCamera(vtkCamera* camera) {
        if (pImpl->overviewRenderer && camera) {
            pImpl->overviewRenderer->SetActiveCamera(camera);
            std::cout << "Overview camera set" << std::endl;
        }
    }
    
    void RenderingEngine::SetEndoscopeCamera(vtkCamera* camera) {
        if (pImpl->endoscopeRenderer && camera) {
            pImpl->endoscopeRenderer->SetActiveCamera(camera);
            std::cout << "Endoscope camera set" << std::endl;
        }
    }
    
    void RenderingEngine::Render() {
        RenderOverview();
        RenderEndoscope();
    }
    
    void RenderingEngine::RenderOverview() {
        if (pImpl->overviewWindow) {
            pImpl->overviewWindow->Render();
        }
    }
    
    void RenderingEngine::RenderEndoscope() {
        if (pImpl->endoscopeWindow) {
            pImpl->endoscopeWindow->Render();
        }
    }
    
    void RenderingEngine::SetOverviewBackground(double r, double g, double b) {
        pImpl->overviewBgColor[0] = r;
        pImpl->overviewBgColor[1] = g;
        pImpl->overviewBgColor[2] = b;
        
        if (pImpl->overviewRenderer) {
            pImpl->overviewRenderer->SetBackground(r, g, b);
        }
    }
    
    void RenderingEngine::SetEndoscopeBackground(double r, double g, double b) {
        pImpl->endoscopeBgColor[0] = r;
        pImpl->endoscopeBgColor[1] = g;
        pImpl->endoscopeBgColor[2] = b;
        
        if (pImpl->endoscopeRenderer) {
            pImpl->endoscopeRenderer->SetBackground(r, g, b);
        }
    }
    
    void RenderingEngine::SetupInteractors() {
        // 设置Overview窗口的交互器
        if (pImpl->overviewWindow) {
            vtkRenderWindowInteractor* interactor = pImpl->overviewWindow->GetInteractor();
            if (interactor) {
                vtkSmartPointer<vtkInteractorStyleTrackballCamera> style = 
                    vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
                interactor->SetInteractorStyle(style);
                std::cout << "Overview interactor configured" << std::endl;
            }
        }
        
        // 设置Endoscope窗口的交互器
        if (pImpl->endoscopeWindow) {
            vtkRenderWindowInteractor* interactor = pImpl->endoscopeWindow->GetInteractor();
            if (interactor) {
                vtkSmartPointer<vtkInteractorStyleTrackballCamera> style = 
                    vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
                interactor->SetInteractorStyle(style);
                std::cout << "Endoscope interactor configured" << std::endl;
            }
        }
    }
    
    void RenderingEngine::SetViewport(int view, double xmin, double ymin, double xmax, double ymax) {
        if (view == 0 && pImpl->overviewRenderer) {
            pImpl->overviewRenderer->SetViewport(xmin, ymin, xmax, ymax);
        } else if (view == 1 && pImpl->endoscopeRenderer) {
            pImpl->endoscopeRenderer->SetViewport(xmin, ymin, xmax, ymax);
        }
    }
    
    bool RenderingEngine::IsInitialized() const {
        return pImpl->initialized;
    }
    
} // namespace BronchoscopyLib