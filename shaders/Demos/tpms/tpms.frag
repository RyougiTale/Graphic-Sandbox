#version 460 core

in vec2 v_UV;
out vec4 FragColor;

// ============================================================
// Uniforms
// ============================================================

// 相机
uniform mat4 u_InvView;        // view 矩阵的逆 — 将相机空间方向还原到世界空间
uniform mat4 u_InvProjection;  // projection 矩阵的逆 — 将 NDC 坐标还原到相机空间
uniform vec3 u_CameraPos;

// TPMS 参数
uniform int   u_TPMSType;   // 0=Gyroid, 1=SchwarzP, 2=SchwarzD, 3=Neovius, 4=Lidinoid
uniform int   u_SolidMode;  // 0=Sheet(|F|<t), 1=Network(F<t)
uniform float u_K;           // 频率 k = 2π / cellSize
uniform float u_Thickness;   // 壁厚 (sheet 模式)
uniform float u_IsoValue;    // 等值面偏移 (network 模式)
uniform float u_Time;
uniform int   u_Animate;

// 渲染
uniform int   u_MaxSteps;
uniform float u_MaxDist;
uniform float u_BoundingBox; // 渲染范围半边长

// 外观
uniform vec3 u_Color1;       // 正面颜色
uniform vec3 u_Color2;       // 背面颜色
uniform vec3 u_BgColor;      // 背景色

// 5 种 TPMS 速查表:
// Type         Formula complexity   Symmetry              Visual character
// Gyroid       3 terms              Chiral (no mirrors)   Spiraling channels
// Schwarz P    3 terms              Full cubic (Oh)       Swiss cheese / 6-way pipes
// Schwarz D    4 terms              Full cubic (Oh)       Diamond-like twisted tunnels
// Neovius      4 terms              Full cubic (Oh)       Schwarz P with extra branches
// Lidinoid     6 terms, double freq Chiral (like Gyroid)  Tangled triangular channels

// ============================================================
// TPMS 场函数
// ============================================================
// 每个 TPMS 类型定义一个隐式函数 F(p), 其零等值面 F=0 就是极小曲面

float tpmsField(vec3 p)
{
    float x = p.x, y = p.y, z = p.z;

    // 动画: 缓慢变形, 混合两种 TPMS 产生 morphing 效果
    float anim = (u_Animate != 0) ? sin(u_Time * 0.5) * 0.3 : 0.0;

    // ----------------------------------------------------------------
    // Gyroid (螺旋面)    — Alan Schoen, 1970
    // ----------------------------------------------------------------
    // 发现者: NASA科学家 Alan Schoen 在研究高比表面积结构时发现
    // 几何特征: 没有直线, 没有平面对称面, 整个曲面由连续的螺旋通道构成
    //   - 将空间分成两个互相贯穿、永不相交的迷宫通道
    //   - 每个通道都是连通的 (可以从任意一点走到任意另一点)
    //   - 曲面上每一点的平均曲率为 0 (极小曲面的定义)
    // 数学: F = sin(x)cos(y) + sin(y)cos(z) + sin(z)cos(x)
    //   - 三项结构完全对称: x→y→z→x 循环置换不变
    //   - 这种对称性来自 Gyroid 属于空间群 I4₁32 (手性立方对称)
    // 应用:
    //   - 阿迪达斯 4D 鞋底 (缓震 + 透气)
    //   - 蝴蝶翅膀的微观结构就是天然 Gyroid (产生结构色)
    //   - 换热器: 两侧流体各走一个通道, 极大化接触面积
    // ----------------------------------------------------------------
    if (u_TPMSType == 0)
    {
        return sin(x) * cos(y) + sin(y) * cos(z) + sin(z) * cos(x) + anim;
    }

    // ----------------------------------------------------------------
    // Schwarz P (Primitive, 原始面)    — Hermann Schwarz, 1865
    // ----------------------------------------------------------------
    // 发现者: 德国数学家 Schwarz, 最早被严格证明的 TPMS 之一
    // 几何特征: 最"直觉"的 TPMS — 看起来像一个被掏了 6 个圆孔的立方体
    //   - 6 个管道分别沿 ±x, ±y, ±z 贯穿, 在中心交汇
    //   - 具有完整的立方对称性 (Oh 群, 48 个对称操作)
    //   - Sheet 模式下像一块瑞士奶酪; Network 模式下像管道交叉口
    // 数学: F = cos(x) + cos(y) + cos(z)
    //   - 三个独立余弦的简单叠加 — 最简单的 TPMS 公式
    //   - F=0 曲面把空间分成 "里" (F<0, 管道) 和 "外" (F>0, 实体)
    //   - 调节 isoValue 可以控制 "管道" 粗细 (孔隙率)
    // 应用:
    //   - 骨科植入物: 孔径可控, 促进骨骼长入
    //   - 声学: 吸声结构, 管道形成亥姆霍兹共振腔
    // ----------------------------------------------------------------
    else if (u_TPMSType == 1)
    {
        return cos(x) + cos(y) + cos(z) + anim;
    }

    // ----------------------------------------------------------------
    // Schwarz D (Diamond, 金刚石面)    — Hermann Schwarz, 1865
    // ----------------------------------------------------------------
    // 几何特征: 曲面的拓扑结构与金刚石晶体的键合网络同构
    //   - 两个互穿的通道各自形成金刚石 (FCC) 晶格
    //   - 比 Schwarz P 更"弯曲", 通道像扭曲的四面体连接
    //   - 曲面在四面体顶点方向形成"鞍点"
    // 数学: F = sin(x)sin(y)sin(z) + sin(x)cos(y)cos(z)
    //         + cos(x)sin(y)cos(z) + cos(x)cos(y)sin(z)
    //   - 等价形式: F = cos(x)cos(y)cos(z) - sin(x)sin(y)sin(z)
    //     (通过三角恒等式可化简为这个更紧凑的形式)
    //   - 但展开形式更清楚地展示了 sin/cos 的排列组合
    // 应用:
    //   - 力学性能优于 Schwarz P (各向同性更好)
    //   - 电池电极: 两个互穿通道分别填充正极材料和电解质
    // ----------------------------------------------------------------
    else if (u_TPMSType == 2)
    {
        return sin(x)*sin(y)*sin(z)
             + sin(x)*cos(y)*cos(z)
             + cos(x)*sin(y)*cos(z)
             + cos(x)*cos(y)*sin(z) + anim;
    }

    // ----------------------------------------------------------------
    // Neovius    — Edvard Rudolf Neovius, 1883
    // ----------------------------------------------------------------
    // 发现者: 芬兰数学家 Neovius, Schwarz 的学生
    // 几何特征: 比 Schwarz P 更复杂的管道结构
    //   - 每个晶胞内有更多的连通管道和更大的表面积
    //   - 外观像 "加强版 Schwarz P" — 管道交汇处有额外的分支
    //   - 比表面积 (surface-to-volume ratio) 比 Schwarz P 大约高 30%
    // 数学: F = 3(cos(x) + cos(y) + cos(z)) + 4·cos(x)cos(y)cos(z)
    //   - 前半部分就是 3 倍的 Schwarz P
    //   - 后半部分 cos(x)cos(y)cos(z) 引入了三方向耦合
    //   - 系数 3:4 的比例决定了曲面的具体形状
    //   - 场值范围比其他 TPMS 大 (约 [-10, 10]), 调参时注意
    // 应用:
    //   - 需要极高比表面积的场景: 催化剂载体, 过滤器
    //   - 研究中: 力学性能介于 Schwarz P 和 Gyroid 之间
    // ----------------------------------------------------------------
    else if (u_TPMSType == 3)
    {
        return 3.0 * (cos(x) + cos(y) + cos(z))
             + 4.0 * cos(x) * cos(y) * cos(z) + anim;
    }

    // ----------------------------------------------------------------
    // Lidinoid    — Sven Lidin, 1990
    // ----------------------------------------------------------------
    // 发现者: 瑞典晶体学家 Sven Lidin
    // 几何特征: 与 Gyroid 同属手性 (chiral) TPMS — 没有镜面对称
    //   - 可以看作 Gyroid 的"表亲": 都具有 I4₁32 空间群对称性
    //   - 但通道的扭转方式不同, 曲面更加复杂
    //   - 外观: 比 Gyroid 更"纠缠", 通道截面不是圆形而是三角形
    // 数学: F = 0.5[sin(2x)cos(y)sin(z) + sin(2y)cos(z)sin(x)
    //                                    + sin(2z)cos(x)sin(y)]
    //       - 0.5[cos(2x)cos(2y) + cos(2y)cos(2z) + cos(2z)cos(2x)]
    //       + 0.15
    //   - 包含 sin(2x) 等二倍频项, 所以每个周期内细节是 Gyroid 的两倍
    //   - 常数 0.15 是为了让 F=0 更接近真正的极小曲面
    //   - 因为倍频项的存在, 收敛区间更窄, ray marching 可能需要更多步数
    // 应用:
    //   - 主要出现在学术研究中
    //   - 光子晶体: 手性结构产生圆偏振光带隙
    // ----------------------------------------------------------------
    else
    {
        return 0.5 * (sin(2.0*x)*cos(y)*sin(z) + sin(2.0*y)*cos(z)*sin(x) + sin(2.0*z)*cos(x)*sin(y))
             - 0.5 * (cos(2.0*x)*cos(2.0*y) + cos(2.0*y)*cos(2.0*z) + cos(2.0*z)*cos(2.0*x))
             + 0.15 + anim;
    }
}

// ============================================================
// SDF: 将 TPMS 场函数转换为有符号距离函数(近似)
// ============================================================

float sdfTPMS(vec3 p)
{
    // 缩放到 TPMS 频率空间: 乘以 k
    vec3 sp = p * u_K;
    float f = tpmsField(sp);

    if (u_SolidMode == 0)
    {
        // Sheet 模式: |F| < thickness → 实体
        // SDF ≈ (|F| - thickness) / |∇F|
        // 简化: 除以 k 近似还原到世界空间距离
        return (abs(f) - u_Thickness) / u_K;
    }
    else
    {
        // Network 模式: F < isoValue → 实体
        return (f - u_IsoValue) / u_K;
    }
}

// AABB 包围盒 SDF — 限制渲染范围
float sdfBox(vec3 p, float halfSize)
{
    vec3 d = abs(p) - vec3(halfSize);
    return length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0);
}

// 最终场景 SDF: TPMS 与包围盒的交集
float sceneSDF(vec3 p)
{
    float tpms = sdfTPMS(p);
    float box  = sdfBox(p, u_BoundingBox);
    return max(tpms, box); // 交集: 取两者较大值
}

// ============================================================
// 法线计算 (中心差分)
// ============================================================

vec3 calcNormal(vec3 p)
{
    // 用微小偏移计算梯度 → 法线方向
    vec2 e = vec2(0.001, 0.0);
    return normalize(vec3(
        sceneSDF(p + e.xyy) - sceneSDF(p - e.xyy),
        sceneSDF(p + e.yxy) - sceneSDF(p - e.yxy),
        sceneSDF(p + e.yyx) - sceneSDF(p - e.yyx)
    ));
}

// ============================================================
// Ray-AABB 求交 — 跳过空白区域加速
// ============================================================

bool intersectBox(vec3 ro, vec3 rd, float halfSize, out float tNear, out float tFar)
{
    vec3 invRd = 1.0 / rd;
    vec3 t1 = (-vec3(halfSize) - ro) * invRd;
    vec3 t2 = ( vec3(halfSize) - ro) * invRd;
    vec3 tmin = min(t1, t2);
    vec3 tmax = max(t1, t2);
    tNear = max(max(tmin.x, tmin.y), tmin.z);
    tFar  = min(min(tmax.x, tmax.y), tmax.z);
    return tNear < tFar && tFar > 0.0;
}

// ============================================================
// 主函数: Ray Marching
// ============================================================

void main()
{
    // ---- 1. 从屏幕像素重建世界空间射线 ----
    // v_UV [0,1] → NDC [-1,1]
    vec2 ndc = v_UV * 2.0 - 1.0;

    // NDC → 相机空间方向
    vec4 clipPos = vec4(ndc, -1.0, 1.0);
    vec4 viewPos = u_InvProjection * clipPos;
    viewPos = vec4(viewPos.xy, -1.0, 0.0); // 方向向量, w=0

    // 相机空间 → 世界空间
    vec3 rd = normalize((u_InvView * viewPos).xyz);
    vec3 ro = u_CameraPos;

    // ---- 2. Ray-AABB 预判: 射线是否穿过渲染区域 ----
    float tNear, tFar;
    if (!intersectBox(ro, rd, u_BoundingBox + 0.1, tNear, tFar))
    {
        FragColor = vec4(u_BgColor, 1.0);
        return;
    }

    // 从包围盒入口开始 march (如果相机在盒子内, 从 0 开始)
    float t = max(tNear, 0.0);

    // ---- 3. Sphere tracing (ray marching) ----
    bool hit = false;
    for (int i = 0; i < u_MaxSteps; i++)
    {
        vec3 p = ro + rd * t;
        float d = sceneSDF(p);

        if (d < 0.001)
        {
            hit = true;
            break;
        }

        // 步进: SDF 值就是到最近表面的安全距离
        t += d;

        if (t > tFar || t > u_MaxDist)
            break;
    }

    if (!hit)
    {
        FragColor = vec4(u_BgColor, 1.0);
        return;
    }

    // ---- 4. 着色 ----
    vec3 p = ro + rd * t;
    vec3 N = calcNormal(p);

    // 用 TPMS 场值的符号区分颜色 (比 dot(N,rd) 更稳定)
    // 场值 > 0 → color1, 场值 < 0 → color2
    float fieldVal = tpmsField(p * u_K);
    vec3 baseColor = (fieldVal > 0.0) ? u_Color1 : u_Color2;

    // 始终将法线朝向观察者 — 避免薄壁法线采样穿透产生伪影
    vec3 sN = faceforward(N, rd, N);

    // Blinn-Phong 光照
    vec3 L = normalize(vec3(0.5, 1.0, 0.8));
    vec3 V = -rd;
    vec3 H = normalize(L + V);

    float ambient  = 0.12;
    float diffuse  = max(dot(sN, L), 0.0) * 0.65;
    float specular = pow(max(dot(sN, H), 0.0), 48.0) * 0.5;

    // 简单 AO: 用 SDF 采样估算环境遮蔽
    float ao = 1.0;
    for (int i = 1; i <= 3; i++)
    {
        float dist = float(i) * 0.15;
        float sampled = sceneSDF(p + sN * dist);
        ao -= (dist - sampled) * 0.25 / float(i);
    }
    ao = clamp(ao, 0.3, 1.0);

    vec3 color = baseColor * (ambient + diffuse) * ao + vec3(specular);

    // 深度雾化: 远处渐变到背景色
    float fog = 1.0 - exp(-t * t * 0.002);
    color = mix(color, u_BgColor, fog);

    FragColor = vec4(color, 1.0);
}
