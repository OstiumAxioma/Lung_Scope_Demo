#ifndef CAMERA_PATH_H
#define CAMERA_PATH_H

#include <vector>

// 前向声明VTK类
class vtkPolyData;
class vtkPoints;
class vtkCellArray;

namespace BronchoscopyLib {

    // 路径节点结构
    struct PathNode {
        double position[3];     // 位置坐标
        double direction[3];    // 朝向方向（归一化向量）
        PathNode* next;
        PathNode* prev;
        
        PathNode() : next(nullptr), prev(nullptr) {
            for(int i = 0; i < 3; i++) {
                position[i] = 0.0;
                direction[i] = 0.0;
            }
        }
    };

    class CameraPath {
    public:
        CameraPath();
        ~CameraPath();
        
        // 路径操作
        void AddPoint(double x, double y, double z, 
                     double dx, double dy, double dz);
        void AddPoint(const double pos[3], const double dir[3]);
        void Clear();
        
        // 导航控制
        bool MoveNext();
        bool MovePrevious();
        void Reset();
        bool JumpTo(int index);
        
        // 获取当前状态
        PathNode* GetCurrent() const { return current; }
        PathNode* GetHead() const { return head; }
        int GetCurrentIndex() const;
        int GetTotalNodes() const { return nodeCount; }
        bool IsAtEnd() const;
        bool IsAtStart() const;
        
        // 文件I/O由主程序负责，静态库不处理
        
        // 可视化
        vtkPolyData* GeneratePathPolyData() const;
        vtkPolyData* GeneratePathTube(double radius = 1.0) const;
        
        // 获取特定位置的插值
        void GetInterpolatedPosition(double t, double pos[3]) const;
        void GetInterpolatedDirection(double t, double dir[3]) const;
        
    private:
        PathNode* head;
        PathNode* tail;
        PathNode* current;
        int nodeCount;
        
        // 辅助函数
        void NormalizeVector(double vec[3]);
        double CalculatePathLength() const;
    };

} // namespace BronchoscopyLib

#endif // CAMERA_PATH_H