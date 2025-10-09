#include "SceneManager.h"
#include "CameraController.h"
#include "ModelManager.h"
#include "PathVisualization.h"
#include "RenderingEngine.h"
#include "NavigationController.h"
#include "CameraPath.h"

#include <vtkPolyData.h>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cmath>

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

        // 样条动画状态
        bool splineAnimating = false;
        int lastNavIndex = -1;
        int splineSegmentIndex = -1; // 当前段索引
        bool splineReverse = false;
        double splineDuration = 0.6; // 每段基本时长
        std::chrono::steady_clock::time_point splineStartTime;
        
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

        static double EaseInOutCubic(double t) {
            if (t < 0.5) return 4.0 * t * t * t;
            double p = 2.0 * t - 2.0;
            return 1.0 + (p * p * p) / 2.0;
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
            // 若当前存在样条动画，则不强制设置为节点位置
            if (!pImpl->splineAnimating) {
                PathNode* currentNode = pImpl->navigationController->GetCurrentNode();
                int currentIndex = pImpl->navigationController->GetCurrentIndex();
                if (currentNode) {
                    UpdateFromNavigation(currentNode, currentIndex);
                }
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
                    pImpl->renderingEngine->GetEndoscopeRenderer());
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
                pImpl->renderingEngine->GetEndoscopeRenderer());
        }
        
        // 设置导航控制器
        if (pImpl->navigationController) {
            CameraPath* path = pImpl->pathVisualization->GetCameraPath();
            pImpl->navigationController->SetCameraPath(path);
            // 重置动画状态
            pImpl->splineAnimating = false;
            pImpl->lastNavIndex = pImpl->navigationController->GetCurrentIndex();
        }
        
        // 更新场景
        UpdateScene();
        
        std::cout << "SceneManager: Path loaded and added to scene" << std::endl;
    }
    
    void SceneManager::OnNavigationChanged(PathNode* node, int index) {
        // 若无路径或节点，直接返回
        if (!pImpl->pathVisualization || !pImpl->pathVisualization->GetCameraPath() || !node) {
            UpdateFromNavigation(node, index);
            pImpl->TriggerRender();
            return;
        }

        CameraPath* path = pImpl->pathVisualization->GetCameraPath();
        int prevIndex = pImpl->lastNavIndex;
        pImpl->lastNavIndex = index;

        // 计算段索引与方向
        int segments = path->GetSegmentCount();
        int segIdx = -1;
        bool reverse = false;
        if (prevIndex >= 0 && index >= 0) {
            if (index == prevIndex + 1) { // next
                segIdx = std::max(0, index - 1);
                reverse = false;
            } else if (index == prevIndex - 1) { // previous
                segIdx = std::min(segments - 1, index);
                segIdx = std::max(0, segIdx);
                reverse = true;
            }
        }

        if (segIdx < 0 || segments <= 0) {
            // 无法确定段，直接跳转
            UpdateFromNavigation(node, index);
            pImpl->TriggerRender();
            return;
        }

        // 基于段长度设置时长
        double a[3], b[3], tmp[3];
        path->GetSplinePosDirBetween(segIdx, 0.0, a, tmp);
        path->GetSplinePosDirBetween(segIdx, 1.0, b, tmp);
        double d = std::sqrt((b[0]-a[0])*(b[0]-a[0]) + (b[1]-a[1])*(b[1]-a[1]) + (b[2]-a[2])*(b[2]-a[2]));
        double base = 0.4, scale = 0.01;
        pImpl->splineDuration = std::min(1.5, std::max(0.2, base + d * scale));

        pImpl->splineSegmentIndex = segIdx;
        pImpl->splineReverse = reverse;
        pImpl->splineAnimating = true;
        pImpl->splineStartTime = std::chrono::steady_clock::now();
        // 首帧立即更新一次
        UpdateAnimation();
    }

    bool SceneManager::UpdateAnimation() {
        if (!pImpl->splineAnimating) return false;
        CameraPath* path = pImpl->pathVisualization ? pImpl->pathVisualization->GetCameraPath() : nullptr;
        if (!path) { pImpl->splineAnimating = false; return false; }

        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - pImpl->splineStartTime;
        double t = elapsed.count() / pImpl->splineDuration;
        if (t >= 1.0) { t = 1.0; pImpl->splineAnimating = false; }
        double eased = Impl::EaseInOutCubic(std::max(0.0, std::min(1.0, t)));
        if (pImpl->splineReverse) eased = 1.0 - eased;

        double pos[3], dir[3];
        path->GetSplinePosDirBetween(pImpl->splineSegmentIndex, eased, pos, dir);
        if (pImpl->cameraController) {
            pImpl->cameraController->UpdateEndoscopeCamera(pos, dir);
        }
        if (pImpl->pathVisualization && pImpl->showMarker) {
            pImpl->pathVisualization->UpdatePositionMarker(pos);
        }
        pImpl->TriggerRender();
        return pImpl->splineAnimating;
    }

    void SceneManager::SetSplineT(double t) {
        pImpl->splineAnimating = false; // 直接定位不启用动画
        if (!pImpl->pathVisualization) return;
        CameraPath* path = pImpl->pathVisualization->GetCameraPath();
        if (!path) return;

        double pos[3], dir[3];
        path->GetSplinePosDirGlobal(t, pos, dir);
        if (pImpl->cameraController) {
            pImpl->cameraController->UpdateEndoscopeCamera(pos, dir);
        }
        if (pImpl->pathVisualization && pImpl->showMarker) {
            pImpl->pathVisualization->UpdatePositionMarker(pos);
        }
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
