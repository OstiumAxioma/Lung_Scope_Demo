varying vec3 normalVCVSOutput;
varying vec3 vertexVCVSOutput;

void main() {
    // 硬编码颜色值用于测试
    vec3 baseColor = vec3(0.85, 0.85, 0.8);  // 略带灰色的白色
    vec3 ambientColor = vec3(0.3, 0.3, 0.3); // 环境光
    
    // 光源方向（从观察者方向）
    vec3 lightVec = normalize(vec3(0.0, 0.0, 1.0));
    vec3 normal = normalize(normalVCVSOutput);
    
    // Phong光照模型
    // 1. 漫反射
    float diffuseStrength = max(0.0, dot(normal, lightVec));
    vec3 diffuse = diffuseStrength * baseColor * 0.7;
    
    // 2. 镜面反射
    vec3 viewDir = normalize(-vertexVCVSOutput);
    vec3 reflectDir = reflect(-lightVec, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = vec3(0.2) * spec;
    
    // 3. 边缘光（轮廓增强）
    float rim = 1.0 - max(dot(viewDir, normal), 0.0);
    rim = smoothstep(0.6, 1.0, rim);
    vec3 rimLight = vec3(0.1, 0.1, 0.15) * rim;
    
    // 组合所有光照
    vec3 finalColor = ambientColor + diffuse + specular + rimLight;
    gl_FragColor = vec4(finalColor, 1.0);
}