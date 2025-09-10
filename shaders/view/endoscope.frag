varying vec3 normalVCVSOutput;
varying vec3 vertexVCVSOutput;

void main() {
    // 内窥镜视图 - 微红色调模拟真实组织
    vec3 baseColor = vec3(0.95, 0.75, 0.7);  // 微红肉色
    vec3 ambientColor = vec3(0.25, 0.2, 0.2); // 暖色调环境光
    
    // 光源方向
    vec3 lightVec = normalize(vec3(0.0, 0.0, 1.0));
    vec3 normal = normalize(normalVCVSOutput);
    
    // 漫反射
    float diffuseStrength = max(0.0, dot(normal, lightVec));
    vec3 diffuse = diffuseStrength * baseColor * 0.8;
    
    // 简单的镜面反射（模拟湿润表面）
    vec3 viewDir = normalize(-vertexVCVSOutput);
    vec3 reflectDir = reflect(-lightVec, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    vec3 specular = vec3(0.15, 0.1, 0.1) * spec;
    
    // 组合
    vec3 finalColor = ambientColor + diffuse + specular;
    gl_FragColor = vec4(finalColor, 1.0);
}