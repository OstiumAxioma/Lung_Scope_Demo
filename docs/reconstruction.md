  1. CameraController.h/cpp - 相机管理模块

  负责内容：
  - 双相机的创建和管理
  - 相机位置更新
  - 相机参数设置

  从BronchoscopyViewer.cpp迁移的代码：
  // 行 43-44: 相机对象
  vtkSmartPointer<vtkCamera> overviewCamera;
  vtkSmartPointer<vtkCamera> endoscopeCamera;

  // 行 75-76: 相机创建
  overviewCamera = vtkSmartPointer<vtkCamera>::New();
  endoscopeCamera = vtkSmartPointer<vtkCamera>::New();

  // 行 87-151: UpdateEndoscopeCamera() 整个函数
  void UpdateEndoscopeCamera() { ... }

  // 行 233-246: LoadAirwayModel中的相机重置部分
  pImpl->overviewRenderer->ResetCamera();
  pImpl->endoscopeRenderer->ResetCamera();
  // ... 相机调试输出

  2. RenderingEngine.h/cpp - 渲染管理模块

  负责内容：
  - 渲染器创建和管理
  - 渲染窗口关联
  - 渲染执行

  从BronchoscopyViewer.cpp迁移的代码：
  // 行 37-40: 渲染器和窗口对象
  vtkSmartPointer<vtkRenderer> overviewRenderer;
  vtkSmartPointer<vtkRenderer> endoscopeRenderer;
  vtkSmartPointer<vtkRenderWindow> overviewWindow;
  vtkSmartPointer<vtkRenderWindow> endoscopeWindow;

  // 行 71-72: 渲染器创建
  overviewRenderer = vtkSmartPointer<vtkRenderer>::New();
  endoscopeRenderer = vtkSmartPointer<vtkRenderer>::New();

  // 行 79-84: 渲染器设置
  overviewRenderer->SetActiveCamera(overviewCamera);
  endoscopeRenderer->SetActiveCamera(endoscopeCamera);
  overviewRenderer->SetBackground(0.1, 0.1, 0.2);
  endoscopeRenderer->SetBackground(0.0, 0.0, 0.0);

  // 行 336-356: SetOverviewRenderWindow和SetEndoscopeRenderWindow函数
  // 行 443-469: Render()函数

  3. ModelManager.h/cpp - 模型管理模块

  负责内容：
  - 气管模型的加载和管理
  - Actor和Mapper的创建

  从BronchoscopyViewer.cpp迁移的代码：
  // 行 47-50: 模型相关对象
  vtkSmartPointer<vtkPolyData> airwayModel;
  vtkSmartPointer<vtkPolyDataMapper> modelMapper;
  vtkSmartPointer<vtkActor> modelActorForOverview;
  vtkSmartPointer<vtkActor> modelActorForEndoscope;

  // 行 189-268: LoadAirwayModel()函数的主体部分
  // (除了相机重置部分，那部分属于CameraController)

  4. PathVisualization.h/cpp - 路径和标记可视化模块

  负责内容：
  - 路径的可视化（绿色管道）
  - 位置标记（红球）的管理
  - 路径和标记的显示控制

  从BronchoscopyViewer.cpp迁移的代码：
  // 行 53-60: 路径和标记相关对象
  CameraPath* cameraPath;
  vtkSmartPointer<vtkActor> pathActor;
  vtkSmartPointer<vtkPolyDataMapper> pathMapper;
  vtkSmartPointer<vtkSphereSource> positionMarker;
  vtkSmartPointer<vtkActor> markerActor;
  vtkSmartPointer<vtkPolyDataMapper> markerMapper;

  // 行 153-168: UpdatePositionMarker()函数
  // 行 174-185: Initialize()中的标记初始化部分
  // 行 270-326: LoadCameraPath()和ApplyCameraPath()函数
  // 行 427-438: ShowPath()和ShowPositionMarker()函数
  // 行 441-456: SetPathColor(), SetMarkerColor(), SetPathOpacity()函数

  5. NavigationController.h/cpp - 导航控制模块

  负责内容：
  - 路径导航（前进/后退/跳转）
  - 自动播放控制

  从BronchoscopyViewer.cpp迁移的代码：
  // 行 53: CameraPath对象的导航部分
  CameraPath* cameraPath; // (导航功能部分)

  // 行 387-414: MoveToNext(), MoveToPrevious(), MoveToPosition(), ResetToStart()
  // 行 416-424: SetAutoPlay(), SetPlaySpeed(), IsPlaying()
  // 行 376-385: GetCurrentPathIndex(), GetTotalPathNodes()

  6. SceneManager.h/cpp - 场景管理器

  负责内容：
  - 协调所有模块
  - 管理对象之间的关系
  - 场景的清理

  从BronchoscopyViewer.cpp迁移的代码：
  // 行 63-66: 状态管理
  bool showPath;
  bool showMarker;
  bool isPlaying;
  double playSpeed;

  // 行 471-485: ClearAll(), ClearPath(), ClearModel()函数
  // 各模块之间的协调逻辑

  7. BronchoscopyAPI.h/cpp - 对外统一接口

  新创建的内容：
  - 封装所有模块，提供简洁的对外接口
  - 维护与现有BronchoscopyViewer相似的接口
  - 内部调用各个模块

  接口设计示例：
  class BronchoscopyAPI {
  private:
      std::unique_ptr<CameraController> m_cameraController;
      std::unique_ptr<RenderingEngine> m_renderingEngine;
      std::unique_ptr<ModelManager> m_modelManager;
      std::unique_ptr<PathVisualization> m_pathViz;
      std::unique_ptr<NavigationController> m_navigation;
      std::unique_ptr<SceneManager> m_sceneManager;

  public:
      // 保持原有的公共接口不变
      bool LoadAirwayModel(vtkPolyData* polyData);
      bool LoadCameraPath(const std::vector<double>& positions);
      void MoveToNext();
      // ... 其他接口
  };

  实施顺序建议

  1. 第一步：创建CameraController（最独立）
  2. 第二步：创建ModelManager
  3. 第三步：创建PathVisualization
  4. 第四步：创建RenderingEngine
  5. 第五步：创建NavigationController
  6. 第六步：创建SceneManager协调各模块
  7. 第七步：创建BronchoscopyAPI作为统一接口
  8. 第八步：修改主程序使用新API