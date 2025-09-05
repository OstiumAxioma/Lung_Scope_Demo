#include "BronchoscopyViewer.h"
#include "CameraPath.h"
#include "CameraController.h"
#include "ModelManager.h"

// VTK头文件
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyData.h>
#include <vtkCamera.h>
#include <vtkSphereSource.h>
#include <vtkProperty.h>
// 文件读取由主程序负责，静态库不处理文件I/O
// #include <vtkPolyDataReader.h>
// #include <vtkXMLPolyDataReader.h>
// #include <vtkSTLReader.h>
#include <vtkTubeFilter.h>
#include <vtkPolyLine.h>
#include <vtkCellArray.h>
#include <vtkPoints.h>
#include <vtkMath.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include <iostream>
#include <thread>
#include <chrono>

namespace BronchoscopyLib {

    // 使用Pimpl模式隐藏VTK实现细节
    class BronchoscopyViewer::Impl {
    public:
        // 渲染器
        vtkSmartPointer<vtkRenderer> overviewRenderer;
        vtkSmartPointer<vtkRenderer> endoscopeRenderer;
        vtkSmartPointer<vtkRenderWindow> overviewWindow;
        vtkSmartPointer<vtkRenderWindow> endoscopeWindow;
        
        // 相机控制器
        std::unique_ptr<CameraController> cameraController;
        
        // 模型管理器
        std::unique_ptr<ModelManager> modelManager;
        
        // 路径可视化
        CameraPath* cameraPath;
        vtkSmartPointer<vtkActor> pathActor;
        vtkSmartPointer<vtkPolyDataMapper> pathMapper;
        
        // 位置标记
        vtkSmartPointer<vtkSphereSource> positionMarker;
        vtkSmartPointer<vtkActor> markerActor;
        vtkSmartPointer<vtkPolyDataMapper> markerMapper;
        
        // 状态
        bool showPath;
        bool showMarker;
        bool isPlaying;
        double playSpeed;
        
        Impl() : cameraPath(nullptr), showPath(true), showMarker(true), 
                 isPlaying(false), playSpeed(1.0) {
            // 创建渲染器
            overviewRenderer = vtkSmartPointer<vtkRenderer>::New();
            endoscopeRenderer = vtkSmartPointer<vtkRenderer>::New();
            
            // 创建相机控制器
            cameraController = std::make_unique<CameraController>();
            
            // 创建模型管理器
            modelManager = std::make_unique<ModelManager>();
            
            // 关联相机到渲染器
            cameraController->AttachToRenderers(overviewRenderer, endoscopeRenderer);
            
            // 设置背景
            overviewRenderer->SetBackground(0.1, 0.1, 0.2);  // 深蓝色
            endoscopeRenderer->SetBackground(0.0, 0.0, 0.0);  // 黑色
        }
        
        void UpdateEndoscopeCamera() {
            if (!cameraPath || !cameraPath->GetCurrent() || !cameraController) return;
            
            PathNode* current = cameraPath->GetCurrent();
            
            // 使用CameraController更新相机
            cameraController->UpdateEndoscopeCamera(current);
            
            // 如果需要额外的调试信息，可以保留
            if (endoscopeRenderer) {
                // 检查渲染器中的actors
                vtkActorCollection* actors = endoscopeRenderer->GetActors();
                std::cout << "Number of actors in endoscope renderer: " 
                          << actors->GetNumberOfItems() << std::endl;
                
                // 获取渲染器的边界
                double bounds[6];
                endoscopeRenderer->ComputeVisiblePropBounds(bounds);
                std::cout << "Visible bounds: [" 
                          << bounds[0] << ", " << bounds[1] << "] ["
                          << bounds[2] << ", " << bounds[3] << "] ["
                          << bounds[4] << ", " << bounds[5] << "]" << std::endl;
            }
        }
        
        void UpdatePositionMarker() {
            if (!cameraPath || !cameraPath->GetCurrent() || !markerActor) return;
            
            PathNode* current = cameraPath->GetCurrent();
            
            // 通过设置actor的位置而不是修改数据源
            markerActor->SetPosition(current->position);
            
            // 标记渲染器需要更新（Modified会标记渲染器为dirty）
            overviewRenderer->Modified();
            
            // 如果窗口存在，立即渲染
            if (overviewWindow) {
                overviewWindow->Render();
            }
        }
    };

    BronchoscopyViewer::BronchoscopyViewer() : pImpl(std::make_unique<Impl>()) {
    }

    BronchoscopyViewer::~BronchoscopyViewer() = default;

    void BronchoscopyViewer::Initialize() {
        // 初始化位置标记
        pImpl->positionMarker = vtkSmartPointer<vtkSphereSource>::New();
        pImpl->positionMarker->SetCenter(0, 0, 0);  // 保持在原点
        pImpl->positionMarker->SetRadius(2.0);
        pImpl->positionMarker->SetPhiResolution(20);
        pImpl->positionMarker->SetThetaResolution(20);
        
        pImpl->markerMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        pImpl->markerMapper->SetInputConnection(pImpl->positionMarker->GetOutputPort());
        
        pImpl->markerActor = vtkSmartPointer<vtkActor>::New();
        pImpl->markerActor->SetMapper(pImpl->markerMapper);
        pImpl->markerActor->GetProperty()->SetColor(1.0, 0.0, 0.0);  // 红色
        
        // 只在overview中显示标记
        pImpl->overviewRenderer->AddActor(pImpl->markerActor);
    }

    // 文件读取由主程序负责，静态库只处理数据

    bool BronchoscopyViewer::LoadAirwayModel(vtkPolyData* polyData) {
        if (!polyData) return false;
        
        // 使用ModelManager加载模型
        if (!pImpl->modelManager->LoadModel(polyData)) {
            std::cerr << "Failed to load model in ModelManager" << std::endl;
            return false;
        }
        
        // 添加Actor到渲染器
        pImpl->modelManager->AddToRenderers(pImpl->overviewRenderer, pImpl->endoscopeRenderer);
        
        std::cout << "Overview renderer actors: " 
                  << pImpl->overviewRenderer->GetActors()->GetNumberOfItems() << std::endl;
        std::cout << "Endoscope renderer actors: " 
                  << pImpl->endoscopeRenderer->GetActors()->GetNumberOfItems() << std::endl;
        
        // 获取模型边界
        double bounds[6];
        pImpl->modelManager->GetModelBounds(bounds);
        
        // 使用CameraController重置相机
        pImpl->cameraController->ResetCameras(bounds);
        
        // 调试：打印相机状态
        std::cout << "After ResetCameras:" << std::endl;
        pImpl->cameraController->PrintCameraStatus();
        
        // 获取endoscope视口的实际大小
        int* size = pImpl->endoscopeRenderer->GetSize();
        std::cout << "  Endoscope renderer size: " << size[0] << " x " << size[1] << std::endl;
        
        std::cout << "=============================" << std::endl;
        
        return true;
    }

    bool BronchoscopyViewer::LoadCameraPath(const std::vector<double>& positions) {
        if (positions.size() < 6 || positions.size() % 3 != 0) {
            std::cerr << "Invalid path data: need at least 2 points (6 values)" << std::endl;
            return false;
        }
        
        CameraPath* path = new CameraPath();
        int numPoints = positions.size() / 3;
        
        // 处理每个点，自动计算方向
        for (int i = 0; i < numPoints; i++) {
            double pos[3] = {positions[i*3], positions[i*3+1], positions[i*3+2]};
            double dir[3];
            
            if (i < numPoints - 1) {
                // 非末尾点：方向指向下一个点
                double nextPos[3] = {positions[(i+1)*3], positions[(i+1)*3+1], positions[(i+1)*3+2]};
                for (int j = 0; j < 3; j++) {
                    dir[j] = nextPos[j] - pos[j];
                }
            } else {
                // 末尾点：使用前一个点的方向
                if (i > 0) {
                    double prevPos[3] = {positions[(i-1)*3], positions[(i-1)*3+1], positions[(i-1)*3+2]};
                    for (int j = 0; j < 3; j++) {
                        dir[j] = pos[j] - prevPos[j];
                    }
                } else {
                    // 只有一个点的特殊情况（不应该发生）
                    dir[0] = 0; dir[1] = 0; dir[2] = 1;
                }
            }
            
            // 归一化方向向量
            double length = std::sqrt(dir[0]*dir[0] + dir[1]*dir[1] + dir[2]*dir[2]);
            if (length > 0.0) {
                dir[0] /= length;
                dir[1] /= length;
                dir[2] /= length;
            }
            
            path->AddPoint(pos, dir);
        }
        
        return ApplyCameraPath(path);
    }

    // 内部辅助方法 - 应用已构建的路径对象
    bool BronchoscopyViewer::ApplyCameraPath(CameraPath* path) {
        if (!path || path->GetTotalNodes() == 0) return false;
        
        pImpl->cameraPath = path;
        
        // 生成路径可视化
        vtkPolyData* pathPolyData = path->GeneratePathTube(1.0);
        if (pathPolyData) {
            pImpl->pathMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            pImpl->pathMapper->SetInputData(pathPolyData);
            
            pImpl->pathActor = vtkSmartPointer<vtkActor>::New();
            pImpl->pathActor->SetMapper(pImpl->pathMapper);
            pImpl->pathActor->GetProperty()->SetColor(0.0, 1.0, 0.0);  // 绿色
            pImpl->pathActor->GetProperty()->SetOpacity(0.5);
            
            // 只在overview中显示路径
            pImpl->overviewRenderer->AddActor(pImpl->pathActor);
            
            // 释放pathPolyData（GeneratePathTube已经增加了引用计数）
            pathPolyData->UnRegister(nullptr);
        }
        
        // 更新相机位置
        pImpl->UpdateEndoscopeCamera();
        pImpl->UpdatePositionMarker();
        
        return true;
    }

    vtkRenderer* BronchoscopyViewer::GetOverviewRenderer() {
        return pImpl->overviewRenderer;
    }

    vtkRenderer* BronchoscopyViewer::GetEndoscopeRenderer() {
        return pImpl->endoscopeRenderer;
    }

    void BronchoscopyViewer::SetOverviewRenderWindow(vtkRenderWindow* window) {
        if (window) {
            pImpl->overviewWindow = window;
            pImpl->overviewWindow->AddRenderer(pImpl->overviewRenderer);
            std::cout << "Overview render window set, renderer added" << std::endl;
        }
    }

    void BronchoscopyViewer::SetEndoscopeRenderWindow(vtkRenderWindow* window) {
        if (window) {
            pImpl->endoscopeWindow = window;
            pImpl->endoscopeWindow->AddRenderer(pImpl->endoscopeRenderer);
            std::cout << "Endoscope render window set, renderer added" << std::endl;
            
            // 检查渲染器视口设置
            double* viewport = pImpl->endoscopeRenderer->GetViewport();
            std::cout << "Endoscope renderer viewport: [" 
                      << viewport[0] << ", " << viewport[1] << ", " 
                      << viewport[2] << ", " << viewport[3] << "]" << std::endl;
        }
    }

    void BronchoscopyViewer::MoveToNext() {
        if (pImpl->cameraPath && pImpl->cameraPath->MoveNext()) {
            pImpl->UpdateEndoscopeCamera();
            pImpl->UpdatePositionMarker();
        }
    }

    void BronchoscopyViewer::MoveToPrevious() {
        if (pImpl->cameraPath && pImpl->cameraPath->MovePrevious()) {
            pImpl->UpdateEndoscopeCamera();
            pImpl->UpdatePositionMarker();
        }
    }

    void BronchoscopyViewer::MoveToPosition(int index) {
        if (pImpl->cameraPath && pImpl->cameraPath->JumpTo(index)) {
            pImpl->UpdateEndoscopeCamera();
            pImpl->UpdatePositionMarker();
        }
    }

    void BronchoscopyViewer::ResetToStart() {
        if (pImpl->cameraPath) {
            pImpl->cameraPath->Reset();
            pImpl->UpdateEndoscopeCamera();
            pImpl->UpdatePositionMarker();
        }
    }

    void BronchoscopyViewer::SetAutoPlay(bool play) {
        pImpl->isPlaying = play;
    }

    void BronchoscopyViewer::SetPlaySpeed(double speed) {
        pImpl->playSpeed = speed;
    }

    int BronchoscopyViewer::GetCurrentPathIndex() const {
        return pImpl->cameraPath ? pImpl->cameraPath->GetCurrentIndex() : -1;
    }

    int BronchoscopyViewer::GetTotalPathNodes() const {
        return pImpl->cameraPath ? pImpl->cameraPath->GetTotalNodes() : 0;
    }

    bool BronchoscopyViewer::IsPlaying() const {
        return pImpl->isPlaying;
    }

    void BronchoscopyViewer::ShowPath(bool show) {
        pImpl->showPath = show;
        if (pImpl->pathActor) {
            pImpl->pathActor->SetVisibility(show);
        }
    }

    void BronchoscopyViewer::ShowPositionMarker(bool show) {
        pImpl->showMarker = show;
        if (pImpl->markerActor) {
            pImpl->markerActor->SetVisibility(show);
        }
    }

    void BronchoscopyViewer::SetPathColor(double r, double g, double b) {
        if (pImpl->pathActor) {
            pImpl->pathActor->GetProperty()->SetColor(r, g, b);
        }
    }

    void BronchoscopyViewer::SetMarkerColor(double r, double g, double b) {
        if (pImpl->markerActor) {
            pImpl->markerActor->GetProperty()->SetColor(r, g, b);
        }
    }

    void BronchoscopyViewer::SetPathOpacity(double opacity) {
        if (pImpl->pathActor) {
            pImpl->pathActor->GetProperty()->SetOpacity(opacity);
        }
    }

    void BronchoscopyViewer::UpdateVisualization() {
        Render();
    }

    void BronchoscopyViewer::Render() {
        // 直接渲染窗口
        if (pImpl->overviewWindow) {
            std::cout << "Rendering overview window..." << std::endl;
            pImpl->overviewWindow->Render();
        } else {
            std::cout << "WARNING: Overview window is null!" << std::endl;
        }
        
        if (pImpl->endoscopeWindow) {
            std::cout << "Rendering endoscope window..." << std::endl;
            
            // 调试：检查渲染器状态
            vtkActorCollection* actors = pImpl->endoscopeRenderer->GetActors();
            std::cout << "  Endoscope renderer has " << actors->GetNumberOfItems() << " actors" << std::endl;
            
            // 检查相机（通过CameraController）
            if (pImpl->cameraController) {
                vtkCamera* endoscopeCam = pImpl->cameraController->GetEndoscopeCamera();
                if (endoscopeCam) {
                    double* pos = endoscopeCam->GetPosition();
                    double* focal = endoscopeCam->GetFocalPoint();
                    std::cout << "  Camera pos: (" << pos[0] << ", " << pos[1] << ", " << pos[2] << ")" << std::endl;
                    std::cout << "  Camera focal: (" << focal[0] << ", " << focal[1] << ", " << focal[2] << ")" << std::endl;
                }
            }
            
            pImpl->endoscopeWindow->Render();
        } else {
            std::cout << "WARNING: Endoscope window is null!" << std::endl;
        }
    }

    void BronchoscopyViewer::ClearAll() {
        ClearModel();
        ClearPath();
    }

    void BronchoscopyViewer::ClearPath() {
        if (pImpl->pathActor) {
            pImpl->overviewRenderer->RemoveActor(pImpl->pathActor);
            pImpl->pathActor = nullptr;
        }
        pImpl->cameraPath = nullptr;
    }

    void BronchoscopyViewer::ClearModel() {
        if (pImpl->modelManager) {
            // 从渲染器移除Actor
            pImpl->modelManager->RemoveFromRenderers(pImpl->overviewRenderer, pImpl->endoscopeRenderer);
            // 清理模型
            pImpl->modelManager->ClearModel();
        }
    }

} // namespace BronchoscopyLib