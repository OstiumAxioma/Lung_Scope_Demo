# VTK静态库模板项目

## 环境要求
- Visual Studio 2022
- CMake 3.20+
- VTK 8.2.0
- Qt 5.14.2（VTK需要）

## 配置方法

### 方法1：使用config.local.cmake（推荐）
1. 复制 `config.cmake` 为 `config.local.cmake`
2. 编辑 `config.local.cmake`，取消注释并设置正确的路径：
   ```cmake
   set(Qt5_DIR "C:/Qt/Qt5.14.2/5.14.2/msvc2017_64/lib/cmake/Qt5")
   set(VTK_DIR "D:/code/vtk8.2.0/VTK-8.2.0/lib/cmake/vtk-8.2")
   ```
3. 运行 `build.bat`

### 方法2：使用命令行参数
直接运行build.bat时传入路径：
```batch
build.bat "C:/Qt/Qt5.14.2/5.14.2/msvc2017_64" "D:/code/vtk8.2.0/VTK-8.2.0/lib/cmake/vtk-8.2"
```

### 方法3：设置环境变量
在系统环境变量中设置：
- `Qt5_DIR` = `C:/Qt/Qt5.14.2/5.14.2/msvc2017_64/lib/cmake/Qt5`
- `VTK_DIR` = `D:/code/vtk8.2.0/VTK-8.2.0/lib/cmake/vtk-8.2`

### 方法4：自动检测
如果Qt和VTK安装在常见路径，CMake会尝试自动检测。支持的路径包括：
- Qt: `C:/Qt/Qt5.14.2/`, `C:/Qt/5.14.2/`, `D:/Qt/Qt5.14.2/` 等
- VTK: `D:/code/vtk8.2.0/VTK-8.2.0/`, `C:/VTK/`, `C:/Program Files/VTK/` 等

## 编译选项
运行 `build.bat` 后会提示选择编译配置：
1. Debug - 调试版本
2. Release - 发布版本
3. Both - 同时编译两个版本（默认）

## 输出文件
编译成功后：
- 静态库文件：`../lib/TemplateLib.lib`（Release）和 `../lib/TemplateLib_d.lib`（Debug）
- 头文件：`../header/template.h`

## 故障排除
1. **找不到Qt5**：确保Qt5路径正确，并包含msvc2017_64编译器版本
2. **找不到VTK**：确保VTK已正确编译并安装
3. **编译失败**：检查Visual Studio 2022是否正确安装

## 在其他项目中使用
1. 将 `lib` 文件夹中的静态库添加到你的项目
2. 包含 `header` 文件夹中的头文件
3. 确保链接了VTK和Qt5的依赖库