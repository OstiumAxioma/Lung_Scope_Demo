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