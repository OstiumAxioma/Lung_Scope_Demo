# GlWdgImpl 变换矩阵方案实现日志

## 实现概述
将VTK相机变换的思路应用到GlWdgImpl的QPainter绘制系统中，通过引入变换矩阵概念解决中心缩放问题。

## 核心修改

### 1. 头文件 (GlWdgImpl.h)

#### 新增成员变量：
```cpp
// 模拟相机的变换参数
QPointF m_viewCenter = QPointF(0, 0);    // 视图中心（图像像素坐标）
double m_zoomFactor = 1.0;               // 缩放因子
QPointF m_panOffset = QPointF(0, 0);     // 平移偏移
```

#### 新增函数声明：
```cpp
void updateViewTransform();     // 更新视图变换
void initializeViewCenter();    // 初始化视图中心
QRect calculateViewRect();      // 计算viewRect
```

### 2. 实现文件 (GlWdgImpl.cpp)

#### 构造函数初始化：
- 初始化m_zoomFactor = 1.0
- 初始化m_panOffset = QPointF(0, 0)
- 初始化m_viewCenter = QPointF(0, 0)

#### 核心函数实现：

1. **initializeViewCenter()**: 将视图中心设置为图像中心
2. **calculateViewRect()**: 根据变换参数计算新的viewRect
3. **updateViewTransform()**: 更新viewRect并保持兼容性
4. **scale()**: 使用新的变换系统实现中心缩放
5. **translate()**: 使用新的变换系统实现平移

## 设计原理

### 变换思路：
1. **统一变换中心**: 所有操作都围绕m_viewCenter进行
2. **分离关注点**:
   - m_zoomFactor: 纯缩放操作
   - m_panOffset: 纯平移操作
   - m_viewCenter: 变换基准点
3. **兼容现有架构**: 通过计算生成兼容的viewRect和m_scale

### 中心缩放算法：
```
新viewRect.x = m_viewCenter.x - (视图宽度/2) * unitView/spacing + m_panOffset.x
新viewRect.y = m_viewCenter.y - (视图高度/2) * unitView/spacing + m_panOffset.y
```

### 关键优势：
- 保持现有QPainter绘制系统不变
- 保持ImgVmFactory接口兼容
- 实现真正的中心缩放
- 支持缩放后平移

## 测试要点

1. **初始加载**: 视图中心应设置为图像中心
2. **中心缩放**: 放大/缩小时图像应围绕视图中心进行
3. **平移功能**: 平移后再缩放仍应以当前视图中心为基准
4. **边界处理**: 缩放和平移不应超出图像边界
5. **三视图兼容**: 轴位、冠状、矢状视图都应正常工作

## 向后兼容性

- 保持m_scale变量用于兼容性
- 保持现有的viewRect接口
- 保持现有的绘制逻辑不变
- 只修改变换计算逻辑

## 预期效果

解决原有的"缩放向左上角偏移"问题，实现类似VTK相机系统的真正中心缩放效果。

## 问题修复记录

### 修复1：缩小时向左上角偏移
**问题**：放大正常，但缩小时仍向左上角偏移
**原因**：边界检查逻辑不当，缩小时限制了可视范围
**解决方案**：
```cpp
// 区分放大和缩小的边界检查
if (m_zoomFactor > 1.0) {
    // 放大时限制范围
    maxViewX = static_cast<int>(dicom.width - centerOffsetX * 2);
    maxViewY = static_cast<int>(dicom.height - centerOffsetY * 2);
} else {
    // 缩小时允许更大范围
    maxViewX = dicom.width - 1;
    maxViewY = dicom.height - 1;
}
```

### 修复2：平移上下方向反了
**问题**：鼠标向上拖拽时图像向下移动，方向相反
**原因**：Qt坐标系Y轴向下，但用户期望Y轴向上
**解决方案**：
```cpp
// 在calculateViewRect中对Y方向取反
int viewY = static_cast<int>(m_viewCenter.y() - centerOffsetY - m_panOffset.y());
```

### 修复3：混合缩放算法实现
**问题**：需要结合两种方案的优势，确保放大和缩小都能正确中心缩放
**解决方案**：
- 采用增量偏移算法（来自GlWdgImpl_zoomout.cpp）作为核心算法
- 保留边界检查和安全性改进
- 使用公式：`offsetX = (1.0 - 1.0/scaleFactor) * m_glRect.width() / 2`
- 直接操作viewRect，避免与transform matrix计算冲突

```cpp
// 混合方案：结合增量偏移算法和变换矩阵方案
double scaleFactor = v / oldScale;
int offsetX = (int)((1.0 - 1.0/scaleFactor) * m_glRect.width() / 2);
int offsetY = (int)((1.0 - 1.0/scaleFactor) * m_glRect.height() / 2);
int newX = viewRect.x() + offsetX;
int newY = viewRect.y() + offsetY;
```

### 当前状态
- ✅ 放大时中心缩放正常
- ✅ 缩小时中心缩放正常（使用增量偏移算法修复）
- ✅ 平移方向正确（上下左右都正常）
- ✅ 边界检查合理
- ✅ 混合算法实现完成