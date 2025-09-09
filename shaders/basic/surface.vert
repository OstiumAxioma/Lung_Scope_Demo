attribute vec4 vertexMC;
attribute vec3 normalMC;

uniform mat4 MCDCMatrix;
uniform mat4 MCVCMatrix; 
uniform mat3 normalMatrix;

varying vec3 normalVCVSOutput;
varying vec3 vertexVCVSOutput;

void main() {
    normalVCVSOutput = normalMatrix * normalMC;
    vertexVCVSOutput = vec3(MCVCMatrix * vertexMC);
    gl_Position = MCDCMatrix * vertexMC;
}