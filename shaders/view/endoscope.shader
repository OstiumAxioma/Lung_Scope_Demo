# Endoscope视图Shader配置 - 平滑网格显示
# 格式: [SHADER_TYPE] [REPLACEMENT_TAG] [BEFORE/AFTER]

[Vertex] //VTK::Normal::Dec BEFORE
//VTK::Normal::Dec
  out vec3 barycentricCoords;
END_REPLACEMENT

[Vertex] //VTK::Normal::Impl BEFORE
//VTK::Normal::Impl
  // 简化的重心坐标分配
  int vid = gl_VertexID % 3;
  if (vid == 0) barycentricCoords = vec3(1.0, 0.0, 0.0);
  else if (vid == 1) barycentricCoords = vec3(0.0, 1.0, 0.0);
  else barycentricCoords = vec3(0.0, 0.0, 1.0);
END_REPLACEMENT

[Fragment] //VTK::Normal::Dec BEFORE
//VTK::Normal::Dec
  in vec3 barycentricCoords;
END_REPLACEMENT

[Fragment] //VTK::Color::Impl AFTER
//VTK::Color::Impl
  // 内窥镜视图颜色调整
  diffuseColor = vec3(0.95, 0.75, 0.7);
  ambientColor = vec3(0.35, 0.25, 0.25);
END_REPLACEMENT

[Fragment] //VTK::Light::Impl AFTER
//VTK::Light::Impl
  // 增强镜面反射模拟湿润表面
  specular *= 1.5;
  
  // 平滑网格线效果
  vec3 d = fwidth(barycentricCoords);
  vec3 a3 = smoothstep(vec3(0.0), d*1.5, barycentricCoords);
  float edgeFactor = min(min(a3.x, a3.y), a3.z);
  
  // 网格线颜色（深红色）
  vec3 wireColor = vec3(0.4, 0.15, 0.15);
  
  // 混合网格线和表面颜色
  vec3 surfaceColor = fragOutput0.rgb;
  fragOutput0.rgb = mix(wireColor, surfaceColor, edgeFactor);
END_REPLACEMENT