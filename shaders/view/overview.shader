# Overview视图Shader配置
# 格式: [SHADER_TYPE] [REPLACEMENT_TAG] [BEFORE/AFTER] 
# 然后是shader代码，以END_REPLACEMENT结束

[Fragment] //VTK::Color::Impl AFTER
//VTK::Color::Impl
  // 微调颜色
  diffuseColor = vec3(0.85, 0.85, 0.8);
  ambientColor = vec3(0.3, 0.3, 0.3);
END_REPLACEMENT

[Fragment] //VTK::Light::Impl AFTER
//VTK::Light::Impl
  // 增强边缘光照
  vec3 viewDir = normalize(-vertexVC.xyz);
  float rim = 1.0 - max(dot(viewDir, normalVCVSOutput), 0.0);
  rim = smoothstep(0.6, 1.0, rim);
  vec3 rimLight = vec3(0.2, 0.2, 0.25) * rim;
  fragOutput0.rgb += rimLight;
END_REPLACEMENT