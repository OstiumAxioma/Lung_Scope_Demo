# 支气管镜导航可视化系统

这是一个基于VTK 8.2.0和Qt 5.12.9的支气管镜导航可视化系统，采用静态库架构设计。用于显示气管3D模型并模拟内窥镜导航。

## 项目特点

- **双窗口显示**: 左侧全局视图显示完整气管模型，右侧内窥镜视图显示相机所见
- **静态库架构**: 核心渲染功能封装在BronchoscopyLib静态库中
- **数据处理分离**: 静态库只处理数据，不涉及文件I/O，所有文件读取由主程序负责
- **相机路径导航**: 支持沿预定路径导航，自动计算方向向量
- **实时渲染更新**: 支持路径导航时的视图实时更新

## 项目结构

```
Lung_demo/
├── CMakeLists.txt          # 主项目CMake配置
├── build.bat              # 主程序构建脚本
├── README.md              # 项目说明
├── header/                # 主程序头文件
│   └── mainwindow.h       # 主窗口头文件
├── src/                   # 主程序源文件
│   ├── main.cpp          # 程序入口
│   ├── mainwindow.cpp    # 主窗口实现（处理文件I/O）
│   └── mainwindow.ui     # Qt Designer UI文件
├── lib/                   # 静态库输出目录
│   ├── BronchoscopyLib.lib   # Release版静态库
│   └── BronchoscopyLib_d.lib # Debug版静态库
├── static/                # 静态库项目
│   ├── CMakeLists.txt    # 静态库CMake配置
│   ├── build.bat         # 静态库构建脚本
│   ├── header/           # 静态库头文件
│   │   ├── BronchoscopyViewer.h  # 主视图管理器接口
│   │   └── CameraPath.h          # 相机路径管理
│   └── src/              # 静态库源文件
│       ├── BronchoscopyViewer.cpp  # 双视图渲染实现
│       └── CameraPath.cpp          # 路径导航实现
├── path_l.txt             # 测试用路径数据
└── generate_vscode_config.py  # VSCode配置生成器
```

## 环境要求

- **VTK**: 8.2.0 (路径: `D:/code/vtk8.2.0/VTK-8.2.0`)
- **Qt**: 5.12.9 (路径: `C:/Qt/Qt5.12.9/5.12.9/msvc2017_64`)
- **编译器**: Visual Studio 2022
- **CMake**: 3.14 或更高版本

## 构建步骤

### 1. 编译静态库

```bash
cd static
build.bat
# 选择构建配置：
# 1 - Debug
# 2 - Release  
# 3 - Both (推荐)
```

静态库将被自动复制到 `lib/` 目录。

### 2. 编译主程序

```bash
# 返回根目录
cd ..
build.bat
```

可执行文件位置: `build/Exe/Release/VTK_Qt_Project.exe`

## 核心架构说明

### 重要设计原则

1. **静态库不处理文件I/O**: 所有文件读取操作（模型文件、路径文件）必须由主程序完成，静态库只接收处理后的数据
2. **单场景双相机**: 使用同一个3D场景，通过两个不同的相机（全局相机和内窥镜相机）渲染到两个窗口
3. **路径方向自动计算**: 输入路径只需提供点序列，方向向量由系统自动计算

### BronchoscopyLib::BronchoscopyViewer 类

静态库提供的主要接口类，管理双窗口渲染：

```cpp
class BronchoscopyViewer {
public:
    // 初始化
    void Initialize();
    
    // 数据加载（只接收数据，不处理文件）
    bool LoadAirwayModel(vtkPolyData* polyData);  // 加载气管模型
    bool LoadCameraPath(const std::vector<double>& positions);  // 加载路径点序列
    
    // 获取渲染器
    vtkRenderer* GetOverviewRenderer();   // 左窗口：全局视图
    vtkRenderer* GetEndoscopeRenderer();  // 右窗口：内窥镜视图
    
    // 设置渲染窗口
    void SetOverviewRenderWindow(vtkRenderWindow* window);
    void SetEndoscopeRenderWindow(vtkRenderWindow* window);
    
    // 相机路径导航
    void MoveToNext();
    void MoveToPrevious();
    void MoveToPosition(int index);
    void ResetToStart();
    
    // 可视化控制
    void ShowPath(bool show);
    void ShowPositionMarker(bool show);
    void SetPathColor(double r, double g, double b);
    void SetMarkerColor(double r, double g, double b);
};
```

### 使用示例

```cpp
// mainwindow.cpp中的文件加载示例
void MainWindow::loadAirwayModel() {
    QString fileName = QFileDialog::getOpenFileName(/*...*/);
    
    // 主程序负责文件读取
    vtkSmartPointer<vtkPolyDataReader> reader = vtkSmartPointer<vtkPolyDataReader>::New();
    reader->SetFileName(fileName.toStdString().c_str());
    reader->Update();
    
    // 传递数据给静态库（不传递文件路径）
    bronchoscopyViewer->LoadAirwayModel(reader->GetOutput());
}

void MainWindow::loadCameraPath() {
    QString fileName = QFileDialog::getOpenFileName(/*...*/);
    
    // 主程序负责文件解析
    std::vector<double> positions;
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            double x, y, z;
            in >> x >> y >> z;
            positions.push_back(x);
            positions.push_back(y);
            positions.push_back(z);
        }
    }
    
    // 传递点序列给静态库（方向自动计算）
    bronchoscopyViewer->LoadCameraPath(positions);
}
```

## 功能特性

### 核心功能
- **双窗口3D可视化**: 左侧全局视图，右侧内窥镜视图
- **气管模型加载**: 支持VTK、VTP、STL格式的3D模型
- **路径导航**: 沿预定路径模拟内窥镜移动
- **实时位置显示**: 红色球体标记当前位置，绿色管道显示路径
- **自动播放**: 支持自动沿路径移动

### 支持的文件格式
- **3D模型**: .vtk, .vtp, .stl
- **路径文件**: .txt, .csv (每行包含x y z坐标)

### 用户交互

#### 鼠标操作
- **左键拖拽**: 旋转视角
- **中键拖拽**: 平移视角
- **滚轮**: 缩放

#### 键盘快捷键
- **Ctrl+O**: 加载气管模型
- **Ctrl+L**: 加载相机路径
- **←/→**: 前进/后退导航
- **R**: 重置到起点
- **Space**: 播放/暂停

#### 菜单功能
- **文件**
  - 加载模型 (Ctrl+O)
  - 加载路径 (Ctrl+L)
  - 退出 (Ctrl+Q)
- **导航**
  - 上一步/下一步
  - 重置位置
  - 自动播放
- **视图**
  - 显示/隐藏路径
  - 显示/隐藏位置标记

## 已知问题和注意事项

### 当前存在的问题

1. **渲染刷新问题**: 
   - 加载模型或路径后需要手动操作视图（如旋转）才能看到内容
   - 导航时位置更新也需要手动操作视图才能刷新
   - 原因：直接调用Render()会导致某些模型消失，目前未找到完美解决方案

2. **内存管理**: 
   - VTK对象使用了手动引用计数管理（Register/UnRegister）
   - 路径数据生成时需要特别注意引用计数

3. **路径导航限制**:
   - 路径文件必须至少包含2个点
   - 方向向量自动从点序列计算，末尾点使用前一个点的方向

### 开发注意事项

1. **文件I/O原则**:
   - 静态库绝不能包含文件读写代码
   - 所有文件操作必须在主程序中完成
   - 静态库只接收vtkPolyData*或std::vector<double>等数据结构

2. **渲染更新**:
   - 避免在数据更新后立即调用Render()
   - 使用Qt的update()可能无效
   - 用户交互（如鼠标操作）会自动触发正确的渲染

3. **Actor管理**:
   - 红球位置更新应使用actor->SetPosition()而非修改数据源
   - 避免频繁修改vtkSource对象的参数

## 故障排除

### 编译错误

1. **找不到BronchoscopyLib.lib**
   - 确保先编译静态库 (`cd static && build.bat`)
   - 检查 `lib/` 目录是否存在静态库文件
   - 选择编译配置3（Both）以同时生成Debug和Release版本

2. **VTK相关错误**
   - 确认VTK_DIR路径正确: `D:/code/vtk8.2.0/VTK-8.2.0`
   - VTK版本必须是8.2.0
   - 确保VTK编译时启用了Qt支持

3. **Qt相关错误**
   - 确认Qt路径正确: `C:/Qt/Qt5.12.9/5.12.9/msvc2017_64`
   - Qt版本必须是5.12.9
   - 确保使用的是MSVC 2017编译器版本

### 运行时问题

1. **程序崩溃**
   - 检查路径文件格式是否正确（每行三个数字，空格分隔）
   - 确保3D模型文件格式正确且未损坏

2. **模型不显示**
   - 手动旋转一下视图触发渲染
   - 检查模型文件是否正确加载
   - 确认模型坐标范围是否合理

3. **导航不工作**
   - 确保先加载模型，再加载路径
   - 检查路径文件是否至少包含2个点

### VSCode IntelliSense错误

如果VSCode显示波浪线错误但编译成功：

1. 运行 `python generate_vscode_config.py` 生成配置
2. 或安装CMake Tools扩展
3. 重启VSCode

## 测试数据

项目包含测试用路径文件 `path_l.txt`，格式示例：
```
61.337109 51.810612 34.851875
62.119164 43.638584 23.498497
60.742504 31.629404 16.486897
...
```

每行包含一个3D坐标点（x y z），至少需要2个点。

## 开发路线图

### 待解决的问题
- [ ] 渲染刷新机制优化
- [ ] 链表导航循环问题修复
- [ ] 内窥镜视图相机初始化
- [ ] 自动播放功能完善

### 计划添加的功能
- [ ] 路径编辑功能
- [ ] 多路径支持
- [ ] 碰撞检测
- [ ] 测量工具
- [ ] 截图和录制功能

## 许可证

本项目仅供学习和研究使用。