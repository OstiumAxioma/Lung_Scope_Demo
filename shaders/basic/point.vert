attribute vec4 vertexMC;

uniform mat4 MCDCMatrix;

void main() {
    gl_Position = MCDCMatrix * vertexMC;
    gl_PointSize = 10.0;
}