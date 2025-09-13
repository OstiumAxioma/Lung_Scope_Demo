# PBR Tissue Material Shader
# Physically Based Rendering for tissue

//VTK::System::Dec AFTER
// PBR material parameters
const float metallic = 0.0;
const float roughness = 0.45;  // Increased for stability
const float subsurface = 0.5;
const float clearcoat = 0.1;
END_REPLACEMENT

//VTK::Light::Impl AFTER
//VTK::Light::Impl
  
  // ============ PBR Enhancement ============
  
  // Base vectors with safety checks
  vec3 N = normalize(normalVCVSOutput);
  vec3 V = normalize(-vertexVC.xyz);
  
  // Ensure vectors are valid
  if (length(N) < 0.5) N = vec3(0.0, 0.0, 1.0);
  if (length(V) < 0.5) V = vec3(0.0, 0.0, 1.0);
  
  // Get base color from VTK with safety
  vec3 baseColor = max(diffuseColor, vec3(0.01));
  
  // PBR F0
  vec3 F0 = vec3(0.04);
  F0 = mix(F0, baseColor, metallic);
  
  // Light setup (HeadLight)
  vec3 L = V;
  vec3 H = normalize(V + L);
  vec3 lightColorPBR = vec3(1.0, 0.98, 0.95);
  
  // Clamp all dot products to avoid negative values
  float NdotL = clamp(dot(N, L), 0.001, 1.0);
  float NdotV = clamp(dot(N, V), 0.001, 1.0);
  float NdotH = clamp(dot(N, H), 0.001, 1.0);
  float VdotH = clamp(dot(V, H), 0.001, 1.0);
  
  // D term - GGX with numerical stability
  float a = max(roughness * roughness, 0.001);
  float a2 = a * a;
  float NdotH2 = NdotH * NdotH;
  float num = a2;
  float denom = NdotH2 * (a2 - 1.0) + 1.0;
  denom = max(3.14159265 * denom * denom, 0.001);
  float D = num / denom;
  D = clamp(D, 0.0, 100.0);  // Prevent extreme values
  
  // G term - Smith with safety
  float r = roughness + 1.0;
  float k = (r * r) / 8.0;
  float GL = NdotL / max(NdotL * (1.0 - k) + k, 0.001);
  float GV = NdotV / max(NdotV * (1.0 - k) + k, 0.001);
  float G = clamp(GL * GV, 0.0, 1.0);
  
  // F term - Schlick with clamping
  float fresnel = clamp(1.0 - VdotH, 0.0, 1.0);
  vec3 F = F0 + (vec3(1.0) - F0) * pow(fresnel, 5.0);
  F = clamp(F, vec3(0.0), vec3(1.0));
  
  // BRDF with numerical safety
  vec3 numerator = D * G * F;
  float denominator = max(4.0 * NdotV * NdotL, 0.001);
  vec3 specularPBR = numerator / denominator;
  specularPBR = clamp(specularPBR, vec3(0.0), vec3(10.0));
  
  // Diffuse
  vec3 kS = F;
  vec3 kD = vec3(1.0) - kS;
  kD *= 1.0 - metallic;
  kD = clamp(kD, vec3(0.0), vec3(1.0));
  
  vec3 diffusePBR = baseColor / 3.14159265;
  
  // Simplified subsurface (more stable)
  vec3 subsurfaceColor = vec3(0.9, 0.5, 0.4);
  float subsurfaceAmount = subsurface * 0.3 * (1.0 - NdotV);
  vec3 subsurfaceContrib = subsurfaceColor * subsurfaceAmount;
  
  // Simplified clearcoat
  float clearcoatSpec = 0.04 * pow(NdotH, 32.0) * clearcoat;
  
  // Environment lighting (simplified)
  vec3 ambientPBR = baseColor * 0.2;
  
  // Combine all contributions with safety
  vec3 directLight = (kD * diffusePBR + specularPBR) * lightColorPBR * NdotL;
  directLight = clamp(directLight, vec3(0.0), vec3(2.0));
  
  vec3 indirectLight = ambientPBR + subsurfaceContrib + vec3(clearcoatSpec);
  indirectLight = clamp(indirectLight, vec3(0.0), vec3(1.0));
  
  vec3 colorPBR = directLight + indirectLight;
  
  // Safe tone mapping
  colorPBR = clamp(colorPBR, vec3(0.0), vec3(10.0));
  colorPBR = colorPBR / (colorPBR + vec3(1.0));
  
  // Safe gamma correction
  colorPBR = pow(max(colorPBR, vec3(0.0)), vec3(1.0/2.2));
  
  // Blend with original for stability
  vec3 originalColor = clamp(fragOutput0.rgb, vec3(0.0), vec3(1.0));
  fragOutput0.rgb = mix(originalColor, colorPBR, 0.7);
  fragOutput0.rgb = clamp(fragOutput0.rgb, vec3(0.0), vec3(1.0));
END_REPLACEMENT