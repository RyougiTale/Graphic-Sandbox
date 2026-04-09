#version 460 core

in vec2 v_UV;
out vec4 FragColor;

// ============================================================
// Uniforms
// ============================================================

uniform mat4  u_InvView;
uniform mat4  u_InvProjection;
uniform vec3  u_CameraPos;
uniform mat4  u_Rotation;       // 整体旋转 (用于 auto-rotate)
uniform mat4  u_InvRotation;    // 旋转的逆

uniform int   u_LatticeType;   // 0=SC, 1=BCC, 2=FCC, 3=Octet, 4=Kelvin
uniform float u_CellSize;
uniform float u_StrutRadius;
uniform float u_NodeRadius;
uniform float u_SmoothK;       // Smooth-Min 融合半径
uniform int   u_Repeat;

uniform int   u_MaxSteps;
uniform float u_MaxDist;
uniform float u_BoundingBox;

uniform vec3  u_StrutColor;
uniform vec3  u_NodeColor;
uniform vec3  u_BgColor;

// ============================================================
// SDF 基元 (Building Blocks)
// ============================================================
// SDF = Signed Distance Function (有符号距离函数)
// 给定空间中一点 p, 返回 p 到几何表面的最近距离
//   > 0: 点在物体外部
//   = 0: 点在物体表面上
//   < 0: 点在物体内部
// 这个"距离"性质让我们可以安全地沿射线步进 (sphere tracing)

// ---- Capsule SDF ----
// 胶囊体 = 线段 + 半径膨胀 (Minkowski sum of segment and sphere)
// 这是 strut-based 晶格的核心基元:
//   一根杆件 = 一个从 a 到 b 的胶囊体
//
// 算法:
//   1. 将点 p 投影到线段 a→b 上, 得到最近点 c
//   2. 距离 = |p - c| - radius
//
//        p
//       /|
//      / |  ← 这个距离
//     /  |
//    a---c---------b    (c 是 p 在线段上的投影)
//
float sdCapsule(vec3 p, vec3 a, vec3 b, float r)
{
    vec3 ab = b - a;
    vec3 ap = p - a;
    // t = 投影比例, clamp 到 [0,1] 保证在线段内
    float t = clamp(dot(ap, ab) / dot(ab, ab), 0.0, 1.0);
    // 线段上最近点
    vec3 closest = a + t * ab;
    return length(p - closest) - r;
}

// ---- Sphere SDF ----
// 最简单的 SDF: 点到球心的距离 - 半径
float sdSphere(vec3 p, vec3 center, float r)
{
    return length(p - center) - r;
}

// ---- Box SDF ----
float sdBox(vec3 p, float halfSize)
{
    vec3 d = abs(p) - vec3(halfSize);
    return length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0);
}

// ============================================================
// Smooth-Min (SDF 融合的核心)
// ============================================================
// 普通 min(a, b) 产生锐利的交线 (像两个物体直接相交)
// Smooth-Min 在 a ≈ b 的区域产生平滑过渡 (圆角)
//
// 对比:
//   min(a, b)         → 两根管子十字交叉, 交线是尖锐的棱
//   smin(a, b, 0.1)   → 交叉处自动长出圆角, k=0.1 控制圆角大小
//
// 原理 (多项式版本):
//   当 a 和 b 的差距 < k 时, 结果比 min(a,b) 更小
//   h = 0.5 + 0.5*(b-a)/k 是混合因子:
//     h=1 → 完全取 a (a 远小于 b)
//     h=0 → 完全取 b (b 远小于 a)
//     h=0.5 → a ≈ b, 产生最大的圆角凹陷
//   最后的 -k*h*(1-h) 项是"额外的凹陷量", 让过渡区域更圆润
//
// k 值含义:
//   k=0   → 退化为普通 min, 没有圆角
//   k=0.05 → 很小的圆角, 像精密加工的倒角
//   k=0.15 → 中等圆角, 像 3D 打印的自然融合
//   k=0.5  → 很大的圆角, 物体几乎融为一体
//
// 这就是 nTop 的 "魔法":
//   传统 CAD: 先布尔合并两根杆, 再手动添加圆角 → 圆角经常失败
//   SDF + smin: 融合自动产生圆角, 且永远不会有拓扑错误
//
float smin(float a, float b, float k)
{
    float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
    return mix(b, a, h) - k * h * (1.0 - h);
}

// 带颜色混合的 smin — 在融合距离的同时混合材质
// 返回: 融合后的距离
// outMix: 混合因子 (0.0 = 全取 b 的颜色, 1.0 = 全取 a 的颜色)
float sminColor(float a, float b, float k, out float outMix)
{
    float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
    outMix = h;
    return mix(b, a, h) - k * h * (1.0 - h);
}

// ============================================================
// 单元格内的杆件定义
// ============================================================
// 每种晶格类型定义了一个单元格 (unit cell) 内的节点和边
// 然后通过 mod (取模) 操作在空间中无限重复
//
// 与 LatticeDemo 的关键区别:
//   LatticeDemo: 遍历所有重复单元, 为每条边生成一对 (球体,圆柱) mesh
//     → N 个单元 = N × edges × (sphere + cylinder) 个 draw call
//     → 节点处圆柱直接穿插, 没有圆角
//
//   SDFLatticeDemo: 空间取模折叠到一个单元格, 评估该单元格内所有杆件的 SDF
//     → 无论多少重复, GPU 只评估一个单元格的杆件数 (6~24 根)
//     → smooth-min 自动产生圆角
//     → 代价: 每个像素每步都要算一次, 但这是 O(1) per cell

// 将世界坐标折叠到单元格内 (域重复)
// 这是 SDF 无限重复的标准技巧:
//   p_local = mod(p, cellSize) 把无限空间映射到一个单元格内
//   但要注意: 靠近单元格边界的杆件会被截断
//   解决: 同时评估相邻单元格 (下面用 offset 循环)
vec3 domainRepeat(vec3 p, float cellSize)
{
    return mod(p + cellSize * 0.5, cellSize) - cellSize * 0.5;
}

// 评估单个单元格内所有杆件的 SDF, 并输出融合后的距离和颜色混合
float evalCell(vec3 p, float halfCell, out float nodeMix)
{
    float d = 1e10;     // 当前最小距离
    float nm = 0.0;     // 节点混合因子
    float h = halfCell; // 半格尺寸, 方便写坐标

    // 辅助: 添加一根杆件并用 smooth-min 融合
    // 同时添加两端的节点球 (如果 nodeRadius > 0)
    #define ADD_STRUT(ax, ay, az, bx, by, bz)                                 \
    {                                                                          \
        vec3 sa = vec3(ax, ay, az) * h;                                        \
        vec3 sb = vec3(bx, by, bz) * h;                                        \
        float ds = sdCapsule(p, sa, sb, u_StrutRadius);                        \
        float mixVal;                                                          \
        d = sminColor(d, ds, u_SmoothK, mixVal);                              \
        nm = mix(nm, 0.0, mixVal);                                             \
        if (u_NodeRadius > 0.001)                                              \
        {                                                                      \
            float dn1 = sdSphere(p, sa, u_NodeRadius);                         \
            d = sminColor(d, dn1, u_SmoothK, mixVal);                          \
            nm = mix(nm, 1.0, mixVal);                                         \
            float dn2 = sdSphere(p, sb, u_NodeRadius);                         \
            d = sminColor(d, dn2, u_SmoothK, mixVal);                          \
            nm = mix(nm, 1.0, mixVal);                                         \
        }                                                                      \
    }

    // ----------------------------------------------------------------
    // Simple Cubic (SC) — 简单立方晶格
    // ----------------------------------------------------------------
    // 历史: 14 种布拉维晶格 (Bravais lattice) 中最简单的一种
    //   Auguste Bravais (1850) 证明了三维空间中恰好存在 14 种不同的点阵
    //   SC 是其中对称性最高、结构最简单的
    //
    // 几何特征:
    //   - 每个节点有 6 个最近邻 (配位数 Z=6), 分别沿 ±x, ±y, ±z
    //   - 杆件全部沿坐标轴方向, 没有对角线
    //   - 填充率仅 52% (π/6) — 是所有立方晶格中最松散的
    //   - 外观: 最规则的"脚手架", 正交网格
    //
    //        +-------+
    //       /|      /|        每个 + 是一个节点
    //      / |     / |        每条 — | / 是一根杆件
    //     +--+----+  |        配位数 Z=6: 上下左右前后
    //     |  +----+--+
    //     | /     | /
    //     |/      |/
    //     +-------+
    //
    // 力学特性:
    //   - 弯曲主导 (bending-dominated): 杆件在载荷下主要发生弯曲而非拉伸
    //   - 刚度 E* ∝ ρ² (相对密度的平方) — 效率较低
    //   - 各向异性严重: 沿轴方向强, 对角线方向弱 (剪切刚度为零!)
    //   - 一推就会变形成平行四边形 (缺少三角形稳定)
    //
    // SDF 实现:
    //   因为域重复 (domainRepeat), 空间被折叠到一个单元格
    //   我们只需 3 条杆件: 沿 x, y, z 各一条
    //   域重复会自动让相邻单元格的杆件首尾相连
    //
    // 自然界: 极少见 (因为不稳定)
    //   唯一以 SC 结晶的元素是钋 (Polonium, Po) — 放射性元素
    //   工程中作为教学用的基准结构
    // ----------------------------------------------------------------
    if (u_LatticeType == 0)
    {
        ADD_STRUT(-1, 0, 0,  1, 0, 0);
        ADD_STRUT( 0,-1, 0,  0, 1, 0);
        ADD_STRUT( 0, 0,-1,  0, 0, 1);
    }

    // ----------------------------------------------------------------
    // Body-Centered Cubic (BCC) — 体心立方晶格
    // ----------------------------------------------------------------
    // 历史: 由 William Barlow (1883) 首次系统描述
    //   是 14 种布拉维晶格之一, 在冶金学中极为重要
    //
    // 几何特征:
    //   - SC 的 8 个角 + 1 个体心 → 每个体心连接 8 个角
    //   - 配位数 Z=8 (体心到 8 个角) + 6 (SC 的轴向) = 总共 14 根杆件
    //   - 体对角线长度 = a√3 ≈ 1.73a (a=晶格常数)
    //   - 填充率 68% (比 SC 高, 但不如 FCC)
    //
    //        +---/---+
    //       /| /    /|       / = 体对角线 (从角到体心)
    //      / |/ \  / |       X = 体心节点
    //     +--X----+  |       每个体心连接 8 个角
    //     |  +--/-+--+       角上的节点由相邻单元格共享
    //     | / /   | /
    //     |/ /    |/
    //     +---\---+
    //
    // 力学特性:
    //   - 仍然是弯曲主导, 但比 SC 好很多
    //   - 体对角线提供了各方向的支撑, 各向同性显著改善
    //   - E* ∝ ρ² (与 SC 相同的 scaling, 但系数更大)
    //   - 常用于需要各向同性但不追求极致刚度的场景
    //
    // 自然界:
    //   α-铁 (室温下的铁), 钨 (W), 铬 (Cr), 钼 (Mo), 钒 (V)
    //   这些金属硬度高、熔点高, BCC 结构是原因之一
    //   BCC 金属的典型特征: 低温脆性转变 (ductile-to-brittle transition)
    //
    // 3D 打印应用:
    //   BCC 晶格是 SLM/DMLS 金属打印中最常用的填充结构之一
    //   因为体对角线不需要支撑 (悬垂角 ≈ 35°, 低于临界 45°)
    // ----------------------------------------------------------------
    else if (u_LatticeType == 1)
    {
        // SC 的 3 条轴向边
        ADD_STRUT(-1, 0, 0,  1, 0, 0);
        ADD_STRUT( 0,-1, 0,  0, 1, 0);
        ADD_STRUT( 0, 0,-1,  0, 0, 1);
        // 4 条体对角线 (从中心到 4 个角, 另 4 个由域重复补全)
        ADD_STRUT( 0, 0, 0,  1, 1, 1);
        ADD_STRUT( 0, 0, 0,  1, 1,-1);
        ADD_STRUT( 0, 0, 0,  1,-1, 1);
        ADD_STRUT( 0, 0, 0,  1,-1,-1);
    }

    // ----------------------------------------------------------------
    // Face-Centered Cubic (FCC) — 面心立方晶格
    // ----------------------------------------------------------------
    // 历史: 同样由 Bravais (1850) 分类
    //   FCC 和 HCP 是自然界中最常见的两种密堆积结构
    //
    // 几何特征:
    //   - 8 个角 + 6 个面心 → 每个面的中心有一个额外节点
    //   - 配位数 Z=12: 每个节点有 12 个等距最近邻
    //     (这是三维空间中配位数的最大值, 与 HCP 并列)
    //   - 面对角线长度 = a√2/2 ≈ 0.707a
    //   - 填充率 74% — 这是球体密堆积的理论上限 (Kepler 猜想, 2005 年被 Hales 证明)
    //
    //        +---X---+      X = 面心节点
    //       /|      /|      每个面的中心都有一个节点
    //      X |     X |      面对角线从角连到面心
    //     /  X    /  X      12 条面对角线提供了完整的三角形稳定
    //     +---X---+  |
    //     |  +---X+--+
    //     X /     X /
    //     |/      |/
    //     +---X---+
    //
    // 力学特性:
    //   - 拉伸主导 (stretch-dominated): 三角形化的结构让杆件主要承受拉/压
    //   - E* ∝ ρ (相对密度的一次方!) — 比弯曲主导的 ρ² 高效得多
    //   - 各向同性好: 12 个方向均匀分布在球面上
    //   - 但注意: 纯 FCC (只有面对角线, 没有轴向边) 沿坐标轴方向刚度为零!
    //     这就是为什么 Octet Truss = FCC + SC (补上轴向边)
    //
    // 自然界:
    //   铝 (Al), 铜 (Cu), 金 (Au), 银 (Ag), 镍 (Ni), 铂 (Pt)
    //   这些金属延展性好, 易于加工 — FCC 结构提供了更多滑移系 (12 个)
    //   FCC 金属不存在低温脆性转变 (与 BCC 的关键区别)
    // ----------------------------------------------------------------
    else if (u_LatticeType == 2)
    {
        // 6 条面对角线 (域重复自动补全另外 6 条)
        ADD_STRUT(-1,-1, 0,  1, 1, 0);
        ADD_STRUT(-1, 1, 0,  1,-1, 0);
        ADD_STRUT(-1, 0,-1,  1, 0, 1);
        ADD_STRUT(-1, 0, 1,  1, 0,-1);
        ADD_STRUT( 0,-1,-1,  0, 1, 1);
        ADD_STRUT( 0,-1, 1,  0, 1,-1);
    }

    // ----------------------------------------------------------------
    // Octet Truss — 八面体桁架 (FCC + SC 的组合)
    // ----------------------------------------------------------------
    // 历史: Alexander Graham Bell (是的, 电话发明者) 在 1903 年就用八面体桁架
    //   造风筝和塔架。后来 Buckminster Fuller (1950s) 将其系统化并命名
    //   为 "octet truss", 证明了它的力学最优性。
    //   现代: 2014 年 MIT 的 Zheng et al. 用微加工制造了纳米级 octet truss,
    //   实现了密度接近气凝胶但刚度接近金属的"超材料" (ultralight metallic microlattice)
    //
    // 几何特征:
    //   - FCC 的 12 条面对角线 + SC 的 6 条轴向边 = 总共 18 条杆件/节点
    //   - 配位数 Z=12+6=18 — 远超 Maxwell 准则的最低要求
    //     (Maxwell 准则: 3D 桁架刚性要求 Z ≥ 6, 最优要求 Z ≥ 12)
    //   - 结构中充满了正八面体和正四面体:
    //     正八面体由 6 条面对角线围成
    //     正四面体填充八面体之间的缝隙
    //     这种排列被称为 "octet 密铺"
    //
    //     从侧面看:
    //         △  ▽  △  ▽       △ = 四面体, ▽ = 倒四面体
    //         ▽  △  ▽  △       中间 = 八面体
    //         △  ▽  △  ▽       完美的三角形化 → 拉伸主导
    //
    // 力学特性 (这是所有杆状晶格中最重要的):
    //   - 完全拉伸主导 (fully stretch-dominated)
    //   - E* ∝ ρ — 线性缩放, 理论最优
    //   - 各向同性: FCC 的面对角线补了球面上的 12 个方向,
    //     SC 的轴向边补了 6 个坐标轴方向, 合在一起几乎完美各向同性
    //   - 在 Ashby chart (刚度 vs 密度图) 上, octet truss 位于
    //     Hashin-Shtrikman 上界附近 — 即理论上能达到的最大刚度/密度比
    //
    // 应用:
    //   - NASA: 卫星和航天器的微桁架结构
    //   - 建筑: 大跨度空间结构 (如 Eden Project 生物穹顶)
    //   - 3D 打印: 轻量化零件的首选填充模式 (nTop, Materialise 等软件默认)
    //   - 能量吸收: 碰撞缓冲、头盔内衬 (均匀坍塌, 不会局部失稳)
    // ----------------------------------------------------------------
    else if (u_LatticeType == 3)
    {
        // SC 的 3 条轴向边
        ADD_STRUT(-1, 0, 0,  1, 0, 0);
        ADD_STRUT( 0,-1, 0,  0, 1, 0);
        ADD_STRUT( 0, 0,-1,  0, 0, 1);
        // FCC 的 6 条面对角线
        ADD_STRUT(-1,-1, 0,  1, 1, 0);
        ADD_STRUT(-1, 1, 0,  1,-1, 0);
        ADD_STRUT(-1, 0,-1,  1, 0, 1);
        ADD_STRUT(-1, 0, 1,  1, 0,-1);
        ADD_STRUT( 0,-1,-1,  0, 1, 1);
        ADD_STRUT( 0,-1, 1,  0, 1,-1);
    }

    // ----------------------------------------------------------------
    // Kelvin Cell (截角八面体, Truncated Octahedron)
    // ----------------------------------------------------------------
    // 历史: Lord Kelvin (William Thomson) 在 1887 年提出著名的 "Kelvin 问题":
    //   "将空间分割成等体积的单元, 怎样使总界面面积最小?"
    //   他的答案: 截角八面体 (truncated octahedron), 即把正八面体的 6 个顶点切掉
    //   这个猜想维持了 100 多年, 直到 1993 年 Weaire 和 Phelan 发现了
    //   表面积更小 0.3% 的结构 (Weaire-Phelan 结构, 北京水立方的外壳设计灵感来源)
    //
    // 几何特征:
    //   - 14 个面: 8 个正六边形 + 6 个正方形
    //   - 24 个顶点, 36 条棱
    //   - 配位数 Z=3: 每个顶点恰好连接 3 条棱 (与 SC/BCC/FCC 完全不同!)
    //   - 这是唯一一种能单独密铺三维空间的阿基米德多面体
    //     (阿基米德多面体 = 由多种正多边形组成的半正多面体)
    //   - 截角八面体的体积 = 8√2 · a³ (a = 棱长)
    //
    //     俯视一层:
    //       __    __    __
    //      /  \__/  \__/  \      六边形面密铺
    //      \__/  \__/  \__/      正方形面连接上下层
    //      /  \__/  \__/  \
    //      \__/  \__/  \__/
    //
    // 力学特性:
    //   - 弯曲主导 (Z=3 < Maxwell 准则的 6)
    //   - 但! 因为配位数低, 单个杆件可以更粗 (在相同密度下)
    //   - 能量吸收优于 Octet: 变形均匀, 没有局部屈曲
    //   - 在低密度区域 (ρ < 10%), Kelvin cell 的刚度可以与 Octet 媲美
    //
    // 自然界与应用:
    //   - 金属泡沫 (如铝泡沫) 的气泡形状近似截角八面体
    //   - 蜂蜡的三维结构也接近这个形状
    //   - 生物医学: 骨科支架 — Z=3 的低连通度有利于细胞迁移
    //   - 建筑: 北京水立方的外壳 (Weaire-Phelan 的变体, 但 Kelvin 是基础)
    //   - 化学工程: 填料塔中的泡沫填料模型
    //
    // 顶点坐标:
    //   24 个顶点是 (0, ±0.5, ±1) 的所有排列 (3种轴 × 4种符号 × 2种位置 = 24)
    //   归一化到 [-1,1] 单元格
    // ----------------------------------------------------------------
    else if (u_LatticeType == 4)
    {
        // Kelvin cell 的棱 — 连接截角八面体的 24 个顶点
        // 顶点在 (±1, ±0.5, 0) 的所有排列上
        // 这里列出产生完整单元格所需的代表性杆件

        // 正方形面的边 (沿轴方向)
        ADD_STRUT(-0.5, 0,-1,  0.5, 0,-1);
        ADD_STRUT(-0.5, 0, 1,  0.5, 0, 1);
        ADD_STRUT( 0,-0.5,-1,  0, 0.5,-1);
        ADD_STRUT( 0,-0.5, 1,  0, 0.5, 1);
        ADD_STRUT(-1,-0.5, 0, -1, 0.5, 0);
        ADD_STRUT( 1,-0.5, 0,  1, 0.5, 0);
        ADD_STRUT(-1, 0,-0.5, -1, 0, 0.5);
        ADD_STRUT( 1, 0,-0.5,  1, 0, 0.5);
        ADD_STRUT(-0.5,-1, 0,  0.5,-1, 0);
        ADD_STRUT(-0.5, 1, 0,  0.5, 1, 0);
        ADD_STRUT( 0,-1,-0.5,  0,-1, 0.5);
        ADD_STRUT( 0, 1,-0.5,  0, 1, 0.5);

        // 六边形面的斜边 (连接相邻正方形面的顶点)
        ADD_STRUT( 0.5, 0,-1,  1,-0.5, 0);
        ADD_STRUT( 0.5, 0,-1,  1, 0.5, 0);
        ADD_STRUT(-0.5, 0,-1, -1,-0.5, 0);
        ADD_STRUT(-0.5, 0,-1, -1, 0.5, 0);
        ADD_STRUT( 0.5, 0, 1,  1,-0.5, 0);
        ADD_STRUT( 0.5, 0, 1,  1, 0.5, 0);
        ADD_STRUT(-0.5, 0, 1, -1,-0.5, 0);
        ADD_STRUT(-0.5, 0, 1, -1, 0.5, 0);

        ADD_STRUT( 0, 0.5,-1,  0.5, 1, 0);
        ADD_STRUT( 0, 0.5,-1, -0.5, 1, 0);
        ADD_STRUT( 0,-0.5,-1,  0.5,-1, 0);
        ADD_STRUT( 0,-0.5,-1, -0.5,-1, 0);
        ADD_STRUT( 0, 0.5, 1,  0.5, 1, 0);
        ADD_STRUT( 0, 0.5, 1, -0.5, 1, 0);
        ADD_STRUT( 0,-0.5, 1,  0.5,-1, 0);
        ADD_STRUT( 0,-0.5, 1, -0.5,-1, 0);
    }

    #undef ADD_STRUT

    nodeMix = nm;
    return d;
}

// ============================================================
// 完整场景 SDF
// ============================================================

float sceneSDF(vec3 p, out float nodeMix)
{
    float halfCell = u_CellSize * 0.5;

    // 域重复: 将无限空间折叠到一个单元格
    vec3 lp = domainRepeat(p, u_CellSize);

    // 评估当前单元格
    float nm;
    float d = evalCell(lp, halfCell, nm);

    // 与包围盒求交, 限制渲染范围
    float totalSize = u_CellSize * float(u_Repeat) * 0.5;
    float boxD = sdBox(p, totalSize);
    d = max(d, boxD);

    nodeMix = nm;
    return d;
}

// 不需要 nodeMix 的版本 (用于法线计算)
float sceneSDF_simple(vec3 p)
{
    float nm;
    return sceneSDF(p, nm);
}

// ============================================================
// 法线 (中心差分)
// ============================================================

vec3 calcNormal(vec3 p)
{
    vec2 e = vec2(0.001, 0.0);
    return normalize(vec3(
        sceneSDF_simple(p + e.xyy) - sceneSDF_simple(p - e.xyy),
        sceneSDF_simple(p + e.yxy) - sceneSDF_simple(p - e.yxy),
        sceneSDF_simple(p + e.yyx) - sceneSDF_simple(p - e.yyx)
    ));
}

// ============================================================
// Ray-AABB
// ============================================================

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
    // 重建世界空间射线
    vec2 ndc = v_UV * 2.0 - 1.0;
    vec4 clipPos = vec4(ndc, -1.0, 1.0);
    vec4 viewPos = u_InvProjection * clipPos;
    viewPos = vec4(viewPos.xy, -1.0, 0.0);
    vec3 rd = normalize((u_InvView * viewPos).xyz);
    vec3 ro = u_CameraPos;

    // 将射线变换到晶格的旋转空间
    // (旋转晶格 = 反向旋转射线, 避免在 SDF 中做旋转)
    vec3 localRo = (u_InvRotation * vec4(ro, 1.0)).xyz;
    vec3 localRd = normalize((u_InvRotation * vec4(rd, 0.0)).xyz);

    // AABB 预判
    float totalSize = u_CellSize * float(u_Repeat) * 0.5 + 0.1;
    float tNear, tFar;
    if (!intersectBox(localRo, localRd, totalSize, tNear, tFar))
    {
        FragColor = vec4(u_BgColor, 1.0);
        return;
    }

    float t = max(tNear, 0.0);

    // Ray marching
    bool hit = false;
    float nodeMix = 0.0;
    for (int i = 0; i < u_MaxSteps; i++)
    {
        vec3 p = localRo + localRd * t;
        float nm;
        float d = sceneSDF(p, nm);

        if (d < 0.001)
        {
            hit = true;
            nodeMix = nm;
            break;
        }

        t += d;
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

    // 将法线旋转回世界空间 (用于光照)
    vec3 worldN = normalize((u_Rotation * vec4(N, 0.0)).xyz);
    vec3 sN = faceforward(worldN, rd, worldN);

    // 材质: 根据 smooth-min 的混合因子混合 strut/node 颜色
    vec3 baseColor = mix(u_StrutColor, u_NodeColor, nodeMix);

    // Blinn-Phong (金属感)
    vec3 L = normalize(vec3(0.5, 1.0, 0.8));
    vec3 V = -rd;
    vec3 H = normalize(L + V);

    float ambient  = 0.10;
    float diffuse  = max(dot(sN, L), 0.0) * 0.60;
    float specular = pow(max(dot(sN, H), 0.0), 64.0) * 0.7;

    // SDF-based AO
    float ao = 1.0;
    for (int i = 1; i <= 4; i++)
    {
        float dist = float(i) * 0.12;
        float sampled = sceneSDF_simple(p + N * dist);
        ao -= (dist - sampled) * 0.22 / float(i);
    }
    ao = clamp(ao, 0.2, 1.0);

    vec3 color = baseColor * (ambient + diffuse) * ao + vec3(specular) * ao;

    // 深度雾
    float fog = 1.0 - exp(-t * t * 0.0015);
    color = mix(color, u_BgColor, fog);

    FragColor = vec4(color, 1.0);
}
