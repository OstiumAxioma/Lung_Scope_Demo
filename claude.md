# 项目结构

## 编译架构
- **主程序**: `D:\Project\Lung_demo\` → 生成 `build\Exe\VTK_Qt_Project.exe`
- **静态库**: `D:\Project\Lung_demo\static\` → 生成 `static\build\lib\TemplateLib.lib`

## 编译步骤
1. `cd static && build.bat` - 编译静态库
2. 拷贝 `static\build\lib\TemplateLib*.lib` → `lib\`
3. `build.bat` - 编译主程序

## 关键文件
### 主程序
- `src/main.cpp` - 程序入口
- `src/mainwindow.cpp` - Qt窗口实现
- `header/mainwindow.h` - Qt窗口接口

### 静态库
- `static/src/template.cpp` - 核心功能实现
- `static/header/template.h` - 对外接口

## 依赖
- Qt5 (Core, Gui, Widgets)
- VTK 8.2
- MSVC编译器

## 开发任务
### 支气管腔镜可视化
1. **双窗口显示**
   - 左: 气管模型正视图 + 相机位置/路径高亮
   - 右: 腔镜视角（相机内部视图）

2. **数据导入**
   - 气管模型网格(polydata)
   - 相机路径点(链表存储)

3. **实现分工**
   - 主程序: 文件选择UI、用户交互
   - 静态库: 可视化核心逻辑、数据处理