#include "CameraPath.h"
#include <cmath>
#include <iostream>

// VTK头文件
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkPolyLine.h>
#include <vtkTubeFilter.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>

namespace BronchoscopyLib {

    CameraPath::CameraPath() 
        : head(nullptr), tail(nullptr), current(nullptr), nodeCount(0) {
    }

    CameraPath::~CameraPath() {
        Clear();
    }

    void CameraPath::Clear() {
        PathNode* node = head;
        while (node != nullptr) {
            PathNode* next = node->next;
            delete node;
            node = next;
        }
        head = tail = current = nullptr;
        nodeCount = 0;
        // 清理样条数据
        splineValid = false;
        splineSamplesPerSegment = 0;
        splinePositions.clear();
        splineTangents.clear();
        segmentSampleOffsets.clear();
    }

    void CameraPath::AddPoint(double x, double y, double z, 
                              double dx, double dy, double dz) {
        double pos[3] = {x, y, z};
        double dir[3] = {dx, dy, dz};
        AddPoint(pos, dir);
    }

    void CameraPath::AddPoint(const double pos[3], const double dir[3]) {
        PathNode* newNode = new PathNode();
        
        // 复制位置
        for (int i = 0; i < 3; i++) {
            newNode->position[i] = pos[i];
            newNode->direction[i] = dir[i];
        }
        
        // 归一化方向向量
        NormalizeVector(newNode->direction);
        
        // 更新链表
        if (head == nullptr) {
            head = tail = current = newNode;
        } else {
            tail->next = newNode;
            newNode->prev = tail;
            tail = newNode;
        }
        
        nodeCount++;

        // 路径变更，样条失效
        splineValid = false;
    }

    bool CameraPath::MoveNext() {
        if (current != nullptr && current->next != nullptr) {
            current = current->next;
            return true;
        }
        return false;
    }

    bool CameraPath::MovePrevious() {
        if (current != nullptr && current->prev != nullptr) {
            current = current->prev;
            return true;
        }
        return false;
    }

    void CameraPath::Reset() {
        current = head;
    }

    bool CameraPath::JumpTo(int index) {
        if (index < 0 || index >= nodeCount) {
            return false;
        }
        
        current = head;
        for (int i = 0; i < index && current != nullptr; i++) {
            current = current->next;
        }
        
        return current != nullptr;
    }

    int CameraPath::GetCurrentIndex() const {
        if (current == nullptr) return -1;
        
        int index = 0;
        PathNode* node = head;
        while (node != nullptr && node != current) {
            node = node->next;
            index++;
        }
        
        return index;
    }

    bool CameraPath::IsAtEnd() const {
        return current != nullptr && current->next == nullptr;
    }

    bool CameraPath::IsAtStart() const {
        return current != nullptr && current == head;
    }

    // 文件I/O由主程序实现，静态库不处理

    vtkPolyData* CameraPath::GeneratePathPolyData() const {
        if (nodeCount < 2) return nullptr;
        
        vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
        vtkSmartPointer<vtkPolyLine> polyLine = vtkSmartPointer<vtkPolyLine>::New();
        
        if (splineValid && !splinePositions.empty()) {
            int total = static_cast<int>(splinePositions.size() / 3);
            polyLine->GetPointIds()->SetNumberOfIds(total);
            for (int i = 0; i < total; ++i) {
                int base = i * 3;
                points->InsertNextPoint(splinePositions[base+0], splinePositions[base+1], splinePositions[base+2]);
                polyLine->GetPointIds()->SetId(i, i);
            }
        } else {
            polyLine->GetPointIds()->SetNumberOfIds(nodeCount);
            PathNode* node = head;
            int index = 0;
            while (node != nullptr) {
                points->InsertNextPoint(node->position);
                polyLine->GetPointIds()->SetId(index, index);
                node = node->next;
                index++;
            }
        }
        
        vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
        cells->InsertNextCell(polyLine);
        
        vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
        polyData->SetPoints(points);
        polyData->SetLines(cells);
        
        // 增加引用计数，防止返回后被释放
        polyData->Register(nullptr);
        return polyData;
    }

    vtkPolyData* CameraPath::GeneratePathTube(double radius) const {
        vtkPolyData* pathPolyData = GeneratePathPolyData();
        if (pathPolyData == nullptr) return nullptr;
        
        vtkSmartPointer<vtkTubeFilter> tubeFilter = vtkSmartPointer<vtkTubeFilter>::New();
        tubeFilter->SetInputData(pathPolyData);
        tubeFilter->SetRadius(radius);
        tubeFilter->SetNumberOfSides(12);
        tubeFilter->CappingOn();
        tubeFilter->Update();
        
        vtkPolyData* output = tubeFilter->GetOutput();
        // 增加引用计数，防止返回后被释放
        output->Register(nullptr);
        // 释放pathPolyData（GeneratePathPolyData已经增加了引用计数）
        pathPolyData->UnRegister(nullptr);
        return output;
    }

    void CameraPath::GetInterpolatedPosition(double t, double pos[3]) const {
        if (nodeCount == 0) return;
        
        t = std::max(0.0, std::min(1.0, t));
        
        if (nodeCount == 1) {
            for (int i = 0; i < 3; i++) {
                pos[i] = head->position[i];
            }
            return;
        }
        
        double totalLength = CalculatePathLength();
        double targetLength = t * totalLength;
        double currentLength = 0.0;
        
        PathNode* node = head;
        while (node->next != nullptr) {
            double segmentLength = 0.0;
            for (int i = 0; i < 3; i++) {
                double diff = node->next->position[i] - node->position[i];
                segmentLength += diff * diff;
            }
            segmentLength = std::sqrt(segmentLength);
            
            if (currentLength + segmentLength >= targetLength) {
                double localT = (targetLength - currentLength) / segmentLength;
                for (int i = 0; i < 3; i++) {
                    pos[i] = node->position[i] + 
                            localT * (node->next->position[i] - node->position[i]);
                }
                return;
            }
            
            currentLength += segmentLength;
            node = node->next;
        }
        
        // 如果到达末尾
        for (int i = 0; i < 3; i++) {
            pos[i] = tail->position[i];
        }
    }

    void CameraPath::GetInterpolatedDirection(double t, double dir[3]) const {
        if (nodeCount == 0) return;
        
        t = std::max(0.0, std::min(1.0, t));
        
        if (nodeCount == 1) {
            for (int i = 0; i < 3; i++) {
                dir[i] = head->direction[i];
            }
            return;
        }
        
        double totalLength = CalculatePathLength();
        double targetLength = t * totalLength;
        double currentLength = 0.0;
        
        PathNode* node = head;
        while (node->next != nullptr) {
            double segmentLength = 0.0;
            for (int i = 0; i < 3; i++) {
                double diff = node->next->position[i] - node->position[i];
                segmentLength += diff * diff;
            }
            segmentLength = std::sqrt(segmentLength);
            
            if (currentLength + segmentLength >= targetLength) {
                double localT = (targetLength - currentLength) / segmentLength;
                for (int i = 0; i < 3; i++) {
                    dir[i] = node->direction[i] + 
                            localT * (node->next->direction[i] - node->direction[i]);
                }
                // 归一化方向向量（内联实现，避免调用非const方法）
                double length = std::sqrt(dir[0]*dir[0] + dir[1]*dir[1] + dir[2]*dir[2]);
                if (length > 0.0) {
                    dir[0] /= length;
                    dir[1] /= length;
                    dir[2] /= length;
                }
                return;
            }
            
            currentLength += segmentLength;
            node = node->next;
        }
        
        // 如果到达末尾
        for (int i = 0; i < 3; i++) {
            dir[i] = tail->direction[i];
        }
    }

    void CameraPath::NormalizeVector(double vec[3]) {
        double length = std::sqrt(vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2]);
        if (length > 0.0) {
            vec[0] /= length;
            vec[1] /= length;
            vec[2] /= length;
        }
    }

    double CameraPath::CalculatePathLength() const {
        double totalLength = 0.0;
        PathNode* node = head;
        
        while (node != nullptr && node->next != nullptr) {
            double segmentLength = 0.0;
            for (int i = 0; i < 3; i++) {
                double diff = node->next->position[i] - node->position[i];
                segmentLength += diff * diff;
            }
            totalLength += std::sqrt(segmentLength);
            node = node->next;
        }
        
        return totalLength;
    }

    // 获取第index个节点的位置（0-based）
    void CameraPath::GetNodePosition(int index, double out[3]) const {
        if (index < 0) index = 0;
        if (index >= nodeCount) index = nodeCount - 1;
        PathNode* node = head;
        int i = 0;
        while (node && i < index) {
            node = node->next;
            ++i;
        }
        if (!node) {
            out[0] = out[1] = out[2] = 0.0;
            return;
        }
        out[0] = node->position[0];
        out[1] = node->position[1];
        out[2] = node->position[2];
    }

    // Catmull-Rom 样条评估：段i（P1=Pi, P2=Pi+1），u in [0,1]
    void CameraPath::EvalCatmullRom(int i, double u, double outPos[3], double outTan[3]) const {
        // 端点处理：使用边界点重复
        int n = nodeCount;
        int i0 = std::max(0, i - 1);
        int i1 = std::max(0, i);
        int i2 = std::min(n - 1, i + 1);
        int i3 = std::min(n - 1, i + 2);

        double P0[3], P1[3], P2[3], P3[3];
        GetNodePosition(i0, P0);
        GetNodePosition(i1, P1);
        GetNodePosition(i2, P2);
        GetNodePosition(i3, P3);

        double u2 = u * u;
        double u3 = u2 * u;

        // 位置：0.5 * (2P1 + (-P0 + P2)u + (2P0 - 5P1 + 4P2 - P3)u^2 + (-P0 + 3P1 - 3P2 + P3)u^3)
        for (int k = 0; k < 3; ++k) {
            double a = 2.0 * P1[k];
            double b = -P0[k] + P2[k];
            double c = 2.0*P0[k] - 5.0*P1[k] + 4.0*P2[k] - P3[k];
            double d = -P0[k] + 3.0*P1[k] - 3.0*P2[k] + P3[k];
            outPos[k] = 0.5 * (a + b*u + c*u2 + d*u3);
        }

        if (outTan) {
            // 切向为导数：0.5 * (b + 2c u + 3d u^2)
            for (int k = 0; k < 3; ++k) {
                double b = -P0[k] + P2[k];
                double c = 2.0*P0[k] - 5.0*P1[k] + 4.0*P2[k] - P3[k];
                double d = -P0[k] + 3.0*P1[k] - 3.0*P2[k] + P3[k];
                outTan[k] = 0.5 * (b + 2.0*c*u + 3.0*d*u2);
            }
            // 归一化
            double len = std::sqrt(outTan[0]*outTan[0] + outTan[1]*outTan[1] + outTan[2]*outTan[2]);
            if (len > 1e-8) {
                outTan[0] /= len; outTan[1] /= len; outTan[2] /= len;
            }
        }
    }

    void CameraPath::GenerateSpline(int samplesPerSegment) {
        splinePositions.clear();
        splineTangents.clear();
        segmentSampleOffsets.clear();

        if (nodeCount < 2) {
            splineValid = false;
            return;
        }

        int segments = nodeCount - 1;
        splineSamplesPerSegment = std::max(2, samplesPerSegment);
        segmentSampleOffsets.resize(segments + 1);

        int totalSamples = segments * splineSamplesPerSegment + 1; // 包含最后端点
        splinePositions.resize(totalSamples * 3);
        splineTangents.resize(totalSamples * 3);

        int sampleIndex = 0;
        for (int seg = 0; seg < segments; ++seg) {
            segmentSampleOffsets[seg] = sampleIndex;
            for (int s = 0; s < splineSamplesPerSegment; ++s) {
                double u = static_cast<double>(s) / static_cast<double>(splineSamplesPerSegment);
                double pos[3], tan[3];
                EvalCatmullRom(seg, u, pos, tan);
                int base = sampleIndex * 3;
                splinePositions[base+0] = pos[0];
                splinePositions[base+1] = pos[1];
                splinePositions[base+2] = pos[2];
                splineTangents[base+0] = tan[0];
                splineTangents[base+1] = tan[1];
                splineTangents[base+2] = tan[2];
                ++sampleIndex;
            }
        }
        // 末端点（u=1）
        segmentSampleOffsets[segments] = sampleIndex;
        double posEnd[3], tanEnd[3];
        EvalCatmullRom(segments-1, 1.0, posEnd, tanEnd);
        int base = sampleIndex * 3;
        splinePositions[base+0] = posEnd[0];
        splinePositions[base+1] = posEnd[1];
        splinePositions[base+2] = posEnd[2];
        splineTangents[base+0] = tanEnd[0];
        splineTangents[base+1] = tanEnd[1];
        splineTangents[base+2] = tanEnd[2];

        splineValid = true;
    }

    void CameraPath::GetSplinePosDirBetween(int segmentIndex, double u, double pos[3], double dir[3]) const {
        if (!splineValid || nodeCount < 2) {
            // 回退到线性插值
            double p1[3], p2[3];
            GetNodePosition(segmentIndex, p1);
            GetNodePosition(segmentIndex+1, p2);
            u = std::max(0.0, std::min(1.0, u));
            for (int i = 0; i < 3; ++i) pos[i] = p1[i] + (p2[i] - p1[i]) * u;
            double d[3] = {p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2]};
            double len = std::sqrt(d[0]*d[0]+d[1]*d[1]+d[2]*d[2]);
            if (len > 1e-8) { dir[0]=d[0]/len; dir[1]=d[1]/len; dir[2]=d[2]/len; }
            else { dir[0]=1; dir[1]=0; dir[2]=0; }
            return;
        }

        int segments = nodeCount - 1;
        if (segmentIndex < 0) segmentIndex = 0;
        if (segmentIndex >= segments) segmentIndex = segments - 1;

        // 使用预采样进行线性插值，近似弧长等速
        u = std::max(0.0, std::min(1.0, u));
        int startOffset = segmentSampleOffsets[segmentIndex];
        int segSamples = splineSamplesPerSegment;
        double f = u * segSamples;
        int sIdx = static_cast<int>(std::floor(f));
        double localU = f - sIdx;

        // 边界处理
        if (sIdx >= segSamples) { sIdx = segSamples - 1; localU = 1.0; }

        int a = startOffset + sIdx;
        int b = a + 1;
        // 防止越界（最后一段的最后一个样本后还有终点）
        int maxSampleIndex = (int)(splinePositions.size()/3) - 1;
        if (b > maxSampleIndex) b = maxSampleIndex;

        int abase = a * 3;
        int bbase = b * 3;
        for (int i = 0; i < 3; ++i) {
            pos[i] = splinePositions[abase+i] + (splinePositions[bbase+i] - splinePositions[abase+i]) * localU;
            dir[i] = splineTangents[abase+i] + (splineTangents[bbase+i] - splineTangents[abase+i]) * localU;
        }
        // 归一化方向
        double len = std::sqrt(dir[0]*dir[0] + dir[1]*dir[1] + dir[2]*dir[2]);
        if (len > 1e-8) { dir[0]/=len; dir[1]/=len; dir[2]/=len; }
    }

    void CameraPath::GetSplinePosDirGlobal(double t, double pos[3], double dir[3]) const {
        if (!splineValid || splinePositions.size() < 6) {
            // 回退：使用线性按路径长度的插值
            GetInterpolatedPosition(t, pos);
            GetInterpolatedDirection(t, dir);
            return;
        }
        double tt = std::max(0.0, std::min(1.0, t));
        int total = static_cast<int>(splinePositions.size() / 3);
        double idxf = tt * (total - 1);
        int i = static_cast<int>(std::floor(idxf));
        double f = idxf - i;
        if (i >= total - 1) { i = total - 2; f = 1.0; }
        int ia = i * 3;
        int ib = (i + 1) * 3;
        for (int k = 0; k < 3; ++k) {
            pos[k] = splinePositions[ia+k] + (splinePositions[ib+k] - splinePositions[ia+k]) * f;
            dir[k] = splineTangents[ia+k] + (splineTangents[ib+k] - splineTangents[ia+k]) * f;
        }
        double len = std::sqrt(dir[0]*dir[0] + dir[1]*dir[1] + dir[2]*dir[2]);
        if (len > 1e-8) { dir[0]/=len; dir[1]/=len; dir[2]/=len; }
    }

} // namespace BronchoscopyLib
