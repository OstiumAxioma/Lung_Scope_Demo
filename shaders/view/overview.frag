# Overview Fragment Shader Replacements
# Enhanced edge lighting for overview

//VTK::Color::Impl] AFTER
//VTK::Color::Impl
  // Slightly gray-white color for overview
  diffuseColor = vec3(0.85, 0.85, 0.8);
  ambientColor = vec3(0.3, 0.3, 0.3);
END_REPLACEMENT

//VTK::Light::Impl] AFTER
//VTK::Light::Impl
  // Enhanced edge lighting
  vec3 viewDir = normalize(-vertexVC.xyz);
  float rim = 1.0 - max(dot(viewDir, normalVCVSOutput), 0.0);
  rim = smoothstep(0.6, 1.0, rim);
  vec3 rimLight = vec3(0.2, 0.2, 0.25) * rim;
  fragOutput0.rgb += rimLight;
END_REPLACEMENT