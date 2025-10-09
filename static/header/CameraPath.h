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
        
        // 获取特定位置的插值（线性分段）
        void GetInterpolatedPosition(double t, double pos[3]) const;
        void GetInterpolatedDirection(double t, double dir[3]) const;

        // 样条曲线支持（Catmull-Rom）
        // 预计算样条采样点，用于平滑过渡
        void GenerateSpline(int samplesPerSegment = 30);
        bool HasSpline() const { return splineValid; }
        int GetSegmentCount() const { return nodeCount > 1 ? (nodeCount - 1) : 0; }
        // 在给定段index与局部参数u(0..1)上获取样条位置与切向（方向）
        void GetSplinePosDirBetween(int segmentIndex, double u, double pos[3], double dir[3]) const;
        
    private:
        PathNode* head;
        PathNode* tail;
        PathNode* current;
        int nodeCount;
        
        // 辅助函数
        void NormalizeVector(double vec[3]);
        double CalculatePathLength() const;

        // 样条内部数据
        bool splineValid = false;
        int splineSamplesPerSegment = 0;
        // 扁平化存储：positions和tangents均为(x,y,z)顺序排列
        std::vector<double> splinePositions;   // size = 3 * totalSamples
        std::vector<double> splineTangents;    // size = 3 * totalSamples
        // 每个段的起始样本在扁平数组中的索引（按样本编号，而非三元组索引）
        // 大小为 GetSegmentCount()+1，最后一个元素是总样本数
        std::vector<int> segmentSampleOffsets;

        // 计算Catmull-Rom样条点和切向
        void EvalCatmullRom(int i, double u, double outPos[3], double outTan[3]) const;
        // 按索引访问节点位置
        void GetNodePosition(int index, double out[3]) const;
    };

} // namespace BronchoscopyLib

#endif // CAMERA_PATH_H
