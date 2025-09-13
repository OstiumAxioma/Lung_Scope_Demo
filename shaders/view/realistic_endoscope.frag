# Realistic Endoscope View Shader
# 真实内窥镜视图 - 模拟医疗成像特性和真实光照

//VTK::System::Dec AFTER
// 视图特定参数
uniform float exposure = 1.2;        // 曝光度
uniform float contrast = 1.1;        // 对比度
uniform float saturation = 0.9;      // 饱和度（略微降低模拟医疗设备）
uniform float vignette = 0.3;        // 暗角强度
uniform float filmGrain = 0.02;      // 胶片颗粒
END_REPLACEMENT

//VTK::Color::Impl AFTER
//VTK::Color::Impl
  // 内窥镜真实色彩 - 偏暖的粉红色调
  diffuseColor = mix(diffuseColor, vec3(0.95, 0.85, 0.82), 0.3);
  ambientColor = vec3(0.3, 0.25, 0.22);
  specularColor = vec3(1.0, 0.95, 0.9);
END_REPLACEMENT

//VTK::Light::Impl AFTER
//VTK::Light::Impl

  // ============ 真实内窥镜成像效果 ============
  
  vec3 color = fragOutput0.rgb;
  vec2 uv = gl_FragCoord.xy / vec2(1920.0, 1080.0); // 假设分辨率
  
  // --- 1. 景深效果模拟 ---
  
  // 计算深度
  float depth = length(vertexVC.xyz);
  float focusDistance = 50.0; // 焦点距离
  float dofStrength = abs(depth - focusDistance) / 100.0;
  dofStrength = clamp(dofStrength, 0.0, 1.0);
  
  // 简单的模糊近似（实际应该用后处理）
  color = mix(color, color * 0.8 + vec3(0.2), dofStrength * 0.3);
  
  // --- 2. 色差（Chromatic Aberration） ---
  
  vec2 centerVec = uv - vec2(0.5);
  float aberration = length(centerVec) * 0.01;
  
  // 这里简化处理，实际需要多次采样
  vec3 aberratedColor = color;
  aberratedColor.r *= 1.0 + aberration;
  aberratedColor.b *= 1.0 - aberration;
  color = mix(color, aberratedColor, 0.5);
  
  // --- 3. 暗角效果（Vignetting） ---
  
  float dist = distance(uv, vec2(0.5));
  float vignetteFactor = smoothstep(0.8, 0.3, dist);
  vignetteFactor = mix(1.0, vignetteFactor, vignette);
  color *= vignetteFactor;
  
  // --- 4. 镜头光晕（简化） ---
  
  vec3 L = normalize(vec3(0.0, 0.0, 1.0)); // 简化的光源方向
  vec3 V = normalize(-vertexVC.xyz);
  float flare = max(dot(V, L), 0.0);
  flare = pow(flare, 8.0) * 0.1;
  color += vec3(1.0, 0.9, 0.7) * flare;
  
  // --- 5. 局部对比度增强（类似Unsharp Mask） ---
  
  float localContrast = (color.r + color.g + color.b) / 3.0;
  vec3 enhanced = color + (color - vec3(localContrast)) * 0.2;
  color = mix(color, enhanced, contrast - 1.0);
  
  // --- 6. 色彩分级（Color Grading） ---
  
  // Lift Gamma Gain
  vec3 lift = vec3(0.02, 0.01, 0.01);
  vec3 gamma = vec3(1.0, 0.98, 0.96);
  vec3 gain = vec3(1.1, 1.05, 1.0);
  
  color = pow(color, gamma);
  color = color * gain + lift;
  
  // 饱和度调整
  float gray = dot(color, vec3(0.299, 0.587, 0.114));
  color = mix(vec3(gray), color, saturation);
  
  // --- 7. 胶片颗粒 ---
  
  float noise = fract(sin(dot(uv + fract(1.0), vec2(12.9898, 78.233))) * 43758.5453);
  color += (noise - 0.5) * filmGrain;
  
  // --- 8. 曝光调整 ---
  
  color *= exposure;
  
  // --- 9. 高级色调映射（Filmic） ---
  
  // Filmic Tonemapping (John Hable's Uncharted 2)
  vec3 x = max(vec3(0.0), color - 0.004);
  vec3 result = (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06);
  color = result;
  
  // --- 10. 最终Gamma校正 ---
  
  color = pow(color, vec3(1.0/2.2));
  
  // 确保在有效范围内
  fragOutput0.rgb = clamp(color, 0.0, 1.0);
END_REPLACEMENT