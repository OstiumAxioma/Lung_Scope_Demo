# Tissue Material Shader
# 组织材质 - 适用于支气管模型的渲染效果

//VTK::Light::Impl AFTER
//VTK::Light::Impl
  
  // === 组织材质渲染 ===
  
  // 1. 法线平滑处理
  vec3 N = normalize(normalVCVSOutput);
  vec3 dx = dFdx(normalVCVSOutput);
  vec3 dy = dFdy(normalVCVSOutput);
  N = normalize(N - 0.25 * (dx + dy));  // 平滑法线
  
  // 2. 基础光照向量
  vec3 V = normalize(-vertexVC.xyz);
  vec3 L = normalize(vec3(0.0, 0.0, 1.0));
  vec3 H = normalize(L + V);
  
  // 3. 曲率自适应
  float curvature = length(dFdx(N) + dFdy(N));
  curvature = clamp(curvature * 2.0, 0.0, 1.0);
  
  // 4. 平滑漫反射
  float NdotL = dot(N, L);
  float diffuseFactor = smoothstep(-0.1, 0.1, NdotL);
  diffuseFactor = mix(diffuseFactor, 0.5, curvature * 0.3);
  
  // 5. 重新计算漫反射和镜面反射
  diffuse = diffuseFactor * diffuseColor * lightColor0;
  
  float NdotH = max(dot(N, H), 0.0);
  float spec = pow(NdotH, 32.0) * (1.0 - curvature * 0.3);
  specular = specularColor * spec * lightColor0;
  
  // 6. 次表面散射（组织特性）
  float NdotV = max(dot(N, V), 0.0);
  float thickness = 1.0 - NdotV;
  vec3 subsurface = vec3(0.15, 0.05, 0.05) * pow(thickness, 2.0);
  
  // 7. 微遮蔽
  float occlusion = 1.0 - curvature * 0.15;
  
  // 8. 更新最终输出
  fragOutput0.rgb = (ambientColor + diffuse + specular) * occlusion + subsurface;
  
  // 测试：添加明显的红色调来验证shader是否生效
  fragOutput0.rgb = mix(fragOutput0.rgb, vec3(1.0, 0.2, 0.2), 0.3);
END_REPLACEMENT