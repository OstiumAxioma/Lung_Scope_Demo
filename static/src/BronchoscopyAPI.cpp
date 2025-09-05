#include "BronchoscopyAPI.h"
#include "CameraController.h"
#include "ModelManager.h"
#include "PathVisualization.h"
#include "RenderingEngine.h"
#include "NavigationController.h"
#include "SceneManager.h"
#include "CameraPath.h"

// VTK headers
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkPolyData.h>

#include <iostream>

namespace BronchoscopyLib {
    
    class BronchoscopyAPI::Impl {
    public:
        // All modules
        std::unique_ptr<CameraController> cameraController;
        std::unique_ptr<ModelManager> modelManager;
        std::unique_ptr<PathVisualization> pathVisualization;
        std::unique_ptr<RenderingEngine> renderingEngine;
        std::unique_ptr<NavigationController> navigationController;
        std::unique_ptr<SceneManager> sceneManager;
        
        Impl() {
            // Create all modules
            cameraController = std::make_unique<CameraController>();
            modelManager = std::make_unique<ModelManager>();
            pathVisualization = std::make_unique<PathVisualization>();
            renderingEngine = std::make_unique<RenderingEngine>();
            navigationController = std::make_unique<NavigationController>();
            sceneManager = std::make_unique<SceneManager>();
            
            // Set up SceneManager with all modules
            sceneManager->SetCameraController(cameraController.get());
            sceneManager->SetModelManager(modelManager.get());
            sceneManager->SetPathVisualization(pathVisualization.get());
            sceneManager->SetRenderingEngine(renderingEngine.get());
            sceneManager->SetNavigationController(navigationController.get());
        }
        
        void UpdateViews() {
            sceneManager->UpdateScene();
        }
    };
    
    BronchoscopyAPI::BronchoscopyAPI() : pImpl(std::make_unique<Impl>()) {
    }
    
    BronchoscopyAPI::~BronchoscopyAPI() = default;
    
    void BronchoscopyAPI::Initialize() {
        // Initialize scene with all modules
        pImpl->sceneManager->InitializeScene();
        
        std::cout << "BronchoscopyAPI initialized" << std::endl;
    }
    
    bool BronchoscopyAPI::LoadAirwayModel(vtkPolyData* polyData) {
        if (!polyData) {
            std::cerr << "BronchoscopyAPI: Invalid model data" << std::endl;
            return false;
        }
        
        // Load model through SceneManager
        bool success = pImpl->sceneManager->OnModelLoaded(polyData);
        
        if (success) {
            // Render
            Render();
        }
        
        return success;
    }
    
    bool BronchoscopyAPI::LoadCameraPath(const std::vector<double>& positions) {
        if (positions.empty()) {
            std::cerr << "BronchoscopyAPI: Empty path data" << std::endl;
            return false;
        }
        
        // Load path using PathVisualization
        if (!pImpl->pathVisualization->LoadPathFromPositions(positions)) {
            return false;
        }
        
        // Notify SceneManager about path loading
        pImpl->sceneManager->OnPathLoaded();
        
        // Set window reference for immediate updates
        if (pImpl->renderingEngine->GetOverviewRenderWindow()) {
            pImpl->pathVisualization->SetOverviewWindow(
                pImpl->renderingEngine->GetOverviewRenderWindow());
        }
        
        // Render
        Render();
        
        return true;
    }
    
    void BronchoscopyAPI::MoveToNext() {
        if (pImpl->navigationController->MoveToNext()) {
            pImpl->UpdateViews();
        }
    }
    
    void BronchoscopyAPI::MoveToPrevious() {
        if (pImpl->navigationController->MoveToPrevious()) {
            pImpl->UpdateViews();
        }
    }
    
    void BronchoscopyAPI::MoveToFirst() {
        pImpl->navigationController->MoveToFirst();
        pImpl->UpdateViews();
    }
    
    void BronchoscopyAPI::MoveToLast() {
        pImpl->navigationController->MoveToLast();
        pImpl->UpdateViews();
    }
    
    int BronchoscopyAPI::GetCurrentNodeIndex() const {
        return pImpl->navigationController->GetCurrentIndex();
    }
    
    int BronchoscopyAPI::GetTotalPathNodes() const {
        return pImpl->navigationController->GetTotalNodes();
    }
    
    void BronchoscopyAPI::SetOverviewRenderWindow(vtkRenderWindow* window) {
        pImpl->renderingEngine->SetOverviewRenderWindow(window);
        pImpl->renderingEngine->SetupInteractors();
        
        // Update PathVisualization window reference
        if (window) {
            pImpl->pathVisualization->SetOverviewWindow(window);
        }
    }
    
    void BronchoscopyAPI::SetEndoscopeRenderWindow(vtkRenderWindow* window) {
        pImpl->renderingEngine->SetEndoscopeRenderWindow(window);
        pImpl->renderingEngine->SetupInteractors();
    }
    
    vtkRenderer* BronchoscopyAPI::GetOverviewRenderer() {
        return pImpl->renderingEngine->GetOverviewRenderer();
    }
    
    vtkRenderer* BronchoscopyAPI::GetEndoscopeRenderer() {
        return pImpl->renderingEngine->GetEndoscopeRenderer();
    }
    
    void BronchoscopyAPI::ShowPath(bool show) {
        pImpl->sceneManager->SetShowPath(show);
    }
    
    void BronchoscopyAPI::ShowMarker(bool show) {
        pImpl->sceneManager->SetShowMarker(show);
    }
    
    bool BronchoscopyAPI::IsPathVisible() const {
        return pImpl->sceneManager->IsPathVisible();
    }
    
    bool BronchoscopyAPI::IsMarkerVisible() const {
        return pImpl->sceneManager->IsMarkerVisible();
    }
    
    void BronchoscopyAPI::SetPathColor(double r, double g, double b) {
        pImpl->pathVisualization->SetPathColor(r, g, b);
        Render();
    }
    
    void BronchoscopyAPI::SetMarkerColor(double r, double g, double b) {
        pImpl->pathVisualization->SetMarkerColor(r, g, b);
        Render();
    }
    
    void BronchoscopyAPI::SetPathOpacity(double opacity) {
        pImpl->pathVisualization->SetPathOpacity(opacity);
        Render();
    }
    
    void BronchoscopyAPI::SetMarkerRadius(double radius) {
        pImpl->pathVisualization->SetMarkerRadius(radius);
        Render();
    }
    
    void BronchoscopyAPI::SetModelOpacity(double opacity) {
        pImpl->modelManager->SetOverviewOpacity(opacity);
        Render();
    }
    
    void BronchoscopyAPI::ResetCameras() {
        pImpl->sceneManager->ResetCameras();
    }
    
    void BronchoscopyAPI::Render() {
        pImpl->renderingEngine->Render();
    }
    
    bool BronchoscopyAPI::HasModel() const {
        return pImpl->sceneManager->HasModel();
    }
    
    bool BronchoscopyAPI::HasPath() const {
        return pImpl->sceneManager->HasPath();
    }
    
    // 新增：自动播放控制
    void BronchoscopyAPI::StartAutoPlay(int intervalMs) {
        pImpl->navigationController->StartAutoPlay(intervalMs);
    }
    
    void BronchoscopyAPI::StopAutoPlay() {
        pImpl->navigationController->StopAutoPlay();
    }
    
    void BronchoscopyAPI::PauseAutoPlay() {
        pImpl->navigationController->PauseAutoPlay();
    }
    
    void BronchoscopyAPI::ResumeAutoPlay() {
        pImpl->navigationController->ResumeAutoPlay();
    }
    
    bool BronchoscopyAPI::IsPlaying() const {
        return pImpl->navigationController->IsPlaying();
    }
    
    void BronchoscopyAPI::SetPlaySpeed(double speed) {
        pImpl->navigationController->SetPlaySpeed(speed);
    }
    
    double BronchoscopyAPI::GetPlaySpeed() const {
        return pImpl->navigationController->GetPlaySpeed();
    }
    
    void BronchoscopyAPI::SetLoopMode(bool loop) {
        pImpl->navigationController->SetLoopMode(loop);
    }
    
    bool BronchoscopyAPI::GetLoopMode() const {
        return pImpl->navigationController->GetLoopMode();
    }
    
    // 新增：进度控制
    bool BronchoscopyAPI::MoveToPosition(int index) {
        bool result = pImpl->navigationController->MoveToPosition(index);
        if (result) {
            pImpl->UpdateViews();
        }
        return result;
    }
    
    double BronchoscopyAPI::GetProgressPercentage() const {
        return pImpl->navigationController->GetProgressPercentage();
    }
    
    bool BronchoscopyAPI::IsAtStart() const {
        return pImpl->navigationController->IsAtStart();
    }
    
    bool BronchoscopyAPI::IsAtEnd() const {
        return pImpl->navigationController->IsAtEnd();
    }
    
    // 新增：场景管理
    void BronchoscopyAPI::ClearScene() {
        pImpl->sceneManager->ClearScene();
    }
    
    void BronchoscopyAPI::ClearModel() {
        pImpl->sceneManager->ClearModel();
    }
    
    void BronchoscopyAPI::ClearPath() {
        pImpl->sceneManager->ClearPath();
    }
    
    void BronchoscopyAPI::PrintSceneInfo() const {
        pImpl->sceneManager->PrintSceneInfo();
    }
    
    // 新增：渲染控制
    void BronchoscopyAPI::SetOverviewBackground(double r, double g, double b) {
        pImpl->renderingEngine->SetOverviewBackground(r, g, b);
        Render();
    }
    
    void BronchoscopyAPI::SetEndoscopeBackground(double r, double g, double b) {
        pImpl->renderingEngine->SetEndoscopeBackground(r, g, b);
        Render();
    }
    
} // namespace BronchoscopyLib