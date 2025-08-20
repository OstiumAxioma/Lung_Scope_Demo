# 尝试过的缩放中心修复方案记录

## ❌ 失败方案列表

### 1. QPainter 变换方案
**尝试内容**：
```cpp
paint.save();
paint.translate(center);
paint.scale(m_scale, m_scale);
paint.translate(-center);
paint.drawImage(m_imgRct, m_curQImg);
paint.restore();
```
**失败原因**：
- 会影响十字线等其他元素的坐标计算
- 十字线位置会错位

### 2. 修改 m_imgRct 位置方案
**尝试内容**：
```cpp
int drawWidth = m_glRect.width() * m_scale;
int drawHeight = m_glRect.height() * m_scale;
int centerX = m_glRect.x() + m_glRect.width() / 2;
int centerY = m_glRect.y() + m_glRect.height() / 2;
m_imgRct = QRect(centerX - drawWidth / 2, centerY - drawHeight / 2, drawWidth, drawHeight);
```
**失败原因**：
- 误解了缩放机制
- m_imgRct 应该保持固定大小

### 3. 在 scale() 函数中调整 viewRect 方案（多次尝试）
**尝试内容 v1**：
```cpp
double centerX = viewRect.x() + (dicom.width / oldScale) / 2.0;
double centerY = viewRect.y() + (dicom.height / oldScale) / 2.0;
int newX = centerX - (dicom.width / v) / 2.0;
int newY = centerY - (dicom.height / v) / 2.0;
ImgVmFactory::stGetInstance()->setViewRect(m_type, newX, newY, viewRect.width(), viewRect.height());
```

**尝试内容 v2**（基于Qt ImageViewer示例）：
```cpp
double factor = v / m_scale;
double pageStepX = dicom.width / m_scale;
double pageStepY = dicom.height / m_scale;
int newX = factor * viewRect.x() + ((factor - 1) * pageStepX / 2);
int newY = factor * viewRect.y() + ((factor - 1) * pageStepY / 2);
```

**失败原因**：
- 仍然往左上角缩放
- 缩放到最小后图像可能消失且无法恢复

### 4. 采样偏移方案（在 paintEvent 中）
**尝试内容**：
```cpp
// 在采样时添加偏移
int offsetX = (dicom.width - img_width) / 2;
int offsetY = (dicom.height - img_height) / 2;
int iterx = std::max(viewRect.x() + offsetX, 0);
int itery = std::max(viewRect.y() + offsetY, 0);
// 同时调整坐标映射
int x1 = x - viewRect.x() - offsetX;
int y1 = y - viewRect.y() - offsetY;
```
**失败原因**：
- 导致 out of range 错误
- CORONAL 和 SAGITTAL 视图坐标系统不同，统一处理有问题
- 偏移计算逻辑有误

### 5. 记录 m_prevScale 方案
**尝试内容**：
添加 `m_prevScale` 成员变量来记录之前的缩放值
**失败原因**：
- 不必要的复杂性，当前 m_scale 已经足够

## ✅ 当前正在尝试的方案

### QImage 中心填充方案
**思路**：
- 保持 QImage 大小不变（img_width × img_height）
- 当缩小时，QImage 比实际数据大
- 计算偏移量，让数据从 QImage 的中心开始填充
- 偏移量 = (QImage大小 - 实际采样大小) / 2

**实现**：
```cpp
int qimageOffsetX = 0;
int qimageOffsetY = 0;
if (m_scale < 1.0) {
    qimageOffsetX = (img_width - iterWidth + viewRect.x()) / 2;
    qimageOffsetY = (img_height - iterHeight + viewRect.y()) / 2;
}
// 填充时使用偏移
int x1 = x - viewRect.x() + qimageOffsetX;
int y1 = y - viewRect.y() + qimageOffsetY;
```

## 问题根本原因分析

1. **缩放机制**：使用采样缩放，不是变换缩放
2. **关键问题**：QImage 大小和实际填充数据大小不匹配
   - 缩小时：QImage 很大，但只在左上角填充数据
   - 导致图像偏向左上角

## 注意事项

1. **不能修改的内容**：
   - viewRect（视口位置）
   - m_imgRct（绘制目标，应保持固定）
   - 十字线坐标系统

2. **坐标系差异**：
   - AXIAL 使用 `x - viewRect.x()`
   - CORONAL/SAGITTAL 使用 `x - iterx`
   - 需要分别处理

3. **边界检查**：
   - 任何偏移计算都要确保不会导致数组越界
   - 特别是缩放到极值时