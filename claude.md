# 支气管镜导航可视化系统 - 项目记录

## 项目概述
基于VTK 8.2.0和Qt 5.12.9的支气管镜导航可视化系统，用于显示气管3D模型并模拟内窥镜导航。

## 核心设计决策

### 1. 架构设计
- **静态库架构**: 核心功能封装在BronchoscopyLib静态库中
- **单场景双相机**: 不是双渲染器，而是使用同一个场景的两个不同相机视角
- **数据处理分离**: 静态库绝不处理文件I/O，所有文件操作由主程序完成

### 2. 关键实现细节

#### 文件I/O分离
```cpp
// 错误示例（静态库不应该这样做）
bool LoadModel(const std::string& filePath); // ❌

// 正确示例（静态库只接收数据）
bool LoadAirwayModel(vtkPolyData* polyData); // ✅
```

#### 路径方向自动计算
- 输入：只需要点序列 `std::vector<double> positions`
- 方向计算：
  - 中间点：方向指向下一个点
  - 末尾点：使用前一个点的方向
  - 归一化处理

#### 红球位置更新（重要修复）
```cpp
// 错误方式（会导致渲染问题）
positionMarker->SetCenter(current->position); // ❌

// 正确方式（只移动actor）
markerActor->SetPosition(current->position); // ✅
```

## 主要问题和解决历程

### 1. 函数命名冲突
- **问题**: 两个LoadCameraPath函数（public和private）造成混淆
- **解决**: 私有方法重命名为ApplyCameraPath

### 2. 标签大小问题
- **问题**: 标签占满整个窗口
- **解决**: 使用setFixedHeight(25)而非setFixedHeight(600)

### 3. 渲染刷新问题（未完全解决）
- **现象**: 加载后需要手动操作视图才能看到内容
- **尝试的方案**:
  - 直接调用Render() - 导致模型消失
  - 使用Qt的update() - 无效
  - 使用交互器的Render() - 仍有问题
- **当前状态**: 需要用户手动操作视图触发渲染

### 4. 内存管理
- **问题**: VTK智能指针返回原始指针导致崩溃
- **解决**: 使用Register/UnRegister手动管理引用计数

### 5. Actor更新问题
- **问题**: 修改SphereSource导致渲染混乱
- **解决**: 保持数据源不变，只移动actor位置

## 文件结构

### 静态库（/static）
- `BronchoscopyViewer.h/cpp`: 主视图管理器，处理双窗口渲染
- `CameraPath.h/cpp`: 相机路径管理，使用双向链表

### 主程序（/src, /header）
- `mainwindow.cpp`: 处理所有文件I/O，包括模型和路径加载
- 使用QSplitter实现双窗口布局

## 关键类接口

### BronchoscopyViewer
- `Initialize()`: 初始化标记和渲染器
- `LoadAirwayModel(vtkPolyData*)`: 加载3D模型数据
- `LoadCameraPath(std::vector<double>&)`: 加载路径点序列
- `MoveToNext/Previous()`: 导航控制
- `GetOverviewRenderer/GetEndoscopeRenderer()`: 获取渲染器

### CameraPath
- 双向链表实现
- `AddPoint()`: 添加路径点
- `MoveNext/Previous()`: 链表导航
- `GeneratePathTube()`: 生成可视化路径

## 待解决问题

1. **渲染刷新机制**: 需要找到正确的方式在数据加载后自动刷新显示
2. **链表导航问题**: 在某些点之间可能出现循环
3. **内窥镜相机初始化**: 右视图相机可能未正确显示

## 重要提醒

1. **永远不要在静态库中进行文件I/O操作**
2. **修改actor位置而非数据源**
3. **路径必须至少包含2个点**
4. **VTK引用计数需要特别注意**

## 测试数据
- `path_l.txt`: 包含12个路径点的测试文件
- 格式: 每行三个数字（x y z），空格分隔

## 编译步骤
1. `cd static && build.bat` (选择3-Both)
2. `cd .. && build.bat`
3. 运行: `build/Exe/Release/VTK_Qt_Project.exe`

## 最近更改
- 2024-xx-xx: 将STL格式支持替换为OBJ格式
  - 修改文件: `src/mainwindow.cpp`
  - 包含 `vtkOBJReader.h` 替代 `vtkSTLReader.h`
  - 更新文件对话框过滤器

- 2025-01-10: 实现自定义Shader系统
  - 创建了ShaderSystem类用于管理shader效果
  - **重要教训**: VTK中正确使用自定义shader的方法
    - ❌ 错误方法: 使用`SetVertexShaderCode()`和`SetFragmentShaderCode()`完全替换shader
    - ✅ 正确方法: 使用`AddShaderReplacement()`替换特定的shader块
  - 详见下方"Shader系统实现"章节

## Bug修复记录

### 1. 内窥镜视图黑屏问题
**症状**: 右侧endoscope视图始终黑屏，左侧视图模型在移动后消失  
**原因**: polyData直接赋值而非深拷贝；两个Actor共享同一个Mapper  
**修复**: 使用`DeepCopy`；为每个Actor创建独立的Mapper  
**文件**: `static/src/BronchoscopyViewer.cpp:196-220`

### 2. 红球位置更新问题
**症状**: 红色位置标记需要手动旋转视图才更新  
**原因**: 修改SphereSource的中心位置导致渲染混乱  
**修复**: 保持SphereSource在原点，通过`actor->SetPosition()`移动  
**文件**: `static/src/BronchoscopyViewer.cpp:115`

### 3. 标签占满窗口问题
**症状**: 视图标签占据整个窗口高度  
**原因**: 错误使用`setFixedHeight(600)`  
**修复**: 改为`setFixedHeight(25)`  
**文件**: `src/mainwindow.cpp:195,207`

### 4. 编译错误：const方法调用非const
**症状**: GetInterpolatedDirection中调用NormalizeVector编译失败  
**原因**: const方法不能调用非const的NormalizeVector  
**修复**: 内联归一化计算代码  
**文件**: `static/src/CameraPath.cpp:247-252`

### 5. 路径加载崩溃
**症状**: 加载路径文件时程序崩溃  
**原因**: VTK对象引用计数管理错误  
**修复**: 使用`Register/UnRegister`手动管理引用计数  
**文件**: `static/src/CameraPath.cpp:148,165-167`

### 6. Shader导致模型全白无阴影问题
**症状**: 使用自定义shader后，模型显示为纯白色，无光照效果  
**原因**: 错误使用`SetVertexShaderCode/SetFragmentShaderCode`完全替换了VTK的shader  
**修复**: 改用`AddShaderReplacement`只替换特定shader块，保留VTK默认光照  
**文件**: `static/src/ShaderSystem.cpp:235-306`

## Shader系统实现

### 关键认识
VTK的shader系统不是简单的"替换整个shader"，而是基于**shader块替换**的机制。

### 错误方法（导致全白渲染）
```cpp
// ❌ 完全替换shader - 破坏了VTK的渲染管线
mapper->SetVertexShaderCode(customVertexShader);
mapper->SetFragmentShaderCode(customFragmentShader);
```

### 正确方法（保留VTK光照）
```cpp
// ✅ 替换特定shader块 - 保留VTK默认功能
mapper->AddShaderReplacement(
    vtkShader::Fragment,
    "//VTK::Light::Impl",  // 替换光照实现块
    false,  // 在标准替换之后
    "//VTK::Light::Impl\n"  // 保留默认光照
    "  // 自定义代码\n"
    "  vec3 rimLight = ...\n"
    "  fragOutput0.rgb += rimLight;\n",
    false  // 只做一次
);
```

### VTK Shader替换点
- `//VTK::System::Dec` - 系统声明
- `//VTK::Normal::Dec` - 法线相关声明
- `//VTK::Color::Impl` - 颜色实现
- `//VTK::Light::Impl` - 光照实现
- `//VTK::Output::Dec` - 输出声明

### 当前实现
- **Overview视图**: 增强边缘光照效果
- **Endoscope视图**: 微红色调模拟真实组织

### 参考资源
- VTK官方示例: SpatterShader
- 关键函数: `vtkOpenGLPolyDataMapper::AddShaderReplacement()`