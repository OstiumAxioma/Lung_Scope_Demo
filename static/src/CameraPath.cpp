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
        
        polyLine->GetPointIds()->SetNumberOfIds(nodeCount);
        
        PathNode* node = head;
        int index = 0;
        while (node != nullptr) {
            points->InsertNextPoint(node->position);
            polyLine->GetPointIds()->SetId(index, index);
            node = node->next;
            index++;
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

} // namespace BronchoscopyLib