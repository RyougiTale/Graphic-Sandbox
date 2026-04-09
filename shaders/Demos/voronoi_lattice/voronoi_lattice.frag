#version 460 core

in vec2 v_UV;
out vec4 FragColor;

// ============================================================
// Uniforms
// ============================================================

uniform mat4  u_InvView;
uniform mat4  u_InvProjection;
uniform vec3  u_CameraPos;
uniform mat4  u_Rotation;
uniform mat4  u_InvRotation;

uniform int   u_VoronoiType;   // 0=StandardFoam, 1=ThickWall, 2=Skeleton, 3=Smooth
uniform float u_CellScale;
uniform float u_WallThickness;
uniform float u_Randomness;
uniform int   u_Seed;
uniform float u_Time;
uniform int   u_Animate;

uniform int   u_MaxSteps;
uniform float u_MaxDist;
uniform float u_BoundingBox;

uniform vec3  u_WallColor;
uniform vec3  u_CavityColor;
uniform vec3  u_BgColor;

// 4 种 Voronoi 泡沫变体速查表:
// Type           SDF 构造方式                    外观特征            对应自然结构
// StandardFoam   d2 - d1 < t                     薄壁泡沫            肥皂泡、啤酒泡沫
// ThickWall      d1 > r (掏空中心)               厚壁蜂窝            珊瑚骨骼、面包气孔
// Skeleton       d2 - d1 与 d3 - d1 联合判定     棱线骨架            骨小梁 (trabeculae)
// Smooth         smoothstep 软化壁面             平滑有机体          生物组织、海绵

// ============================================================
// 哈希函数 — 将整数网格坐标映射为伪随机 vec3
// ============================================================
// 为什么需要哈希?
//   Voronoi 需要在每个网格单元内放一个"种子点"
//   种子点的位置必须是确定性的 (同一单元格每次算结果一样)
//   但又要看起来随机 (否则就是规则网格了)
//   → 哈希函数: 输入整数坐标, 输出 [0,1] 范围的伪随机数
//
// 原理 (这是 Inigo Quilez 的经典 GPU 哈希):
//   1. 把 3D 整数坐标编码为一个 dot product (类似"指纹")
//   2. 用 sin() 制造混沌 — sin(大数) 的小数部分是伪随机的
//   3. fract() 取小数部分, 映射到 [0,1]
//
// 注意: 这不是密码学安全的哈希, 但对图形学足够了
//   更高质量的替代: xxHash, PCG, 或 Murmur hash 的 GPU 移植
//   但 sin-hash 最简单且在大多数场景下质量足够

vec3 hash3(vec3 p)
{
    // 加入 seed 让不同 seed 产生不同的随机模式
    p = p + float(u_Seed) * 0.131;
    p = vec3(
        dot(p, vec3(127.1, 311.7, 74.7)),
        dot(p, vec3(269.5, 183.3, 246.1)),
        dot(p, vec3(113.5, 271.9, 124.6))
    );
    return fract(sin(p) * 43758.5453123);
}

// ============================================================
// 3D Voronoi 距离场
// ============================================================
// 这是整个 demo 的核心算法
//
// Voronoi 图的定义:
//   给定空间中 N 个种子点, Voronoi 图将空间分割成 N 个区域
//   每个区域内的所有点到它对应的种子点比到其他任何种子点都近
//   区域之间的边界叫 "Voronoi 面" (3D) 或 "Voronoi 边" (2D)
//
// 暴力算法: O(N) — 遍历所有种子点, 找最近和次近
//   当 N 很大时 (成千上万个种子), 这太慢了
//
// 网格加速: O(27) — 只检查当前单元格和 26 个邻居
//   关键洞察: 如果种子点在单元格内均匀分布,
//   那么最近的种子点一定在当前单元格或相邻的 26 个单元格中
//   (3×3×3 = 27 个单元格)
//   这把 O(N) 降到了常数 O(27), 适合 GPU 实时计算!
//
// 输出:
//   d1 = 到最近种子点的距离
//   d2 = 到次近种子点的距离
//   d3 = 到第三近种子点的距离 (Skeleton 模式用)
//   id = 最近种子点的身份标识 (用于给每个细胞着色)
//
// 这些距离的组合可以构造不同的泡沫形态:
//   d2 - d1: 在 Voronoi 边界处为 0, 远离边界时增大 → 泡沫壁
//   d1:      在种子点处为 0, 远离时增大 → 细胞中心/掏空

struct VoronoiResult
{
    float d1;   // 最近距离
    float d2;   // 次近距离
    float d3;   // 第三近距离
    vec3  id;   // 最近种子点的哈希值 (用作细胞 ID / 颜色)
};

VoronoiResult voronoi3D(vec3 p)
{
    // 缩放到 Voronoi 空间
    vec3 sp = p / u_CellScale;

    // 当前所在的网格单元 (整数坐标)
    vec3 cellId = floor(sp);
    // 单元格内的小数坐标
    vec3 cellUV = fract(sp);

    float d1 = 100.0;
    float d2 = 100.0;
    float d3 = 100.0;
    vec3  nearestId = vec3(0.0);

    // 遍历 3×3×3 = 27 个相邻单元格
    // 为什么是 -1 到 +1?
    //   当前点可能靠近单元格边界, 最近的种子可能在相邻单元格
    //   3×3×3 保证不会遗漏 (前提: 种子偏移 < 1 个单元格)
    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    for (int z = -1; z <= 1; z++)
    {
        vec3 neighbor = vec3(x, y, z);
        vec3 neighborCell = cellId + neighbor;

        // 该单元格的种子点位置 (网格中心 + 随机偏移)
        vec3 randOffset = hash3(neighborCell);

        // 动画: 种子点缓慢漂移
        if (u_Animate != 0)
        {
            // 每个种子有不同的漂移频率和相位
            randOffset = 0.5 + 0.5 * sin(u_Time + 6.2831 * randOffset);
        }

        // 种子点在该单元格内的位置
        // randomness 控制偏移量: 0=正中心(变成规则网格), 1=完全随机
        vec3 seedPos = neighbor + mix(vec3(0.5), randOffset, u_Randomness);

        // 到该种子点的距离
        float dist = length(cellUV - seedPos);

        // 维护 d1 < d2 < d3 (插入排序)
        if (dist < d1)
        {
            d3 = d2;
            d2 = d1;
            d1 = dist;
            nearestId = randOffset;
        }
        else if (dist < d2)
        {
            d3 = d2;
            d2 = dist;
        }
        else if (dist < d3)
        {
            d3 = dist;
        }
    }

    // 还原到世界空间尺度
    return VoronoiResult(d1 * u_CellScale, d2 * u_CellScale, d3 * u_CellScale, nearestId);
}

// ============================================================
// SDF: 从 Voronoi 距离场构造不同的泡沫形态
// ============================================================
// 这是将数学上的 Voronoi 图转化为可渲染实体的关键步骤
//
// 与 TPMS 的对比:
//   TPMS: 一个解析公式 F(x,y,z) → 直接就是 SDF (除以梯度)
//   Voronoi: 需要从离散距离 d1, d2, d3 构造连续的 SDF
//   技巧: Voronoi 距离本身就有"距离"的语义, 可以直接当 SDF 用
//         (不是精确的 SDF, 但对 ray marching 足够)

float voronoiSDF(vec3 p, out vec3 cellColor)
{
    VoronoiResult vr = voronoi3D(p);
    cellColor = vr.id; // 用种子哈希作为每个细胞的颜色种子

    float d;

    // ----------------------------------------------------------------
    // Standard Foam (标准泡沫)
    // ----------------------------------------------------------------
    // 灵感: 肥皂泡、啤酒泡沫、金属泡沫 (闭孔型)
    //
    // 原理: d2 - d1 表示"离 Voronoi 边界有多远"
    //   - 在 Voronoi 边界上: d1 = d2 (到两个最近种子等距) → d2-d1 = 0
    //   - 在细胞中心: d1 很小, d2 较大 → d2-d1 > 0
    //   - 取 d2 - d1 < thickness 作为壁面
    //
    // 几何直觉:
    //   想象用肥皂水吹泡泡 — 每个泡泡就是一个 Voronoi 细胞
    //   泡泡之间的薄膜就是 d2-d1 = 0 的等值面
    //   thickness 控制薄膜的厚度
    //
    //    ·····|·····|·····       · = 空气 (细胞内部)
    //    ·  o |     | o  ·       o = 种子点 (泡泡中心)
    //    ·····|·····|·····       | = 壁面 (d2-d1 ≈ 0)
    //    ·····|·····|·····
    //    ·  o |     | o  ·
    //    ·····|·····|·····
    //
    // 物理对应:
    //   - 闭孔泡沫 (closed-cell foam): 每个气泡被完整的壁包围
    //   - Plateau 定律: 泡沫膜总是以 120° 角相交 (最小化表面能)
    //   - Voronoi 自然满足这个性质 (当种子均匀分布时)
    //
    // 自然界: 肥皂泡, 面包内部, 浮石 (火山岩), 聚氨酯泡沫
    // ----------------------------------------------------------------
    if (u_VoronoiType == 0)
    {
        d = (vr.d2 - vr.d1) - u_WallThickness;
    }

    // ----------------------------------------------------------------
    // Thick Wall (厚壁泡沫)
    // ----------------------------------------------------------------
    // 灵感: 珊瑚骨骼、面包气孔、发泡水泥
    //
    // 原理: 不同于 Standard Foam 的"薄壁",
    //   这里用 d1 (到最近种子的距离) 掏空每个细胞中心
    //   d1 > threshold → 细胞边缘 (壁) 是实体
    //   d1 < threshold → 细胞中心是空腔
    //
    //   换一种理解: 每个种子点膨胀成一个球,
    //   球与球之间的间隙就是壁面
    //   threshold 控制球的大小 (即空腔大小)
    //
    //    ████|██████|████        █ = 实体壁
    //    ██  ○  ████  ○  ██     ○ = 空腔 (d1 < threshold)
    //    ████|██████|████
    //    ████|██████|████
    //    ██  ○  ████  ○  ██
    //    ████|██████|████
    //
    // 与 Standard Foam 的区别:
    //   Standard: 壁是薄片 (d2-d1), 空腔之间有共享的壁
    //   ThickWall: 壁是实体块, 空腔是从实体中挖出的球
    //   → Standard 更像肥皂泡, ThickWall 更像瑞士奶酪
    //
    // 自然界: 珊瑚 (Coral), 多孔陶瓷, 发泡混凝土, 火山浮石
    // 应用: 过滤器, 隔热材料, 骨科植入物 (大孔径促进血管生长)
    // ----------------------------------------------------------------
    else if (u_VoronoiType == 1)
    {
        float hollowRadius = u_CellScale * 0.5 - u_WallThickness;
        d = hollowRadius - vr.d1;
    }

    // ----------------------------------------------------------------
    // Skeleton (骨架/骨小梁结构)
    // ----------------------------------------------------------------
    // 灵感: 松质骨 (cancellous bone) 的骨小梁 (trabeculae) 网络
    //
    // 原理: Voronoi 图中有三种几何元素:
    //   - 面 (Face): d2-d1 ≈ 0 (两个种子的等距面)
    //   - 棱 (Edge): d2-d1 ≈ 0 且 d3-d2 ≈ 0 (三个种子的等距线)
    //   - 顶点 (Vertex): d1 ≈ d2 ≈ d3 ≈ d4 (四个种子的等距点)
    //
    //   Standard Foam 保留了"面" (d2-d1 < t)
    //   Skeleton 只保留"棱" — 通过要求 d2-d1 和 d3-d1 都很小:
    //     max(d2-d1, d3-d2) < thickness
    //   这样面的中心 (远离棱的地方) 被去掉, 只剩下棱线
    //
    //   更直观的理解: 取 d3-d1 < thickness
    //   d3-d1 在棱上最小 (三个种子同时很近), 在面中心较大
    //
    //    · · · · · · · · ·       · = 空
    //    · · ·/· · ·\· · ·       / \ = 骨小梁 (Voronoi 棱)
    //    · · / · · · \ · ·       交汇点 = Voronoi 顶点
    //    · ·/· · · · ·\· ·
    //    · ·----+----· · ·       + = 节点 (3-4 条棱汇合)
    //    · ·\· · · · ·/· ·
    //    · · \ · · · / · ·
    //    · · ·\· · ·/· · ·
    //
    // 自然界:
    //   松质骨 (海绵骨): 人体骨骼内部的多孔结构
    //     - 骨小梁沿主应力方向排列 (Wolff 定律, 1892)
    //     - 典型孔径 0.5-1mm, 壁厚 0.1-0.3mm
    //     - 孔隙率 50-90%
    //   骨质疏松症: 骨小梁变细、断裂 → 可以通过调节 thickness 模拟
    //
    // 应用:
    //   - 骨科植入物: 模拟松质骨结构, 促进骨长入 (osseointegration)
    //   - 组织工程支架 (scaffold): 需要高连通性让细胞迁移
    //   - 钛合金 3D 打印: Ti-6Al-4V 的 Voronoi 骨架在髋关节假体中已商用
    // ----------------------------------------------------------------
    else if (u_VoronoiType == 2)
    {
        float edge = vr.d3 - vr.d1;
        d = edge - u_WallThickness * 2.0;
    }

    // ----------------------------------------------------------------
    // Smooth (平滑泡沫)
    // ----------------------------------------------------------------
    // 灵感: 海绵、生物组织、柔性泡沫
    //
    // 原理: Standard Foam 的壁面是"硬"的 (线性判定)
    //   Smooth 模式用 smoothstep 软化边界:
    //   smoothstep 是三阶 Hermite 插值: 3t² - 2t³
    //     - 在边界附近产生平滑的 S 形过渡
    //     - 导数在两端为 0, 不会有尖锐的转折
    //
    //   对比:
    //     Standard: SDF = (d2-d1) - thickness    → 硬边界, 像折纸
    //     Smooth:   SDF 经过 smoothstep 过渡     → 软边界, 像粘土
    //
    // 视觉效果: 壁面看起来更"胖", 有圆润的截面
    //   像肉眼看到的真实海绵 — 壁不是无限薄的平面, 而是有体积的弯曲表面
    //
    // 自然界: 天然海绵, 肺泡组织, 脂肪组织, 柔性聚氨酯泡沫
    // ----------------------------------------------------------------
    else
    {
        float edge = vr.d2 - vr.d1;
        float t = u_WallThickness;
        d = smoothstep(0.0, t, edge) * t - t * 0.5;
    }

    return d;
}

// 包围盒 SDF
float sdBox(vec3 p, float halfSize)
{
    vec3 q = abs(p) - vec3(halfSize);
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

// 场景 SDF
float sceneSDF(vec3 p, out vec3 cellColor)
{
    float foam = voronoiSDF(p, cellColor);
    float box  = sdBox(p, u_BoundingBox);
    return max(foam, box);
}

float sceneSDF_simple(vec3 p)
{
    vec3 dummy;
    return sceneSDF(p, dummy);
}

// 法线
// Voronoi 距离场不连续 (在 Voronoi 边界处梯度突变)
// 用自适应 epsilon: Skeleton 用更大的 epsilon 来平滑锯齿
vec3 calcNormal(vec3 p)
{
    // Skeleton 模式 (type==2) 产生 1D 棱线结构, 法线噪声大, 需要更大的采样半径
    // 其他模式是 2D 面结构, 法线相对稳定
    float eps = (u_VoronoiType == 2) ? 0.008 : 0.003;
    vec2 e = vec2(eps, 0.0);
    return normalize(vec3(
        sceneSDF_simple(p + e.xyy) - sceneSDF_simple(p - e.xyy),
        sceneSDF_simple(p + e.yxy) - sceneSDF_simple(p - e.yxy),
        sceneSDF_simple(p + e.yyx) - sceneSDF_simple(p - e.yyx)
    ));
}

// Ray-AABB
bool intersectBox(vec3 ro, vec3 rd, float halfSize, out float tNear, out float tFar)
{
    vec3 inv = 1.0 / rd;
    vec3 t1 = (-vec3(halfSize) - ro) * inv;
    vec3 t2 = ( vec3(halfSize) - ro) * inv;
    vec3 mn = min(t1, t2);
    vec3 mx = max(t1, t2);
    tNear = max(max(mn.x, mn.y), mn.z);
    tFar  = min(min(mx.x, mx.y), mx.z);
    return tNear < tFar && tFar > 0.0;
}

// ============================================================
// Main
// ============================================================

void main()
{
    vec2 ndc = v_UV * 2.0 - 1.0;
    vec4 clipPos = vec4(ndc, -1.0, 1.0);
    vec4 viewPos = u_InvProjection * clipPos;
    viewPos = vec4(viewPos.xy, -1.0, 0.0);
    vec3 rd = normalize((u_InvView * viewPos).xyz);
    vec3 ro = u_CameraPos;

    // 旋转到局部空间
    vec3 localRo = (u_InvRotation * vec4(ro, 1.0)).xyz;
    vec3 localRd = normalize((u_InvRotation * vec4(rd, 0.0)).xyz);

    float tNear, tFar;
    if (!intersectBox(localRo, localRd, u_BoundingBox + 0.1, tNear, tFar))
    {
        FragColor = vec4(u_BgColor, 1.0);
        return;
    }

    float t = max(tNear, 0.0);

    // Ray marching
    //
    // 为什么 Voronoi 需要特殊处理?
    // ──────────────────────────────
    // 真正的 SDF 保证: |∇d| = 1 (梯度恒为 1)
    // 这意味着 d(p) 就是到最近表面的精确距离, 可以安全步进 d
    //
    // 但 Voronoi 的 d2-d1 不满足这个性质:
    //   - 在远离 Voronoi 边界处, 梯度可以 > 1 (步长过大, 跳过薄壁!)
    //   - 在 Voronoi 边界处, 梯度可以突变 (不连续)
    //
    // 解决方案: 自适应步长
    //   1. 安全系数: d * factor (factor < 1, 让每步更保守)
    //   2. 步长上限: min(d * factor, maxStep) (防止一步跳过整面墙)
    //      maxStep 设为壁厚级别, 这样无论 SDF 有多不准, 都不会跨过薄壁
    //   3. Skeleton 模式更保守: 棱线是 1D 结构, 容差更小
    //
    // 代价: 更小的步长 = 更多迭代 = 更慢
    // 这就是 Voronoi ray marching 比 TPMS 慢的根本原因

    // 根据模式选择安全参数
    float stepFactor;
    float maxStep;
    if (u_VoronoiType == 2)
    {
        // Skeleton: 1D 棱线, 最容易跳过, 需要最保守
        stepFactor = 0.4;
        maxStep = u_WallThickness * 1.5;
    }
    else if (u_VoronoiType == 0)
    {
        // Standard Foam: 薄壁, 壁厚即最大安全步长
        stepFactor = 0.5;
        maxStep = u_WallThickness * 2.0;
    }
    else
    {
        // ThickWall / Smooth: 壁较厚, 可以稍微放宽
        stepFactor = 0.6;
        maxStep = u_WallThickness * 3.0;
    }
    // 步长下限: 防止在表面附近步长趋近于 0 导致死循环
    maxStep = max(maxStep, 0.02);

    bool hit = false;
    vec3 cellColor = vec3(0.0);
    for (int i = 0; i < u_MaxSteps; i++)
    {
        vec3 p = localRo + localRd * t;
        vec3 cc;
        float d = sceneSDF(p, cc);

        if (d < 0.001)
        {
            hit = true;
            cellColor = cc;
            break;
        }

        // 自适应步长: 安全系数 + 上限钳制
        t += min(d * stepFactor, maxStep);
        if (t > tFar || t > u_MaxDist)
            break;
    }

    if (!hit)
    {
        FragColor = vec4(u_BgColor, 1.0);
        return;
    }

    // 着色
    vec3 p = localRo + localRd * t;
    vec3 N = calcNormal(p);
    vec3 worldN = normalize((u_Rotation * vec4(N, 0.0)).xyz);
    vec3 sN = faceforward(worldN, rd, worldN);

    // 每个细胞用种子哈希给壁面微调颜色, 增加自然感
    vec3 baseColor = u_WallColor * (0.85 + 0.15 * cellColor);

    // Blinn-Phong
    vec3 L = normalize(vec3(0.5, 1.0, 0.8));
    vec3 V = -rd;
    vec3 H = normalize(L + V);

    float ambient  = 0.12;
    float diffuse  = max(dot(sN, L), 0.0) * 0.65;
    float specular = pow(max(dot(sN, H), 0.0), 32.0) * 0.3;

    // SDF-based AO
    float ao = 1.0;
    for (int i = 1; i <= 3; i++)
    {
        float dist = float(i) * 0.15;
        float sampled = sceneSDF_simple(p + N * dist);
        ao -= (dist - sampled) * 0.3 / float(i);
    }
    ao = clamp(ao, 0.2, 1.0);

    // 次表面散射近似: 壁越薄, 从背面透过来的光越多
    // 用 -L 方向的 SDF 采样估算壁厚
    float sss = 0.0;
    {
        // 沿光线反方向深入壁面, 看多深能穿透
        float thickness = sceneSDF_simple(p - L * 0.3);
        sss = smoothstep(-0.1, 0.15, thickness) * 0.15;
    }
    vec3 sssColor = u_CavityColor * sss;

    vec3 color = baseColor * (ambient + diffuse) * ao + vec3(specular) * ao + sssColor;

    // 深度雾
    float fog = 1.0 - exp(-t * t * 0.002);
    color = mix(color, u_BgColor, fog);

    FragColor = vec4(color, 1.0);
}
