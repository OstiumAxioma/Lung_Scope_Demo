#include "SceneManager.h"
#include "CameraController.h"
#include "ModelManager.h"
#include "PathVisualization.h"
#include "RenderingEngine.h"
#include "NavigationController.h"
#include "CameraPath.h"

#include <vtkPolyData.h>
#include <iostream>

namespace BronchoscopyLib {
    
    class SceneManager::Impl {
    public:
        // 模块引用（不拥有）
        CameraController* cameraController;
        ModelManager* modelManager;
        PathVisualization* pathVisualization;
        RenderingEngine* renderingEngine;
        NavigationController* navigationController;
        
        // 场景状态
        bool showPath;
        bool showMarker;
        bool autoRender;
        bool sceneInitialized;
        
        Impl() : cameraController(nullptr), modelManager(nullptr),
                 pathVisualization(nullptr), renderingEngine(nullptr),
                 navigationController(nullptr),
                 showPath(true), showMarker(true), 
                 autoRender(true), sceneInitialized(false) {
        }
        
        bool AreAllModulesSet() const {
            return cameraController && modelManager && pathVisualization && 
                   renderingEngine && navigationController;
        }
        
        void TriggerRender() {
            if (autoRender && renderingEngine) {
                renderingEngine->Render();
            }
        }
    };
    
    SceneManager::SceneManager() : pImpl(std::make_unique<Impl>()) {
    }
    
    SceneManager::~SceneManager() = default;
    
    void SceneManager::SetCameraController(CameraController* controller) {
        pImpl->cameraController = controller;
        std::cout << "SceneManager: CameraController set" << std::endl;
    }
    
    void SceneManager::SetModelManager(ModelManager* manager) {
        pImpl->modelManager = manager;
        std::cout << "SceneManager: ModelManager set" << std::endl;
    }
    
    void SceneManager::SetPathVisualization(PathVisualization* pathViz) {
        pImpl->pathVisualization = pathViz;
        std::cout << "SceneManager: PathVisualization set" << std::endl;
    }
    
    void SceneManager::SetRenderingEngine(RenderingEngine* engine) {
        pImpl->renderingEngine = engine;
        std::cout << "SceneManager: RenderingEngine set" << std::endl;
    }
    
    void SceneManager::SetNavigationController(NavigationController* navController) {
        pImpl->navigationController = navController;
        
        // 设置导航回调
        if (navController) {
            navController->SetNavigationCallback(
                [this](PathNode* node, int index) {
                    this->OnNavigationChanged(node, index);
                }
            );
        }
        
        std::cout << "SceneManager: NavigationController set" << std::endl;
    }
    
    void SceneManager::InitializeScene() {
        if (!pImpl->AreAllModulesSet()) {
            std::cerr << "SceneManager: Cannot initialize - not all modules are set" << std::endl;
            return;
        }
        
        // 初始化各模块
        pImpl->cameraController->InitializeCameras();
        pImpl->renderingEngine->Initialize();
        pImpl->pathVisualization->Initialize();
        
        // 连接相机到渲染器
        if (pImpl->renderingEngine && pImpl->cameraController) {
            pImpl->renderingEngine->SetOverviewCamera(
                pImpl->cameraController->GetOverviewCamera());
            pImpl->renderingEngine->SetEndoscopeCamera(
                pImpl->cameraController->GetEndoscopeCamera());
                
            pImpl->cameraController->AttachToRenderers(
                pImpl->renderingEngine->GetOverviewRenderer(),
                pImpl->renderingEngine->GetEndoscopeRenderer());
        }
        
        pImpl->sceneInitialized = true;
        std::cout << "SceneManager: Scene initialized" << std::endl;
    }
    
    void SceneManager::UpdateScene() {
        if (!pImpl->sceneInitialized) return;
        
        // 更新场景组件
        if (pImpl->navigationController) {
            PathNode* currentNode = pImpl->navigationController->GetCurrentNode();
            int currentIndex = pImpl->navigationController->GetCurrentIndex();
            
            if (currentNode) {
                UpdateFromNavigation(currentNode, currentIndex);
            }
        }
        
        // 触发渲染
        pImpl->TriggerRender();
    }
    
    void SceneManager::UpdateFromNavigation(PathNode* node, int index) {
        if (!node) return;
        
        // 更新内窥镜相机
        if (pImpl->cameraController) {
            pImpl->cameraController->UpdateEndoscopeCamera(node);
        }
        
        // 更新位置标记
        if (pImpl->pathVisualization && pImpl->showMarker) {
            pImpl->pathVisualization->UpdatePositionMarker(node);
        }
        
        std::cout << "SceneManager: Updated for node " << (index + 1) << std::endl;
    }
    
    void SceneManager::ClearScene() {
        ClearModel();
        ClearPath();
        
        pImpl->sceneInitialized = false;
        std::cout << "SceneManager: Scene cleared" << std::endl;
    }
    
    void SceneManager::ClearModel() {
        if (pImpl->modelManager) {
            // 从渲染器移除
            if (pImpl->renderingEngine) {
                pImpl->modelManager->RemoveFromRenderers(
                    pImpl->renderingEngine->GetOverviewRenderer(),
                    pImpl->renderingEngine->GetEndoscopeRenderer());
            }
            
            // 清理模型数据
            pImpl->modelManager->ClearModel();
        }
        
        pImpl->TriggerRender();
        std::cout << "SceneManager: Model cleared" << std::endl;
    }
    
    void SceneManager::ClearPath() {
        if (pImpl->pathVisualization) {
            // 从渲染器移除
            if (pImpl->renderingEngine) {
                pImpl->pathVisualization->RemoveFromRenderers(
                    pImpl->renderingEngine->GetOverviewRenderer(),
                    nullptr);
            }
            
            // 清理路径数据
            pImpl->pathVisualization->ClearPath();
        }
        
        // 重置导航
        if (pImpl->navigationController) {
            pImpl->navigationController->Reset();
        }
        
        pImpl->TriggerRender();
        std::cout << "SceneManager: Path cleared" << std::endl;
    }
    
    void SceneManager::SetShowPath(bool show) {
        pImpl->showPath = show;
        if (pImpl->pathVisualization) {
            pImpl->pathVisualization->ShowPath(show);
            pImpl->TriggerRender();
        }
    }
    
    void SceneManager::SetShowMarker(bool show) {
        pImpl->showMarker = show;
        if (pImpl->pathVisualization) {
            pImpl->pathVisualization->ShowMarker(show);
            pImpl->TriggerRender();
        }
    }
    
    bool SceneManager::IsPathVisible() const {
        return pImpl->showPath;
    }
    
    bool SceneManager::IsMarkerVisible() const {
        return pImpl->showMarker;
    }
    
    void SceneManager::ResetCameras() {
        if (pImpl->cameraController && pImpl->modelManager && 
            pImpl->modelManager->HasModel()) {
            double bounds[6];
            pImpl->modelManager->GetModelBounds(bounds);
            pImpl->cameraController->ResetCameras(bounds);
            pImpl->TriggerRender();
        }
    }
    
    void SceneManager::ResetToDefaultView() {
        ResetCameras();
        
        // 重置导航到起点
        if (pImpl->navigationController) {
            pImpl->navigationController->MoveToFirst();
        }
        
        UpdateScene();
    }
    
    bool SceneManager::OnModelLoaded(vtkPolyData* polyData) {
        if (!polyData || !pImpl->modelManager) return false;
        
        // 加载模型
        if (pImpl->modelManager->LoadModel(polyData)) {
            // 添加到渲染器
            if (pImpl->renderingEngine) {
                pImpl->modelManager->AddToRenderers(
                    pImpl->renderingEngine->GetOverviewRenderer(),
                    pImpl->renderingEngine->GetEndoscopeRenderer());
            }
            
            // 重置相机以适应模型
            ResetCameras();
            
            std::cout << "SceneManager: Model loaded and added to scene" << std::endl;
            return true;
        }
        return false;
    }
    
    void SceneManager::OnPathLoaded() {
        if (!pImpl->pathVisualization) return;
        
        // 添加路径可视化到渲染器
        if (pImpl->renderingEngine) {
            pImpl->pathVisualization->AddToRenderers(
                pImpl->renderingEngine->GetOverviewRenderer(),
                nullptr);  // 路径只在overview中显示
        }
        
        // 设置导航控制器
        if (pImpl->navigationController) {
            CameraPath* path = pImpl->pathVisualization->GetCameraPath();
            pImpl->navigationController->SetCameraPath(path);
        }
        
        // 更新场景
        UpdateScene();
        
        std::cout << "SceneManager: Path loaded and added to scene" << std::endl;
    }
    
    void SceneManager::OnNavigationChanged(PathNode* node, int index) {
        UpdateFromNavigation(node, index);
        pImpl->TriggerRender();
    }
    
    void SceneManager::RequestRender() {
        if (pImpl->renderingEngine) {
            pImpl->renderingEngine->Render();
        }
    }
    
    bool SceneManager::HasModel() const {
        return pImpl->modelManager && pImpl->modelManager->HasModel();
    }
    
    bool SceneManager::HasPath() const {
        return pImpl->pathVisualization && 
               pImpl->pathVisualization->GetCameraPath() != nullptr;
    }
    
    bool SceneManager::IsSceneReady() const {
        return pImpl->sceneInitialized && pImpl->AreAllModulesSet();
    }
    
    void SceneManager::SetAutoRender(bool autoRender) {
        pImpl->autoRender = autoRender;
    }
    
    bool SceneManager::GetAutoRender() const {
        return pImpl->autoRender;
    }
    
    void SceneManager::PrintSceneInfo() const {
        std::cout << "\n=== Scene Manager Status ===" << std::endl;
        std::cout << "Scene initialized: " << (pImpl->sceneInitialized ? "Yes" : "No") << std::endl;
        std::cout << "All modules set: " << (pImpl->AreAllModulesSet() ? "Yes" : "No") << std::endl;
        std::cout << "Has model: " << (HasModel() ? "Yes" : "No") << std::endl;
        std::cout << "Has path: " << (HasPath() ? "Yes" : "No") << std::endl;
        std::cout << "Path visible: " << (pImpl->showPath ? "Yes" : "No") << std::endl;
        std::cout << "Marker visible: " << (pImpl->showMarker ? "Yes" : "No") << std::endl;
        std::cout << "Auto render: " << (pImpl->autoRender ? "Yes" : "No") << std::endl;
        
        if (pImpl->navigationController) {
            std::cout << "Navigation: " << (pImpl->navigationController->GetCurrentIndex() + 1) 
                     << " / " << pImpl->navigationController->GetTotalNodes() << std::endl;
            std::cout << "Progress: " << pImpl->navigationController->GetProgressPercentage() 
                     << "%" << std::endl;
        }
        
        std::cout << "============================" << std::endl;
    }
    
} // namespace BronchoscopyLib