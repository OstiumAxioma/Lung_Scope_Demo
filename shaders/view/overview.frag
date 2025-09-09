varying vec3 normalVCVSOutput;
varying vec3 vertexVCVSOutput;

void main() {
    vec3 ambientColor = vec3(0.3, 0.3, 0.3);
    vec3 diffuseColor = vec3(0.8, 0.8, 0.8);
    
    // 增强的光照
    vec3 lightVec = normalize(vec3(0.0, 0.0, 1.0));
    vec3 normal = normalize(normalVCVSOutput);
    
    // 漫反射
    float df = max(0.0, dot(normal, lightVec));
    vec3 diffuse = df * diffuseColor;
    
    // 边缘光（轮廓增强）
    vec3 viewDir = normalize(-vertexVCVSOutput);
    float rim = 1.0 - max(dot(viewDir, normal), 0.0);
    rim = smoothstep(0.6, 1.0, rim);
    vec3 rimLight = vec3(0.2) * rim;
    
    gl_FragColor = vec4(ambientColor + diffuse + rimLight, 1.0);
}