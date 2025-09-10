# Endoscope View Shader - Smooth Surface Display
# Enhanced lighting model for realistic endoscope effect

[Fragment] //VTK::Color::Impl AFTER
//VTK::Color::Impl
  // Endoscope view color adjustment - tissue color simulation
  diffuseColor = vec3(0.95, 0.75, 0.7);
  ambientColor = vec3(0.35, 0.25, 0.25);
END_REPLACEMENT

[Fragment] //VTK::Light::Impl AFTER
//VTK::Light::Impl
  // Enhanced Phong lighting for smooth surface
  
  // 1. Enhanced specular reflection - simulate wet tissue surface
  vec3 viewDir = normalize(-vertexVC.xyz);
  vec3 lightDir = normalize(vec3(0.0, 0.0, 1.0));
  vec3 halfwayDir = normalize(lightDir + viewDir);
  float spec = pow(max(dot(normalVCVSOutput, halfwayDir), 0.0), 64.0);
  specular = vec3(0.3, 0.25, 0.25) * spec;
  
  // 2. Subsurface scattering effect
  float thickness = 1.0 - abs(dot(normalVCVSOutput, viewDir));
  vec3 subsurfaceColor = vec3(0.2, 0.05, 0.05) * thickness * thickness;
  
  // 3. Fresnel effect - enhanced reflection at edges
  float fresnel = pow(1.0 - max(dot(normalVCVSOutput, viewDir), 0.0), 2.0);
  vec3 fresnelColor = vec3(0.15, 0.1, 0.1) * fresnel;
  
  // 4. Combine all lighting effects
  fragOutput0.rgb += subsurfaceColor + fresnelColor;
  
  // 5. Tone mapping for realism
  fragOutput0.rgb = fragOutput0.rgb / (fragOutput0.rgb + vec3(1.0));
  fragOutput0.rgb = pow(fragOutput0.rgb, vec3(1.0/2.2));
END_REPLACEMENT