# Endoscope View Shader
# 内窥镜视图特定效果（颜色调整、边缘增强等）

//VTK::Color::Impl AFTER
//VTK::Color::Impl
  // 内窥镜视图颜色调整 - 微红色调模拟真实内窥镜
  diffuseColor = vec3(0.95, 0.75, 0.7);
  ambientColor = vec3(0.35, 0.25, 0.25);
END_REPLACEMENT

//VTK::Light::Impl AFTER
//VTK::Light::Impl
  
  // === 内窥镜视图特定效果 ===
  
  // 1. 增强湿润表面的镜面反射
  specular *= 1.5;  // 增强镜面反射模拟湿润表面
  
  // 2. 环境光遮蔽（AO）效果
  vec3 V = normalize(-vertexVC.xyz);
  vec3 N = normalize(normalVCVSOutput);
  
  // 基于视角和法线的简单AO近似
  float NdotV = max(dot(N, V), 0.0);
  float ao = pow(NdotV, 0.5);  // 较平滑的AO过渡
  ao = 0.7 + 0.3 * ao;  // 限制AO的影响范围
  
  // 应用AO到漫反射
  diffuse *= ao;
  
  // 3. 色调映射（模拟内窥镜成像特性）
  fragOutput0.rgb = fragOutput0.rgb / (fragOutput0.rgb + vec3(1.2));
  fragOutput0.rgb = pow(fragOutput0.rgb, vec3(1.0/2.0));  // 稍微提亮
END_REPLACEMENT