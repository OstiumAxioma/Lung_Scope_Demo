#version 120

// FXAA顶点着色器（全屏四边形）
attribute vec4 vertexMC;

varying vec2 texCoord;

void main() {
    gl_Position = vertexMC;
    texCoord = (vertexMC.xy + 1.0) * 0.5;
}