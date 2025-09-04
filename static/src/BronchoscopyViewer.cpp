#include "BronchoscopyViewer.h"
#include "CameraPath.h"

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
        
        // 相机
        vtkSmartPointer<vtkCamera> overviewCamera;
        vtkSmartPointer<vtkCamera> endoscopeCamera;
        
        // 模型数据
        vtkSmartPointer<vtkPolyData> airwayModel;
        vtkSmartPointer<vtkPolyDataMapper> modelMapper;
        vtkSmartPointer<vtkActor> modelActorForOverview;
        vtkSmartPointer<vtkActor> modelActorForEndoscope;
        
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
            
            // 创建相机
            overviewCamera = vtkSmartPointer<vtkCamera>::New();
            endoscopeCamera = vtkSmartPointer<vtkCamera>::New();
            
            // 设置相机
            overviewRenderer->SetActiveCamera(overviewCamera);
            endoscopeRenderer->SetActiveCamera(endoscopeCamera);
            
            // 设置背景
            overviewRenderer->SetBackground(0.1, 0.1, 0.2);  // 深蓝色
            endoscopeRenderer->SetBackground(0.0, 0.0, 0.0);  // 黑色
        }
        
        void UpdateEndoscopeCamera() {
            if (!cameraPath || !cameraPath->GetCurrent()) return;
            
            PathNode* current = cameraPath->GetCurrent();
            
            // 调试输出：路径点坐标
            std::cout << "=== UpdateEndoscopeCamera Debug ===" << std::endl;
            std::cout << "Path point position: (" 
                      << current->position[0] << ", " 
                      << current->position[1] << ", " 
                      << current->position[2] << ")" << std::endl;
            std::cout << "Path point direction: (" 
                      << current->direction[0] << ", " 
                      << current->direction[1] << ", " 
                      << current->direction[2] << ")" << std::endl;
            
            // 设置相机位置
            endoscopeCamera->SetPosition(current->position);
            
            // 计算焦点位置（沿着方向向前看）
            double focalPoint[3];
            for (int i = 0; i < 3; i++) {
                focalPoint[i] = current->position[i] + 10.0 * current->direction[i];
            }
            endoscopeCamera->SetFocalPoint(focalPoint);
            
            // 设置上方向（假设Y轴向上）
            endoscopeCamera->SetViewUp(0, 1, 0);
            
            // 设置视场角
            endoscopeCamera->SetViewAngle(60);
            
            // 重置相机裁剪范围
            endoscopeRenderer->ResetCameraClippingRange();
            
            // 调试输出：相机状态
            double* camPos = endoscopeCamera->GetPosition();
            double* camFocal = endoscopeCamera->GetFocalPoint();
            double* camUp = endoscopeCamera->GetViewUp();
            double* clippingRange = endoscopeCamera->GetClippingRange();
            
            std::cout << "Camera position: (" 
                      << camPos[0] << ", " << camPos[1] << ", " << camPos[2] << ")" << std::endl;
            std::cout << "Camera focal point: (" 
                      << camFocal[0] << ", " << camFocal[1] << ", " << camFocal[2] << ")" << std::endl;
            std::cout << "Camera view up: (" 
                      << camUp[0] << ", " << camUp[1] << ", " << camUp[2] << ")" << std::endl;
            std::cout << "Camera FOV: " << endoscopeCamera->GetViewAngle() << std::endl;
            std::cout << "Camera clipping range: [" 
                      << clippingRange[0] << ", " << clippingRange[1] << "]" << std::endl;
            
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
            std::cout << "===================================" << std::endl;
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
        
        std::cout << "\n=== LoadAirwayModel Debug ===" << std::endl;
        std::cout << "PolyData points: " << polyData->GetNumberOfPoints() << std::endl;
        std::cout << "PolyData cells: " << polyData->GetNumberOfCells() << std::endl;
        
        // 深拷贝polyData，确保数据的生命周期独立于外部
        pImpl->airwayModel = vtkSmartPointer<vtkPolyData>::New();
        pImpl->airwayModel->DeepCopy(polyData);
        
        // 为每个渲染器创建独立的mapper
        // Overview的mapper
        vtkSmartPointer<vtkPolyDataMapper> overviewMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        overviewMapper->SetInputData(pImpl->airwayModel);
        overviewMapper->ScalarVisibilityOff();
        
        // Endoscope的mapper
        vtkSmartPointer<vtkPolyDataMapper> endoscopeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        endoscopeMapper->SetInputData(pImpl->airwayModel);
        endoscopeMapper->ScalarVisibilityOff();
        
        // 为overview创建actor
        pImpl->modelActorForOverview = vtkSmartPointer<vtkActor>::New();
        pImpl->modelActorForOverview->SetMapper(overviewMapper);
        pImpl->modelActorForOverview->GetProperty()->SetColor(0.8, 0.8, 0.9);
        pImpl->modelActorForOverview->GetProperty()->SetOpacity(0.7);
        
        // 为endoscope创建actor
        pImpl->modelActorForEndoscope = vtkSmartPointer<vtkActor>::New();
        pImpl->modelActorForEndoscope->SetMapper(endoscopeMapper);
        pImpl->modelActorForEndoscope->GetProperty()->SetColor(0.9, 0.7, 0.7);
        
        // 添加到渲染器
        pImpl->overviewRenderer->AddActor(pImpl->modelActorForOverview);
        pImpl->endoscopeRenderer->AddActor(pImpl->modelActorForEndoscope);
        
        std::cout << "Actors added to renderers" << std::endl;
        std::cout << "Overview renderer actors: " 
                  << pImpl->overviewRenderer->GetActors()->GetNumberOfItems() << std::endl;
        std::cout << "Endoscope renderer actors: " 
                  << pImpl->endoscopeRenderer->GetActors()->GetNumberOfItems() << std::endl;
        
        // 获取模型边界
        double bounds[6];
        polyData->GetBounds(bounds);
        std::cout << "Model bounds: [" 
                  << bounds[0] << ", " << bounds[1] << "] ["
                  << bounds[2] << ", " << bounds[3] << "] ["
                  << bounds[4] << ", " << bounds[5] << "]" << std::endl;
        
        // 重置两个相机，让它们都能看到整个模型
        pImpl->overviewRenderer->ResetCamera();
        pImpl->endoscopeRenderer->ResetCamera();
        
        // 调试：打印ResetCamera后的实际结果
        std::cout << "After ResetCamera for both renderers:" << std::endl;
        
        // 获取endoscope相机的裁剪范围
        double* clipRange = pImpl->endoscopeCamera->GetClippingRange();
        std::cout << "  Endoscope clipping range: [" << clipRange[0] << ", " << clipRange[1] << "]" << std::endl;
        
        // 获取endoscope视口的实际大小
        int* size = pImpl->endoscopeRenderer->GetSize();
        std::cout << "  Endoscope renderer size: " << size[0] << " x " << size[1] << std::endl;
        
        // 调试输出两个相机的状态
        std::cout << "Camera object pointers:" << std::endl;
        std::cout << "  Overview camera: " << pImpl->overviewCamera << std::endl;
        std::cout << "  Endoscope camera: " << pImpl->endoscopeCamera << std::endl;
        std::cout << "  Are they the same? " << (pImpl->overviewCamera == pImpl->endoscopeCamera ? "YES" : "NO") << std::endl;
        
        // 验证渲染器的ActiveCamera
        std::cout << "Renderer's active cameras:" << std::endl;
        std::cout << "  Overview renderer's camera: " << pImpl->overviewRenderer->GetActiveCamera() << std::endl;
        std::cout << "  Endoscope renderer's camera: " << pImpl->endoscopeRenderer->GetActiveCamera() << std::endl;
        
        double* overviewPos = pImpl->overviewCamera->GetPosition();
        double* endoscopePos = pImpl->endoscopeCamera->GetPosition();
        std::cout << "After ResetCamera:" << std::endl;
        std::cout << "  Overview camera position: (" 
                  << overviewPos[0] << ", " << overviewPos[1] << ", " << overviewPos[2] << ")" << std::endl;
        std::cout << "  Endoscope camera position: (" 
                  << endoscopePos[0] << ", " << endoscopePos[1] << ", " << endoscopePos[2] << ")" << std::endl;
        
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
            
            // 检查相机
            double* pos = pImpl->endoscopeCamera->GetPosition();
            double* focal = pImpl->endoscopeCamera->GetFocalPoint();
            std::cout << "  Camera pos: (" << pos[0] << ", " << pos[1] << ", " << pos[2] << ")" << std::endl;
            std::cout << "  Camera focal: (" << focal[0] << ", " << focal[1] << ", " << focal[2] << ")" << std::endl;
            
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
        if (pImpl->modelActorForOverview) {
            pImpl->overviewRenderer->RemoveActor(pImpl->modelActorForOverview);
            pImpl->modelActorForOverview = nullptr;
        }
        if (pImpl->modelActorForEndoscope) {
            pImpl->endoscopeRenderer->RemoveActor(pImpl->modelActorForEndoscope);
            pImpl->modelActorForEndoscope = nullptr;
        }
        pImpl->airwayModel = nullptr;
    }

} // namespace BronchoscopyLib