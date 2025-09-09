#version 120

// 简化的FXAA片段着色器
uniform sampler2D screenTexture;
uniform vec2 screenSize;

varying vec2 texCoord;

void main() {
    // 暂时只是pass-through，后期实现FXAA算法
    vec4 color = texture2D(screenTexture, texCoord);
    
    // TODO: 实现FXAA抗锯齿
    // 1. 亮度计算
    // 2. 边缘检测
    // 3. 子像素抗锯齿
    
    gl_FragColor = color;
}