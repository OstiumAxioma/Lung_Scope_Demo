varying vec3 normalVCVSOutput;
varying vec3 vertexVCVSOutput;

void main() {
    vec3 ambientColor = vec3(0.3, 0.3, 0.3);
    vec3 diffuseColor = vec3(0.8, 0.8, 0.8);
    float opacity = 1.0;
    
    vec3 lightVec = normalize(vec3(0.0, 0.0, 1.0));
    vec3 normal = normalize(normalVCVSOutput);
    
    float df = max(0.0, dot(normal, lightVec));
    vec3 diffuse = df * diffuseColor;
    
    gl_FragColor = vec4(ambientColor + diffuse, opacity);
}