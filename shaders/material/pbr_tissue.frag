# PBR Tissue Material Shader
# 基于物理的渲染 - 模拟真实的组织材质

//VTK::System::Dec AFTER
// PBR参数声明
uniform float metallic = 0.0;      // 组织非金属
uniform float roughness = 0.4;     // 中等粗糙度
uniform float subsurface = 0.8;    // 高次表面散射
uniform float specularTint = 0.0;  // 无颜色偏移
uniform float sheen = 0.3;          // 轻微织物光泽
uniform float clearcoat = 0.1;     // 湿润表面涂层
END_REPLACEMENT

//VTK::Light::Impl AFTER
//VTK::Light::Impl

  // ============ PBR光照实现 ============
  
  // 获取基础向量
  vec3 N = normalize(normalVCVSOutput);
  vec3 V = normalize(-vertexVC.xyz);
  
  // --- 1. 基础PBR计算 ---
  
  // Fresnel-Schlick近似
  vec3 F0 = vec3(0.04); // 非金属的F0
  F0 = mix(F0, diffuseColor, metallic);
  
  // 多光源循环（这里简化为主光源）
  vec3 Lo = vec3(0.0);
  
  // 主光源方向（HeadLight）
  vec3 L = V; // HeadLight在相机位置
  vec3 H = normalize(V + L);
  
  float NdotL = max(dot(N, L), 0.0);
  float NdotV = max(dot(N, V), 0.0);
  float NdotH = max(dot(N, H), 0.0);
  float VdotH = max(dot(V, H), 0.0);
  
  // --- 2. BRDF组件 ---
  
  // 2.1 分布函数 D - GGX/Trowbridge-Reitz
  float alpha = roughness * roughness;
  float alpha2 = alpha * alpha;
  float NdotH2 = NdotH * NdotH;
  float denom = NdotH2 * (alpha2 - 1.0) + 1.0;
  float D = alpha2 / (3.14159265 * denom * denom);
  
  // 2.2 几何函数 G - Smith's Schlick-GGX
  float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
  float G1L = NdotL / (NdotL * (1.0 - k) + k);
  float G1V = NdotV / (NdotV * (1.0 - k) + k);
  float G = G1L * G1V;
  
  // 2.3 Fresnel项 F
  vec3 F = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);
  
  // Cook-Torrance BRDF
  vec3 numerator = D * G * F;
  float denominator = 4.0 * NdotV * NdotL + 0.001;
  vec3 specular = numerator / denominator;
  
  // --- 3. 次表面散射（组织特有） ---
  
  // 简化的次表面散射模型
  float thickness = 1.0;
  vec3 subsurfaceColor = vec3(0.8, 0.4, 0.3); // 肉色调
  
  // 透射光近似
  vec3 vLTLight = L + N * 0.3;
  float fLTDot = pow(max(dot(V, -vLTLight), 0.0), 3.0) * thickness;
  vec3 subsurfaceContrib = subsurfaceColor * fLTDot * subsurface;
  
  // 前向散射
  float forwardScatter = exp2(dot(V, -L) * 1.0 - 1.0);
  subsurfaceContrib += subsurfaceColor * forwardScatter * subsurface * 0.5;
  
  // --- 4. 漫反射（Lambert修正） ---
  
  vec3 kS = F;
  vec3 kD = vec3(1.0) - kS;
  kD *= 1.0 - metallic;
  
  // Disney漫反射模型
  float FD90 = 0.5 + 2.0 * roughness * VdotH * VdotH;
  float FdV = 1.0 + (FD90 - 1.0) * pow(1.0 - NdotV, 5.0);
  float FdL = 1.0 + (FD90 - 1.0) * pow(1.0 - NdotL, 5.0);
  vec3 diffuse = diffuseColor * (1.0 / 3.14159265) * FdV * FdL;
  
  // --- 5. Sheen（织物/绒毛光泽） ---
  
  float FH = pow(1.0 - VdotH, 5.0);
  vec3 sheenColor = mix(vec3(1.0), diffuseColor, specularTint);
  vec3 sheenContrib = sheenColor * sheen * FH;
  
  // --- 6. Clearcoat（湿润涂层） ---
  
  // 固定IOR=1.5的涂层
  float clearcoatRoughness = 0.04;
  float Dr = D * 0.25; // 简化的clearcoat分布
  float Gr = G * 0.25; // 简化的clearcoat几何
  float Fr = 0.04 + 0.96 * pow(1.0 - VdotH, 5.0);
  float clearcoatSpec = Dr * Gr * Fr / (4.0 * NdotV * NdotL + 0.001);
  vec3 clearcoatContrib = vec3(clearcoatSpec * clearcoat);
  
  // --- 7. 环境光照近似 ---
  
  // 简单的IBL近似
  vec3 irradiance = ambientColor * 2.0; // 提升环境光
  vec3 ambient = irradiance * diffuseColor * (1.0 - metallic);
  
  // 环境反射近似
  float mipLevel = roughness * 4.0;
  vec3 prefilteredColor = mix(diffuseColor, vec3(1.0), 1.0 - roughness) * 0.5;
  vec2 envBRDF = vec2(1.0 - roughness, NdotV); // 简化的BRDF查找
  vec3 ambientSpecular = prefilteredColor * (F * envBRDF.x + envBRDF.y * 0.1);
  
  // --- 8. 全局光照近似 ---
  
  // 简单的色彩渗透
  vec3 gi = diffuseColor * 0.1 * (1.0 - metallic);
  
  // --- 9. 组合所有光照贡献 ---
  
  Lo = (kD * diffuse + specular + sheenContrib + clearcoatContrib) * NdotL;
  Lo += subsurfaceContrib;
  Lo += ambient + ambientSpecular + gi;
  
  // --- 10. 色调映射和颜色校正 ---
  
  // ACES色调映射
  vec3 a = Lo * 2.51;
  vec3 b = Lo * 0.03;
  vec3 c = Lo * 2.43;
  vec3 d = Lo * 0.59;
  vec3 e = Lo * 0.14;
  Lo = clamp((a * b) / (c * d + e), 0.0, 1.0);
  
  // Gamma校正
  Lo = pow(Lo, vec3(1.0/2.2));
  
  // 输出
  fragOutput0.rgb = Lo;
  fragOutput0.a = 1.0;
END_REPLACEMENT