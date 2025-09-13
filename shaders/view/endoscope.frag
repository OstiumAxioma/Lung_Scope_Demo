# Realistic Endoscope View Shader
# Endoscope imaging effects

//VTK::System::Dec AFTER
// Imaging parameters
const float exposure = 1.3;
const float contrast = 1.15;
const float vignette = 0.25;
END_REPLACEMENT

//VTK::Color::Impl AFTER
//VTK::Color::Impl
  // Endoscope color correction
  diffuseColor = diffuseColor * vec3(1.05, 1.0, 0.95);
  ambientColor = ambientColor * vec3(1.1, 1.05, 1.0);
END_REPLACEMENT

//VTK::Light::Impl AFTER
//VTK::Light::Impl
  
  // ============ Endoscope Post Processing ============
  
  vec3 color = fragOutput0.rgb;
  
  // Screen coordinates
  vec2 uv = gl_FragCoord.xy / vec2(1920.0, 1080.0);
  vec2 center = uv - vec2(0.5);
  
  // 1. Radial distortion
  float dist2 = dot(center, center);
  float distortion = 1.0 + dist2 * 0.1;
  
  // 2. Chromatic aberration
  float aberration = dist2 * 0.008;
  vec3 aberratedColor = color;
  aberratedColor.r *= 1.0 + aberration;
  aberratedColor.b *= 1.0 - aberration;
  color = mix(color, aberratedColor, 0.6);
  
  // 3. Vignetting
  float vignetteFactor = 1.0 - smoothstep(0.0, 0.7, dist2 * 0.8);
  vignetteFactor = mix(1.0, vignetteFactor, vignette);
  color *= vignetteFactor;
  
  // 4. Edge blur
  float edgeBlur = smoothstep(0.3, 0.7, dist2);
  color = mix(color, color * 0.7, edgeBlur * 0.3);
  
  // 5. Contrast
  vec3 avgColor = vec3(0.5);
  color = mix(avgColor, color, contrast);
  
  // 6. Color grading
  color.r *= 1.05;
  color.g *= 1.0;
  color.b *= 0.95;
  
  vec3 lift = vec3(0.01, 0.005, 0.0);
  vec3 gamma = vec3(0.98, 1.0, 1.02);
  vec3 gain = vec3(1.05, 1.0, 0.98);
  
  color = pow(color, gamma) * gain + lift;
  
  // 7. Bloom
  float brightness = dot(color, vec3(0.299, 0.587, 0.114));
  if (brightness > 0.8) {
    vec3 bloom = (color - 0.8) * 0.5;
    color += bloom * vec3(1.0, 0.95, 0.9);
  }
  
  // 8. Exposure
  color *= exposure;
  
  // 9. Tone mapping (ACES)
  float a = 2.51;
  float b = 0.03;
  float c = 2.43;
  float d = 0.59;
  float e = 0.14;
  color = clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
  
  // 10. Output
  fragOutput0.rgb = color;
END_REPLACEMENT