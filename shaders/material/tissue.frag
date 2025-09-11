# Tissue Material Shader
# 组织材质 - 适用于支气管模型的渲染效果

//VTK::Light::Impl AFTER
//VTK::Light::Impl
  
  // === 组织材质渲染 ===
  
  // 1. 直接使用插值的顶点法线（完全平滑）
  vec3 N = normalize(normalVCVSOutput);
  
  // 2. 基础光照向量
  vec3 V = normalize(-vertexVC.xyz);
  vec3 L = normalize(vec3(0.0, 0.0, 1.0));
  vec3 H = normalize(L + V);
  
  // 3. 改进的漫反射计算（使用更柔和的光照）
  float NdotL = dot(N, L);
  // 半兰伯特：将[-1,1]映射到[0,1]，避免过暗的背光面
  float halfLambert = NdotL * 0.5 + 0.5;
  // 使用非常平滑的幂次，最大程度减少对比度
  halfLambert = pow(halfLambert, 0.6);  // 进一步降低幂次
  
  // 4. 调整光照平衡
  ambientColor = ambientColor * 0.6;  // 适度的环境光
  diffuse = diffuseColor * halfLambert * lightColor0 * 0.8;  // 降低直接光照强度
  
  // 5. 改进的镜面反射（Blinn-Phong）
  float NdotH = max(dot(N, H), 0.0);
  float spec = pow(NdotH, 64.0);  // 提高光泽度
  specular = specularColor * spec * lightColor0 * 0.8;
  
  // 6. 菲涅尔效果（边缘轻微提亮）
  float NdotV = max(dot(N, V), 0.0);
  float fresnel = pow(1.0 - NdotV, 1.5);
  fresnel = fresnel * 0.15;  // 轻微的菲涅尔
  
  // 7. 更柔和的阴影效果
  float shadow = smoothstep(-0.5, 0.8, NdotL);  // 扩大过渡范围
  shadow = 0.5 + 0.5 * shadow;  // 提高最暗处亮度到50%
  
  // 8. 组合最终颜色
  vec3 finalColor = ambientColor + (diffuse + specular) * shadow;
  finalColor += vec3(0.05, 0.03, 0.03) * fresnel;  // 轻微的边缘光
  
  fragOutput0.rgb = finalColor;
END_REPLACEMENT