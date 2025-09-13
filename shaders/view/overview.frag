# Overview View Shader  
# 全局视图 - 清晰的PBR渲染

//VTK::Light::Impl AFTER
//VTK::Light::Impl
  
  // ============ 全局视图增强 ============
  
  vec3 color = fragOutput0.rgb;
  
  // --- 1. 边缘增强 ---
  
  vec3 N = normalize(normalVCVSOutput);
  vec3 V = normalize(-vertexVC.xyz);
  float NdotV = max(dot(N, V), 0.0);
  
  // 边缘检测
  float edge = 1.0 - NdotV;
  edge = pow(edge, 1.5);
  
  // 轮廓光
  vec3 rimColor = vec3(0.3, 0.4, 0.5);
  color += rimColor * edge * 0.3;
  
  // --- 2. 增强对比度 ---
  
  color = pow(color, vec3(0.95)); // 轻微提亮
  
  // --- 3. 清晰度增强 ---
  
  float luminance = dot(color, vec3(0.299, 0.587, 0.114));
  vec3 sharpened = color + (color - vec3(luminance)) * 0.15;
  color = mix(color, sharpened, 0.5);
  
  // --- 4. 简单色调映射 ---
  
  // Reinhard
  color = color / (color + vec3(1.0));
  
  // Gamma校正
  color = pow(color, vec3(1.0/2.2));
  
  // --- 5. 输出 ---
  
  fragOutput0.rgb = color;
END_REPLACEMENT