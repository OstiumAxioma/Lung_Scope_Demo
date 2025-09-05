#include "BronchoscopyAPI.h"
#include "CameraController.h"
#include "ModelManager.h"
#include "PathVisualization.h"
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
        // Core modules
        std::unique_ptr<CameraController> cameraController;
        std::unique_ptr<ModelManager> modelManager;
        std::unique_ptr<PathVisualization> pathVisualization;
        
        // Renderers and windows (temporary until RenderingEngine is created)
        vtkSmartPointer<vtkRenderer> overviewRenderer;
        vtkSmartPointer<vtkRenderer> endoscopeRenderer;
        vtkRenderWindow* overviewWindow;
        vtkRenderWindow* endoscopeWindow;
        
        // Navigation state
        PathNode* currentNode;
        int currentNodeIndex;
        
        Impl() : overviewWindow(nullptr), endoscopeWindow(nullptr), 
                 currentNode(nullptr), currentNodeIndex(-1) {
            // Create modules
            cameraController = std::make_unique<CameraController>();
            modelManager = std::make_unique<ModelManager>();
            pathVisualization = std::make_unique<PathVisualization>();
            
            // Create renderers (temporary until RenderingEngine is created)
            overviewRenderer = vtkSmartPointer<vtkRenderer>::New();
            endoscopeRenderer = vtkSmartPointer<vtkRenderer>::New();
            
            // Set background colors
            overviewRenderer->SetBackground(0.1, 0.2, 0.4);
            endoscopeRenderer->SetBackground(0.15, 0.15, 0.15);
        }
        
        void UpdateViews() {
            if (currentNode) {
                // Update endoscope camera
                cameraController->UpdateEndoscopeCamera(currentNode);
                
                // Update position marker
                pathVisualization->UpdatePositionMarker(currentNode);
            }
            
            // Trigger render if windows exist
            if (overviewWindow) {
                overviewWindow->Render();
            }
            if (endoscopeWindow) {
                endoscopeWindow->Render();
            }
        }
        
        void NavigateToNode(PathNode* node, int index) {
            currentNode = node;
            currentNodeIndex = index;
            UpdateViews();
        }
    };
    
    BronchoscopyAPI::BronchoscopyAPI() : pImpl(std::make_unique<Impl>()) {
    }
    
    BronchoscopyAPI::~BronchoscopyAPI() = default;
    
    void BronchoscopyAPI::Initialize() {
        // Initialize camera controller
        pImpl->cameraController->InitializeCameras();
        pImpl->cameraController->AttachToRenderers(pImpl->overviewRenderer, pImpl->endoscopeRenderer);
        
        // Initialize path visualization
        pImpl->pathVisualization->Initialize();
        
        std::cout << "BronchoscopyAPI initialized" << std::endl;
    }
    
    bool BronchoscopyAPI::LoadAirwayModel(vtkPolyData* polyData) {
        if (!polyData) {
            std::cerr << "BronchoscopyAPI: Invalid model data" << std::endl;
            return false;
        }
        
        // Load model
        if (!pImpl->modelManager->LoadModel(polyData)) {
            return false;
        }
        
        // Add to renderers
        pImpl->modelManager->AddToRenderers(pImpl->overviewRenderer, pImpl->endoscopeRenderer);
        
        // Reset cameras to fit model
        double bounds[6];
        pImpl->modelManager->GetModelBounds(bounds);
        pImpl->cameraController->ResetCameras(bounds);
        
        // Render
        Render();
        
        return true;
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
        
        // Add visualization to renderers
        pImpl->pathVisualization->AddToRenderers(pImpl->overviewRenderer, nullptr);
        
        // Set window reference for immediate updates
        if (pImpl->overviewWindow) {
            pImpl->pathVisualization->SetOverviewWindow(pImpl->overviewWindow);
        }
        
        // Navigate to first node
        CameraPath* path = pImpl->pathVisualization->GetCameraPath();
        if (path && path->GetTotalNodes() > 0) {
            path->Reset();
            pImpl->currentNode = path->GetCurrent();
            pImpl->currentNodeIndex = 0;
            pImpl->UpdateViews();
        }
        
        // Render
        Render();
        
        return true;
    }
    
    void BronchoscopyAPI::MoveToNext() {
        CameraPath* path = pImpl->pathVisualization->GetCameraPath();
        if (!path) return;
        
        if (path->MoveNext()) {
            pImpl->currentNodeIndex++;
            pImpl->currentNode = path->GetCurrent();
            pImpl->NavigateToNode(pImpl->currentNode, pImpl->currentNodeIndex);
            std::cout << "Moved to node " << (pImpl->currentNodeIndex + 1) 
                      << " / " << path->GetTotalNodes() << std::endl;
        }
    }
    
    void BronchoscopyAPI::MoveToPrevious() {
        CameraPath* path = pImpl->pathVisualization->GetCameraPath();
        if (!path) return;
        
        if (path->MovePrevious()) {
            pImpl->currentNodeIndex--;
            pImpl->currentNode = path->GetCurrent();
            pImpl->NavigateToNode(pImpl->currentNode, pImpl->currentNodeIndex);
            std::cout << "Moved to node " << (pImpl->currentNodeIndex + 1) 
                      << " / " << path->GetTotalNodes() << std::endl;
        }
    }
    
    void BronchoscopyAPI::MoveToFirst() {
        CameraPath* path = pImpl->pathVisualization->GetCameraPath();
        if (!path) return;
        
        path->Reset();
        pImpl->currentNode = path->GetCurrent();
        pImpl->currentNodeIndex = 0;
        pImpl->UpdateViews();
        std::cout << "Moved to first node" << std::endl;
    }
    
    void BronchoscopyAPI::MoveToLast() {
        CameraPath* path = pImpl->pathVisualization->GetCameraPath();
        if (!path) return;
        
        // Move to last by repeatedly calling MoveNext
        while (path->MoveNext()) {
            pImpl->currentNodeIndex++;
        }
        
        pImpl->currentNode = path->GetCurrent();
        pImpl->NavigateToNode(pImpl->currentNode, pImpl->currentNodeIndex);
        std::cout << "Moved to last node" << std::endl;
    }
    
    int BronchoscopyAPI::GetCurrentNodeIndex() const {
        return pImpl->currentNodeIndex;
    }
    
    int BronchoscopyAPI::GetTotalPathNodes() const {
        return pImpl->pathVisualization->GetTotalPathNodes();
    }
    
    void BronchoscopyAPI::SetOverviewRenderWindow(vtkRenderWindow* window) {
        pImpl->overviewWindow = window;
        
        if (window) {
            window->AddRenderer(pImpl->overviewRenderer);
            
            // Set up interactor
            vtkRenderWindowInteractor* interactor = window->GetInteractor();
            if (interactor) {
                vtkSmartPointer<vtkInteractorStyleTrackballCamera> style = 
                    vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
                interactor->SetInteractorStyle(style);
            }
            
            // Update PathVisualization window reference
            pImpl->pathVisualization->SetOverviewWindow(window);
        }
    }
    
    void BronchoscopyAPI::SetEndoscopeRenderWindow(vtkRenderWindow* window) {
        pImpl->endoscopeWindow = window;
        
        if (window) {
            window->AddRenderer(pImpl->endoscopeRenderer);
            
            // Set up interactor for endoscope (optional, can be disabled for realism)
            vtkRenderWindowInteractor* interactor = window->GetInteractor();
            if (interactor) {
                vtkSmartPointer<vtkInteractorStyleTrackballCamera> style = 
                    vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
                interactor->SetInteractorStyle(style);
            }
        }
    }
    
    vtkRenderer* BronchoscopyAPI::GetOverviewRenderer() {
        return pImpl->overviewRenderer;
    }
    
    vtkRenderer* BronchoscopyAPI::GetEndoscopeRenderer() {
        return pImpl->endoscopeRenderer;
    }
    
    void BronchoscopyAPI::ShowPath(bool show) {
        pImpl->pathVisualization->ShowPath(show);
        Render();
    }
    
    void BronchoscopyAPI::ShowMarker(bool show) {
        pImpl->pathVisualization->ShowMarker(show);
        Render();
    }
    
    bool BronchoscopyAPI::IsPathVisible() const {
        return pImpl->pathVisualization->IsPathVisible();
    }
    
    bool BronchoscopyAPI::IsMarkerVisible() const {
        return pImpl->pathVisualization->IsMarkerVisible();
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
        if (pImpl->modelManager->HasModel()) {
            double bounds[6];
            pImpl->modelManager->GetModelBounds(bounds);
            pImpl->cameraController->ResetCameras(bounds);
            Render();
        }
    }
    
    void BronchoscopyAPI::Render() {
        if (pImpl->overviewWindow) {
            pImpl->overviewWindow->Render();
        }
        if (pImpl->endoscopeWindow) {
            pImpl->endoscopeWindow->Render();
        }
    }
    
    bool BronchoscopyAPI::HasModel() const {
        return pImpl->modelManager->HasModel();
    }
    
    bool BronchoscopyAPI::HasPath() const {
        return pImpl->pathVisualization->GetCameraPath() != nullptr;
    }
    
} // namespace BronchoscopyLib