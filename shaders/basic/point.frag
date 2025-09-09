void main() {
    // 创建圆形点
    vec2 coord = gl_PointCoord - vec2(0.5);
    if (length(coord) > 0.5) {
        discard;
    }
    
    // 红色标记球
    float alpha = 1.0 - smoothstep(0.4, 0.5, length(coord));
    gl_FragColor = vec4(1.0, 0.0, 0.0, alpha);
}