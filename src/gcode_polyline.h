//------------------------------------------------------------------------------
// gcode_polyline.h - G-code path renderer with timeline effect (header-only)
//
// Renders G-code toolpath as thick polylines with alpha blending.
// Features timeline scrubbing: highlight a percentage along the path
// with gradual alpha fade to base value.
//------------------------------------------------------------------------------
#ifndef GCODE_POLYLINE_H
#define GCODE_POLYLINE_H

#include "platform.h"
#include "sokol_gfx.h"
#include "math3d.h"
#include "gcode_loader.h"
#include "shaders/instanced_line_shaders.h"
#include <math.h>
#include <stdlib.h>

// Embedded G-code data for web and iOS builds (no filesystem access)
#if defined(PLATFORM_WEB) || defined(PLATFORM_IOS)
#include "gcode_benchy_embedded.h"
#endif

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------
#define GCODE_POLYLINE_CAP_SEGMENTS 8
#define GCODE_POLYLINE_JOIN_SEGMENTS 12
#define GCODE_POLYLINE_DEFAULT_WIDTH 0.03f
#define GCODE_POLYLINE_DEFAULT_SCALE 0.05f
#define GCODE_POLYLINE_DEFAULT_BASE_ALPHA 0.15f
#define GCODE_POLYLINE_DEFAULT_HIGHLIGHT_WIDTH 0.02f

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef struct {
    float x, y, z;
} gcode_template_vertex_t;

// Old segment instance (2 points) - kept for caps
typedef struct {
    float ax, ay, az;
    float bx, by, bz;
    float r, g, b, a;
} gcode_segment_instance_t;

// Intermediate segment instance (4 points: pA, pB, pC, pD)
typedef struct {
    float pAx, pAy, pAz;
    float pBx, pBy, pBz;
    float pCx, pCy, pCz;
    float pDx, pDy, pDz;
    float r, g, b, a;
} gcode_intermediate_segment_t;

// Terminal segment instance (3 points: pA, pB, pC)
typedef struct {
    float pAx, pAy, pAz;
    float pBx, pBy, pBz;
    float pCx, pCy, pCz;
    float r, g, b, a;
} gcode_terminal_segment_t;

// Pie-slice join instance (3 points: pA, pB, pC)
typedef struct {
    float pAx, pAy, pAz;  // previous point
    float pBx, pBy, pBz;  // join center
    float pCx, pCy, pCz;  // next point
    float r, g, b, a;
} gcode_pie_join_instance_t;

// Old join instance (1 point) - kept for reference
typedef struct {
    float px, py, pz;
    float r, g, b, a;
} gcode_join_instance_t;

typedef struct {
    mat4_t mvp;
    float line_width;
    float aspect_ratio;
    float _pad[2];
} gcode_polyline_params_t;

typedef struct {
    mat4_t mvp;
    float line_width;
    float aspect_ratio;
    float join_resolution;
    float miter_angle_limit;  // Angle threshold in radians (above this, use semicircle)
} gcode_pie_join_params_t;

typedef struct {
    // GPU resources for intermediate segments (miter-adjusted at both ends)
    sg_pipeline intermediate_pip;
    sg_buffer intermediate_template_vbuf;
    sg_buffer intermediate_template_ibuf;
    sg_buffer intermediate_instance_buf;
    sg_shader intermediate_shd;
    int intermediate_template_vertex_count;
    int intermediate_template_index_count;

    // GPU resources for terminal segments (miter-adjusted at one end)
    sg_pipeline terminal_pip;
    sg_buffer terminal_template_vbuf;
    sg_buffer terminal_template_ibuf;
    sg_buffer terminal_start_instance_buf;  // First segment
    sg_buffer terminal_end_instance_buf;    // Last segment
    sg_shader terminal_shd;
    int terminal_template_vertex_count;
    int terminal_template_index_count;

    // GPU resources for pie-slice joins
    sg_pipeline pie_join_pip;
    sg_buffer pie_join_template_vbuf;
    sg_buffer pie_join_template_ibuf;
    sg_buffer pie_join_instance_buf;
    sg_shader pie_join_shd;
    int pie_join_template_vertex_count;
    int pie_join_template_index_count;

    // GPU resources for caps (semicircles at start and end)
    sg_pipeline cap_pip;
    sg_buffer cap_template_vbuf;
    sg_buffer cap_template_ibuf;
    sg_buffer cap_instance_buf;
    sg_shader cap_shd;
    int cap_template_vertex_count;
    int cap_template_index_count;

    // Path data
    gcode_path_t path;

    // Instance data (dynamically allocated)
    gcode_intermediate_segment_t* intermediate_segments;
    gcode_terminal_segment_t* terminal_start;  // First segment (single)
    gcode_terminal_segment_t* terminal_end;    // Last segment (single)
    gcode_pie_join_instance_t* pie_joins;
    gcode_segment_instance_t* caps;
    int intermediate_count;
    int pie_join_count;
    int cap_count;
    int max_intermediate;
    int max_pie_joins;

    // Parameters
    float line_width;
    float scale;
    float color_r, color_g, color_b;
    float timeline_position;   // 0.0 - 1.0
    float base_alpha;          // Base transparency (0.0 - 1.0)
    float highlight_width;     // Width of highlight fade (0.0 - 0.5)
    float miter_angle_limit;   // Angle threshold in degrees (above this, use semicircle join)
    bool debug_colors;         // Use different colors for each component
} gcode_polyline_t;

//------------------------------------------------------------------------------
// Intermediate segment shader with proper miter joins
// Takes 4 points: pA (before segment), pB (segment start), pC (segment end), pD (after segment)
// Implements miter-adjusted vertices at both ends to prevent overlaps
//------------------------------------------------------------------------------
#if defined(SOKOL_METAL)
static const char* gcode_intermediate_segment_vs_source =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct vs_in {\n"
    "    float3 template_pos [[attribute(0)]];\n"
    "    float3 pA [[attribute(1)]];\n"  // point before segment start
    "    float3 pB [[attribute(2)]];\n"  // segment start
    "    float3 pC [[attribute(3)]];\n"  // segment end
    "    float3 pD [[attribute(4)]];\n"  // point after segment end
    "    float4 color [[attribute(5)]];\n"
    "};\n"
    "struct vs_out {\n"
    "    float4 pos [[position]];\n"
    "    float4 color;\n"
    "};\n"
    "struct vs_params {\n"
    "    float4x4 mvp;\n"
    "    float line_width;\n"
    "    float aspect_ratio;\n"
    "};\n"
    "vertex vs_out vs_main(vs_in in [[stage_in]], constant vs_params& params [[buffer(0)]]) {\n"
    "    vs_out out;\n"
    "    \n"
    "    // Transform all 4 points to clip space\n"
    "    float4 clipA = params.mvp * float4(in.pA, 1.0);\n"
    "    float4 clipB = params.mvp * float4(in.pB, 1.0);\n"
    "    float4 clipC = params.mvp * float4(in.pC, 1.0);\n"
    "    float4 clipD = params.mvp * float4(in.pD, 1.0);\n"
    "    \n"
    "    // Convert to NDC (screen space)\n"
    "    float2 ndcA = clipA.xy / clipA.w;\n"
    "    float2 ndcB = clipB.xy / clipB.w;\n"
    "    float2 ndcC = clipC.xy / clipC.w;\n"
    "    float2 ndcD = clipD.xy / clipD.w;\n"
    "    \n"
    "    // Apply aspect ratio correction to get proper screen coordinates\n"
    "    ndcA.x *= params.aspect_ratio;\n"
    "    ndcB.x *= params.aspect_ratio;\n"
    "    ndcC.x *= params.aspect_ratio;\n"
    "    ndcD.x *= params.aspect_ratio;\n"
    "    \n"
    "    // Determine which end we're at based on template Z\n"
    "    // z=0: at pB (start), z=1: at pC (end)\n"
    "    float2 p0, p1, p2;\n"
    "    float2 pos = float2(in.template_pos.z, in.template_pos.y);  // z=0/1 for ends, y=+-0.5 for sides\n"
    "    float4 baseClip;\n"
    "    \n"
    "    if (in.template_pos.z < 0.5) {\n"
    "        // At B end (start of segment): compute miter using A-B-C\n"
    "        p0 = ndcA; p1 = ndcB; p2 = ndcC;\n"
    "        baseClip = clipB;\n"
    "    } else {\n"
    "        // At C end (end of segment): compute miter using D-C-B (reversed)\n"
    "        p0 = ndcD; p1 = ndcC; p2 = ndcB;\n"
    "        pos = float2(1.0 - in.template_pos.z, -in.template_pos.y);  // flip template\n"
    "        baseClip = clipC;\n"
    "    }\n"
    "    \n"
    "    // Compute direction vectors\n"
    "    float2 d01 = p1 - p0;  // incoming direction (to p1)\n"
    "    float2 d21 = p1 - p2;  // reversed outgoing (to p1)\n"
    "    float2 d12 = p2 - p1;  // outgoing direction (from p1)\n"
    "    \n"
    "    float len01 = length(d01);\n"
    "    float len12 = length(d12);\n"
    "    \n"
    "    // Safe normalized directions (fallback to arbitrary direction if degenerate)\n"
    "    float2 dir01 = (len01 > 0.0001) ? d01 / len01 : float2(1.0, 0.0);\n"
    "    float2 dir12 = (len12 > 0.0001) ? d12 / len12 : float2(1.0, 0.0);\n"
    "    \n"
    "    // Tangent: average of incoming and outgoing directions\n"
    "    float2 tangentSum = dir01 + dir12;\n"
    "    float tangentLen = length(tangentSum);\n"
    "    float2 tangent = (tangentLen > 0.0001) ? tangentSum / tangentLen : float2(-dir01.y, dir01.x);\n"
    "    \n"
    "    // Normal: perpendicular to tangent\n"
    "    float2 normal = float2(-tangent.y, tangent.x);\n"
    "    \n"
    "    // Perpendicular to incoming edge (for miter calculation)\n"
    "    float2 p01Norm = float2(-dir01.y, dir01.x);\n"
    "    \n"
    "    // Sigma: which side the bend is on\n"
    "    float sigma = sign(dot(d01 + d21, normal));\n"
    "    if (abs(sigma) < 0.001) sigma = 1.0;  // Default if straight\n"
    "    \n"
    "    // Dot product for miter calculation (with safety clamp)\n"
    "    float dotNP = dot(normal, p01Norm);\n"
    "    float minDot = 0.1;  // Prevent extreme miter for acute angles\n"
    "    if (abs(dotNP) < minDot) {\n"
    "        dotNP = (dotNP >= 0.0) ? minDot : -minDot;\n"
    "    }\n"
    "    \n"
    "    float2 offset;\n"
    "    float halfWidth = params.line_width * 0.5;\n"
    "    \n"
    "    if (sign(pos.y) == -sigma) {\n"
    "        // Intersecting vertex - use miter point\n"
    "        // This is the inside of the bend where segments would overlap\n"
    "        float2 miterOffset = normal * (-sigma) * halfWidth / dotNP;\n"
    "        offset = miterOffset;\n"
    "    } else {\n"
    "        // Non-intersecting vertex - standard rectangle corner\n"
    "        // This is the outside of the bend\n"
    "        float2 yBasis = float2(-dir12.y, dir12.x);  // perpendicular to segment direction\n"
    "        offset = yBasis * params.line_width * pos.y;  // pos.y is +-0.5, gives +-halfWidth\n"
    "    }\n"
    "    \n"
    "    // Undo aspect ratio correction for final position\n"
    "    offset.x /= params.aspect_ratio;\n"
    "    \n"
    "    out.pos = baseClip;\n"
    "    out.pos.xy += offset * baseClip.w;\n"
    "    out.color = in.color;\n"
    "    return out;\n"
    "}\n";

static const char* gcode_intermediate_segment_fs_source =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct fs_in {\n"
    "    float4 color;\n"
    "};\n"
    "fragment float4 fs_main(fs_in in [[stage_in]]) {\n"
    "    return in.color;\n"
    "}\n";

//------------------------------------------------------------------------------
// Terminal segment shader - miter at one end only
// Takes 3 points: pA (cap end), pB (miter end), pC (neighbor for miter calc)
// z=0 vertices: flat end at pA (cap side)
// z=1 vertices: miter-adjusted at pB (join side)
//------------------------------------------------------------------------------
static const char* gcode_terminal_segment_vs_source =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct vs_in {\n"
    "    float3 template_pos [[attribute(0)]];\n"
    "    float3 pA [[attribute(1)]];\n"  // cap end (flat)
    "    float3 pB [[attribute(2)]];\n"  // miter end
    "    float3 pC [[attribute(3)]];\n"  // neighbor for miter
    "    float4 color [[attribute(4)]];\n"
    "};\n"
    "struct vs_out {\n"
    "    float4 pos [[position]];\n"
    "    float4 color;\n"
    "};\n"
    "struct vs_params {\n"
    "    float4x4 mvp;\n"
    "    float line_width;\n"
    "    float aspect_ratio;\n"
    "};\n"
    "vertex vs_out vs_main(vs_in in [[stage_in]], constant vs_params& params [[buffer(0)]]) {\n"
    "    vs_out out;\n"
    "    \n"
    "    // Transform all points to clip space\n"
    "    float4 clipA = params.mvp * float4(in.pA, 1.0);\n"
    "    float4 clipB = params.mvp * float4(in.pB, 1.0);\n"
    "    float4 clipC = params.mvp * float4(in.pC, 1.0);\n"
    "    \n"
    "    // Convert to NDC and apply aspect ratio\n"
    "    float2 ndcA = clipA.xy / clipA.w;\n"
    "    float2 ndcB = clipB.xy / clipB.w;\n"
    "    float2 ndcC = clipC.xy / clipC.w;\n"
    "    ndcA.x *= params.aspect_ratio;\n"
    "    ndcB.x *= params.aspect_ratio;\n"
    "    ndcC.x *= params.aspect_ratio;\n"
    "    \n"
    "    // Direction from A to B (segment direction)\n"
    "    float2 dAB = ndcB - ndcA;\n"
    "    float lenAB = length(dAB);\n"
    "    float2 dirAB = (lenAB > 0.0001) ? dAB / lenAB : float2(1.0, 0.0);\n"
    "    \n"
    "    // Perpendicular to segment\n"
    "    float2 perp = float2(-dirAB.y, dirAB.x);\n"
    "    \n"
    "    float halfWidth = params.line_width * 0.5;\n"
    "    float2 offset;\n"
    "    float4 baseClip;\n"
    "    \n"
    "    if (in.template_pos.z < 0.5) {\n"
    "        // At A end (cap side) - simple perpendicular offset\n"
    "        // Reference: point = pA + yBasis * width * position.y\n"
    "        baseClip = clipA;\n"
    "        offset = perp * params.line_width * in.template_pos.y;  // y is +-0.5, gives +-halfWidth\n"
    "    } else {\n"
    "        // At B end (miter side) - compute miter using A-B-C\n"
    "        baseClip = clipB;\n"
    "        \n"
    "        // Direction from B to C (outgoing direction)\n"
    "        float2 dBC = ndcC - ndcB;\n"
    "        float lenBC = length(dBC);\n"
    "        float2 dirBC = (lenBC > 0.0001) ? dBC / lenBC : float2(1.0, 0.0);\n"
    "        \n"
    "        // Tangent: average of incoming and outgoing directions\n"
    "        float2 tangentSum = dirAB + dirBC;\n"
    "        float tangentLen = length(tangentSum);\n"
    "        float2 tangent = (tangentLen > 0.0001) ? tangentSum / tangentLen : float2(-dirAB.y, dirAB.x);\n"
    "        \n"
    "        // Normal perpendicular to tangent\n"
    "        float2 normal = float2(-tangent.y, tangent.x);\n"
    "        \n"
    "        // Perpendicular to incoming segment (AB)\n"
    "        float2 abNorm = float2(-dirAB.y, dirAB.x);\n"
    "        \n"
    "        // Sigma: which side the bend is on\n"
    "        // Using ab = B - A and cb = B - C\n"
    "        float2 ab = dAB;      // A to B\n"
    "        float2 cb = -dBC;     // C to B\n"
    "        float sigma = sign(dot(ab + cb, normal));\n"
    "        if (abs(sigma) < 0.001) sigma = 1.0;\n"
    "        \n"
    "        // Miter dot product with safety clamp\n"
    "        float dotNP = dot(normal, abNorm);\n"
    "        float minDot = 0.1;\n"
    "        if (abs(dotNP) < minDot) {\n"
    "            dotNP = (dotNP >= 0.0) ? minDot : -minDot;\n"
    "        }\n"
    "        \n"
    "        float posY = in.template_pos.y;  // +-0.5\n"
    "        \n"
    "        if (sign(posY) == -sigma) {\n"
    "            // Intersecting vertex - use miter\n"
    "            // Reference: 0.5 * normal * -sigma * width / dot(normal, abNorm)\n"
    "            offset = 0.5 * normal * (-sigma) * params.line_width / dotNP;\n"
    "        } else {\n"
    "            // Non-intersecting - standard rectangle\n"
    "            // Reference: point = pA + xBasis * position.x + yBasis * width * position.y\n"
    "            // At position.x=1: point = pB + yBasis * width * posY\n"
    "            float2 yBasis = float2(-dirAB.y, dirAB.x);\n"
    "            offset = yBasis * params.line_width * posY;  // posY is +-0.5, gives +-halfWidth\n"
    "        }\n"
    "    }\n"
    "    \n"
    "    // Undo aspect ratio correction\n"
    "    offset.x /= params.aspect_ratio;\n"
    "    \n"
    "    out.pos = baseClip;\n"
    "    out.pos.xy += offset * baseClip.w;\n"
    "    out.color = in.color;\n"
    "    return out;\n"
    "}\n";

static const char* gcode_terminal_segment_fs_source =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct fs_in {\n"
    "    float4 color;\n"
    "};\n"
    "fragment float4 fs_main(fs_in in [[stage_in]]) {\n"
    "    return in.color;\n"
    "}\n";

//------------------------------------------------------------------------------
// Pie-slice round join shader - fills the angular gap at bends
// Takes 3 points: pA (prev), pB (join center), pC (next)
// Template vertices have x = id (0=center, 1..n = arc positions)
//
// Based on: https://wwwtyro.net/2021/10/01/instanced-lines-part-2.html
// Reference: https://github.com/wwwtyro/instanced-lines-2/blob/master/src/new-2d/round-join.ts
//------------------------------------------------------------------------------
static const char* gcode_pie_join_vs_source =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct vs_in {\n"
    "    float3 template_pos [[attribute(0)]];\n"
    "    float3 pA [[attribute(1)]];\n"  // previous point
    "    float3 pB [[attribute(2)]];\n"  // join center (the polyline vertex)
    "    float3 pC [[attribute(3)]];\n"  // next point
    "    float4 color [[attribute(4)]];\n"
    "};\n"
    "struct vs_out {\n"
    "    float4 pos [[position]];\n"
    "    float4 color;\n"
    "};\n"
    "struct vs_params {\n"
    "    float4x4 mvp;\n"
    "    float line_width;\n"
    "    float aspect_ratio;\n"
    "    float join_resolution;\n"
    "    float miter_angle_limit;\n"  // Angle in radians above which we use semicircle
    "};\n"
    "\n"
    "vertex vs_out vs_main(vs_in in [[stage_in]], constant vs_params& params [[buffer(0)]]) {\n"
    "    vs_out out;\n"
    "    const float PI = 3.14159265359;\n"
    "    \n"
    "    // Transform points to clip space\n"
    "    float4 clipA = params.mvp * float4(in.pA, 1.0);\n"
    "    float4 clipB = params.mvp * float4(in.pB, 1.0);\n"
    "    float4 clipC = params.mvp * float4(in.pC, 1.0);\n"
    "    \n"
    "    // Convert to NDC and apply aspect ratio for circular geometry\n"
    "    float2 ndcA = clipA.xy / clipA.w;\n"
    "    float2 ndcB = clipB.xy / clipB.w;\n"
    "    float2 ndcC = clipC.xy / clipC.w;\n"
    "    ndcA.x *= params.aspect_ratio;\n"
    "    ndcB.x *= params.aspect_ratio;\n"
    "    ndcC.x *= params.aspect_ratio;\n"
    "    \n"
    "    // === Reference implementation from wwwtyro ===\n"
    "    // Calculate the x- and y- basis vectors\n"
    "    float2 dirBC = ndcC - ndcB;\n"
    "    float2 dirBA = ndcA - ndcB;\n"
    "    float lenBC = length(dirBC);\n"
    "    float lenBA = length(dirBA);\n"
    "    \n"
    "    // Handle degenerate cases\n"
    "    if (lenBC < 0.0001 || lenBA < 0.0001) {\n"
    "        out.pos = clipB;\n"
    "        out.color = in.color;\n"
    "        return out;\n"
    "    }\n"
    "    \n"
    "    float2 normBC = dirBC / lenBC;  // normalized B->C\n"
    "    float2 normBA = dirBA / lenBA;  // normalized B->A (note: opposite of A->B)\n"
    "    \n"
    "    // xBasis: the tangent direction (average of the two segment directions)\n"
    "    // Note: we use -normBA because reference uses (C-B) + (B-A) = (C-B) - (A-B)\n"
    "    float2 xBasisSum = normBC - normBA;  // equivalent to normalize(C-B) + normalize(B-A)\n"
    "    float xBasisLen = length(xBasisSum);\n"
    "    \n"
    "    if (xBasisLen < 0.0001) {\n"
    "        // Nearly 180 degree turn or straight - no visible gap\n"
    "        out.pos = clipB;\n"
    "        out.color = in.color;\n"
    "        return out;\n"
    "    }\n"
    "    \n"
    "    float2 xBasis = xBasisSum / xBasisLen;\n"
    "    float2 yBasis = float2(-xBasis.y, xBasis.x);\n"
    "    \n"
    "    // Calculate the normal vectors for each neighboring segment\n"
    "    float2 ab = ndcB - ndcA;  // A to B direction\n"
    "    float2 cb = ndcB - ndcC;  // C to B direction\n"
    "    float2 abn = normalize(float2(-ab.y, ab.x));   // normal to AB segment\n"
    "    float2 cbn = -normalize(float2(-cb.y, cb.x));  // normal to CB segment (negated per reference)\n"
    "    \n"
    "    // Determine the direction of the bend\n"
    "    float sigma = sign(dot(ab + cb, yBasis));\n"
    "    if (abs(sigma) < 0.001) {\n"
    "        // Straight line - no gap to fill\n"
    "        out.pos = clipB;\n"
    "        out.color = in.color;\n"
    "        return out;\n"
    "    }\n"
    "    \n"
    "    float halfWidth = params.line_width * 0.5;\n"
    "    float id = in.template_pos.x;\n"
    "    float resolution = params.join_resolution;\n"
    "    \n"
    "    float2 offset;\n"
    "    \n"
    "    // Find the angle between the two segment normals (used for both center and arc)\n"
    "    float cosTheta = clamp(dot(abn, cbn), -1.0, 1.0);\n"
    "    float theta = acos(cosTheta);  // Angle between normals = bend angle\n"
    "    \n"
    "    // Check if angle exceeds miter limit (use semicircle for sharp bends)\n"
    "    bool useSemicircle = (theta > params.miter_angle_limit);\n"
    "    \n"
    "    if (id < 0.5) {\n"
    "        // Center vertex (id == 0)\n"
    "        if (useSemicircle) {\n"
    "            // For sharp bends, keep center at pointB (semicircle join)\n"
    "            offset = float2(0.0, 0.0);\n"
    "        } else {\n"
    "            // For normal bends, position at miter intersection point\n"
    "            float dotYBasisAbn = dot(yBasis, abn);\n"
    "            if (abs(dotYBasisAbn) < 0.001) dotYBasisAbn = 0.001;  // prevent division by zero\n"
    "            offset = -halfWidth * yBasis * sigma / dotYBasisAbn;\n"
    "        }\n"
    "    } else {\n"
    "        // Arc vertex: positioned along the circular arc on the outside of the bend\n"
    "        // Calculate the angle for this vertex\n"
    "        // sigma * 0.5 * PI puts us at the starting edge\n"
    "        // Then we interpolate through theta\n"
    "        float vertexAngle = (sigma * 0.5 * PI) + (-0.5 * theta) + (theta * (id - 1.0) / resolution);\n"
    "        \n"
    "        // Convert angle to position using basis vectors\n"
    "        // Arc radius = halfWidth to match segment edges (which are at +-halfWidth from center)\n"
    "        float2 pos = halfWidth * float2(cos(vertexAngle), sin(vertexAngle));\n"
    "        offset = xBasis * pos.x + yBasis * pos.y;\n"
    "    }\n"
    "    \n"
    "    // Undo aspect ratio correction for final position\n"
    "    offset.x /= params.aspect_ratio;\n"
    "    \n"
    "    out.pos = clipB;\n"
    "    out.pos.xy += offset * clipB.w;\n"
    "    out.color = in.color;\n"
    "    return out;\n"
    "}\n";

static const char* gcode_pie_join_fs_source =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct fs_in {\n"
    "    float4 color;\n"
    "};\n"
    "fragment float4 fs_main(fs_in in [[stage_in]]) {\n"
    "    return in.color;\n"
    "}\n";

//------------------------------------------------------------------------------
// OpenGL 3.3 GLSL
//------------------------------------------------------------------------------
#elif defined(SOKOL_GLCORE)

static const char* gcode_intermediate_segment_vs_source =
    "#version 330\n"
    "uniform mat4 mvp;\n"
    "uniform float line_width;\n"
    "uniform float aspect_ratio;\n"
    "layout(location=0) in vec3 template_pos;\n"
    "layout(location=1) in vec3 pA;\n"
    "layout(location=2) in vec3 pB;\n"
    "layout(location=3) in vec3 pC;\n"
    "layout(location=4) in vec3 pD;\n"
    "layout(location=5) in vec4 color;\n"
    "out vec4 v_color;\n"
    "void main() {\n"
    "    vec4 clipA = mvp * vec4(pA, 1.0);\n"
    "    vec4 clipB = mvp * vec4(pB, 1.0);\n"
    "    vec4 clipC = mvp * vec4(pC, 1.0);\n"
    "    vec4 clipD = mvp * vec4(pD, 1.0);\n"
    "    vec2 ndcA = clipA.xy / clipA.w;\n"
    "    vec2 ndcB = clipB.xy / clipB.w;\n"
    "    vec2 ndcC = clipC.xy / clipC.w;\n"
    "    vec2 ndcD = clipD.xy / clipD.w;\n"
    "    ndcA.x *= aspect_ratio;\n"
    "    ndcB.x *= aspect_ratio;\n"
    "    ndcC.x *= aspect_ratio;\n"
    "    ndcD.x *= aspect_ratio;\n"
    "    vec2 p0, p1, p2;\n"
    "    vec2 pos = vec2(template_pos.z, template_pos.y);\n"
    "    vec4 baseClip;\n"
    "    if (template_pos.z < 0.5) {\n"
    "        p0 = ndcA; p1 = ndcB; p2 = ndcC;\n"
    "        baseClip = clipB;\n"
    "    } else {\n"
    "        p0 = ndcD; p1 = ndcC; p2 = ndcB;\n"
    "        pos = vec2(1.0 - template_pos.z, -template_pos.y);\n"
    "        baseClip = clipC;\n"
    "    }\n"
    "    vec2 d01 = p1 - p0;\n"
    "    vec2 d21 = p1 - p2;\n"
    "    vec2 d12 = p2 - p1;\n"
    "    float len01 = length(d01);\n"
    "    float len12 = length(d12);\n"
    "    vec2 dir01 = (len01 > 0.0001) ? d01 / len01 : vec2(1.0, 0.0);\n"
    "    vec2 dir12 = (len12 > 0.0001) ? d12 / len12 : vec2(1.0, 0.0);\n"
    "    vec2 tangentSum = dir01 + dir12;\n"
    "    float tangentLen = length(tangentSum);\n"
    "    vec2 tangent = (tangentLen > 0.0001) ? tangentSum / tangentLen : vec2(-dir01.y, dir01.x);\n"
    "    vec2 normal = vec2(-tangent.y, tangent.x);\n"
    "    vec2 p01Norm = vec2(-dir01.y, dir01.x);\n"
    "    float sigma = sign(dot(d01 + d21, normal));\n"
    "    if (abs(sigma) < 0.001) sigma = 1.0;\n"
    "    float dotNP = dot(normal, p01Norm);\n"
    "    float minDot = 0.1;\n"
    "    if (abs(dotNP) < minDot) {\n"
    "        dotNP = (dotNP >= 0.0) ? minDot : -minDot;\n"
    "    }\n"
    "    vec2 offset;\n"
    "    float halfWidth = line_width * 0.5;\n"
    "    if (sign(pos.y) == -sigma) {\n"
    "        offset = normal * (-sigma) * halfWidth / dotNP;\n"
    "    } else {\n"
    "        vec2 yBasis = vec2(-dir12.y, dir12.x);\n"
    "        offset = yBasis * line_width * pos.y;\n"
    "    }\n"
    "    offset.x /= aspect_ratio;\n"
    "    gl_Position = baseClip;\n"
    "    gl_Position.xy += offset * baseClip.w;\n"
    "    v_color = color;\n"
    "}\n";

static const char* gcode_intermediate_segment_fs_source =
    "#version 330\n"
    "in vec4 v_color;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = v_color;\n"
    "}\n";

static const char* gcode_terminal_segment_vs_source =
    "#version 330\n"
    "uniform mat4 mvp;\n"
    "uniform float line_width;\n"
    "uniform float aspect_ratio;\n"
    "layout(location=0) in vec3 template_pos;\n"
    "layout(location=1) in vec3 pA;\n"
    "layout(location=2) in vec3 pB;\n"
    "layout(location=3) in vec3 pC;\n"
    "layout(location=4) in vec4 color;\n"
    "out vec4 v_color;\n"
    "void main() {\n"
    "    vec4 clipA = mvp * vec4(pA, 1.0);\n"
    "    vec4 clipB = mvp * vec4(pB, 1.0);\n"
    "    vec4 clipC = mvp * vec4(pC, 1.0);\n"
    "    vec2 ndcA = clipA.xy / clipA.w;\n"
    "    vec2 ndcB = clipB.xy / clipB.w;\n"
    "    vec2 ndcC = clipC.xy / clipC.w;\n"
    "    ndcA.x *= aspect_ratio;\n"
    "    ndcB.x *= aspect_ratio;\n"
    "    ndcC.x *= aspect_ratio;\n"
    "    vec2 dAB = ndcB - ndcA;\n"
    "    float lenAB = length(dAB);\n"
    "    vec2 dirAB = (lenAB > 0.0001) ? dAB / lenAB : vec2(1.0, 0.0);\n"
    "    vec2 perp = vec2(-dirAB.y, dirAB.x);\n"
    "    float halfWidth = line_width * 0.5;\n"
    "    vec2 offset;\n"
    "    vec4 baseClip;\n"
    "    if (template_pos.z < 0.5) {\n"
    "        baseClip = clipA;\n"
    "        offset = perp * line_width * template_pos.y;\n"
    "    } else {\n"
    "        baseClip = clipB;\n"
    "        vec2 dBC = ndcC - ndcB;\n"
    "        float lenBC = length(dBC);\n"
    "        vec2 dirBC = (lenBC > 0.0001) ? dBC / lenBC : vec2(1.0, 0.0);\n"
    "        vec2 tangentSum = dirAB + dirBC;\n"
    "        float tangentLen = length(tangentSum);\n"
    "        vec2 tangent = (tangentLen > 0.0001) ? tangentSum / tangentLen : vec2(-dirAB.y, dirAB.x);\n"
    "        vec2 normal = vec2(-tangent.y, tangent.x);\n"
    "        vec2 abNorm = vec2(-dirAB.y, dirAB.x);\n"
    "        vec2 ab = dAB;\n"
    "        vec2 cb = -dBC;\n"
    "        float sigma = sign(dot(ab + cb, normal));\n"
    "        if (abs(sigma) < 0.001) sigma = 1.0;\n"
    "        float dotNP = dot(normal, abNorm);\n"
    "        float minDot = 0.1;\n"
    "        if (abs(dotNP) < minDot) {\n"
    "            dotNP = (dotNP >= 0.0) ? minDot : -minDot;\n"
    "        }\n"
    "        float posY = template_pos.y;\n"
    "        if (sign(posY) == -sigma) {\n"
    "            offset = 0.5 * normal * (-sigma) * line_width / dotNP;\n"
    "        } else {\n"
    "            vec2 yBasis = vec2(-dirAB.y, dirAB.x);\n"
    "            offset = yBasis * line_width * posY;\n"
    "        }\n"
    "    }\n"
    "    offset.x /= aspect_ratio;\n"
    "    gl_Position = baseClip;\n"
    "    gl_Position.xy += offset * baseClip.w;\n"
    "    v_color = color;\n"
    "}\n";

static const char* gcode_terminal_segment_fs_source =
    "#version 330\n"
    "in vec4 v_color;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = v_color;\n"
    "}\n";

static const char* gcode_pie_join_vs_source =
    "#version 330\n"
    "uniform mat4 mvp;\n"
    "uniform float line_width;\n"
    "uniform float aspect_ratio;\n"
    "uniform float join_resolution;\n"
    "uniform float miter_angle_limit;\n"
    "layout(location=0) in vec3 template_pos;\n"
    "layout(location=1) in vec3 pA;\n"
    "layout(location=2) in vec3 pB;\n"
    "layout(location=3) in vec3 pC;\n"
    "layout(location=4) in vec4 color;\n"
    "out vec4 v_color;\n"
    "const float PI = 3.14159265359;\n"
    "void main() {\n"
    "    vec4 clipA = mvp * vec4(pA, 1.0);\n"
    "    vec4 clipB = mvp * vec4(pB, 1.0);\n"
    "    vec4 clipC = mvp * vec4(pC, 1.0);\n"
    "    vec2 ndcA = clipA.xy / clipA.w;\n"
    "    vec2 ndcB = clipB.xy / clipB.w;\n"
    "    vec2 ndcC = clipC.xy / clipC.w;\n"
    "    ndcA.x *= aspect_ratio;\n"
    "    ndcB.x *= aspect_ratio;\n"
    "    ndcC.x *= aspect_ratio;\n"
    "    vec2 dirBC = ndcC - ndcB;\n"
    "    vec2 dirBA = ndcA - ndcB;\n"
    "    float lenBC = length(dirBC);\n"
    "    float lenBA = length(dirBA);\n"
    "    if (lenBC < 0.0001 || lenBA < 0.0001) {\n"
    "        gl_Position = clipB;\n"
    "        v_color = color;\n"
    "        return;\n"
    "    }\n"
    "    vec2 normBC = dirBC / lenBC;\n"
    "    vec2 normBA = dirBA / lenBA;\n"
    "    vec2 xBasisSum = normBC - normBA;\n"
    "    float xBasisLen = length(xBasisSum);\n"
    "    if (xBasisLen < 0.0001) {\n"
    "        gl_Position = clipB;\n"
    "        v_color = color;\n"
    "        return;\n"
    "    }\n"
    "    vec2 xBasis = xBasisSum / xBasisLen;\n"
    "    vec2 yBasis = vec2(-xBasis.y, xBasis.x);\n"
    "    vec2 ab = ndcB - ndcA;\n"
    "    vec2 cb = ndcB - ndcC;\n"
    "    vec2 abn = normalize(vec2(-ab.y, ab.x));\n"
    "    vec2 cbn = -normalize(vec2(-cb.y, cb.x));\n"
    "    float sigma = sign(dot(ab + cb, yBasis));\n"
    "    if (abs(sigma) < 0.001) {\n"
    "        gl_Position = clipB;\n"
    "        v_color = color;\n"
    "        return;\n"
    "    }\n"
    "    float halfWidth = line_width * 0.5;\n"
    "    float id = template_pos.x;\n"
    "    float resolution = join_resolution;\n"
    "    vec2 offset;\n"
    "    float cosTheta = clamp(dot(abn, cbn), -1.0, 1.0);\n"
    "    float theta = acos(cosTheta);\n"
    "    bool useSemicircle = (theta > miter_angle_limit);\n"
    "    if (id < 0.5) {\n"
    "        if (useSemicircle) {\n"
    "            offset = vec2(0.0, 0.0);\n"
    "        } else {\n"
    "            float dotYBasisAbn = dot(yBasis, abn);\n"
    "            if (abs(dotYBasisAbn) < 0.001) dotYBasisAbn = 0.001;\n"
    "            offset = -halfWidth * yBasis * sigma / dotYBasisAbn;\n"
    "        }\n"
    "    } else {\n"
    "        float vertexAngle = (sigma * 0.5 * PI) + (-0.5 * theta) + (theta * (id - 1.0) / resolution);\n"
    "        vec2 pos = halfWidth * vec2(cos(vertexAngle), sin(vertexAngle));\n"
    "        offset = xBasis * pos.x + yBasis * pos.y;\n"
    "    }\n"
    "    offset.x /= aspect_ratio;\n"
    "    gl_Position = clipB;\n"
    "    gl_Position.xy += offset * clipB.w;\n"
    "    v_color = color;\n"
    "}\n";

static const char* gcode_pie_join_fs_source =
    "#version 330\n"
    "in vec4 v_color;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = v_color;\n"
    "}\n";

//------------------------------------------------------------------------------
// WebGPU WGSL
//------------------------------------------------------------------------------
#elif defined(SOKOL_WGPU)

static const char* gcode_intermediate_segment_vs_source =
    "struct vs_params {\n"
    "    mvp: mat4x4<f32>,\n"
    "    line_width: f32,\n"
    "    aspect_ratio: f32,\n"
    "};\n"
    "@group(0) @binding(0) var<uniform> params: vs_params;\n"
    "struct vs_out {\n"
    "    @builtin(position) pos: vec4<f32>,\n"
    "    @location(0) color: vec4<f32>,\n"
    "};\n"
    "@vertex\n"
    "fn vs_main(\n"
    "    @location(0) template_pos: vec3<f32>,\n"
    "    @location(1) pA: vec3<f32>,\n"
    "    @location(2) pB: vec3<f32>,\n"
    "    @location(3) pC: vec3<f32>,\n"
    "    @location(4) pD: vec3<f32>,\n"
    "    @location(5) color: vec4<f32>\n"
    ") -> vs_out {\n"
    "    var out: vs_out;\n"
    "    let clipA = params.mvp * vec4<f32>(pA, 1.0);\n"
    "    let clipB = params.mvp * vec4<f32>(pB, 1.0);\n"
    "    let clipC = params.mvp * vec4<f32>(pC, 1.0);\n"
    "    let clipD = params.mvp * vec4<f32>(pD, 1.0);\n"
    "    var ndcA = clipA.xy / clipA.w;\n"
    "    var ndcB = clipB.xy / clipB.w;\n"
    "    var ndcC = clipC.xy / clipC.w;\n"
    "    var ndcD = clipD.xy / clipD.w;\n"
    "    ndcA.x = ndcA.x * params.aspect_ratio;\n"
    "    ndcB.x = ndcB.x * params.aspect_ratio;\n"
    "    ndcC.x = ndcC.x * params.aspect_ratio;\n"
    "    ndcD.x = ndcD.x * params.aspect_ratio;\n"
    "    var p0: vec2<f32>; var p1: vec2<f32>; var p2: vec2<f32>;\n"
    "    var pos: vec2<f32>;\n"
    "    var baseClip: vec4<f32>;\n"
    "    if (template_pos.z < 0.5) {\n"
    "        p0 = ndcA; p1 = ndcB; p2 = ndcC;\n"
    "        pos = vec2<f32>(template_pos.z, template_pos.y);\n"
    "        baseClip = clipB;\n"
    "    } else {\n"
    "        p0 = ndcD; p1 = ndcC; p2 = ndcB;\n"
    "        pos = vec2<f32>(1.0 - template_pos.z, -template_pos.y);\n"
    "        baseClip = clipC;\n"
    "    }\n"
    "    let d01 = p1 - p0;\n"
    "    let d21 = p1 - p2;\n"
    "    let d12 = p2 - p1;\n"
    "    let len01 = length(d01);\n"
    "    let len12 = length(d12);\n"
    "    var dir01: vec2<f32>; var dir12: vec2<f32>;\n"
    "    if (len01 > 0.0001) { dir01 = d01 / len01; } else { dir01 = vec2<f32>(1.0, 0.0); }\n"
    "    if (len12 > 0.0001) { dir12 = d12 / len12; } else { dir12 = vec2<f32>(1.0, 0.0); }\n"
    "    let tangentSum = dir01 + dir12;\n"
    "    let tangentLen = length(tangentSum);\n"
    "    var tangent: vec2<f32>;\n"
    "    if (tangentLen > 0.0001) { tangent = tangentSum / tangentLen; } else { tangent = vec2<f32>(-dir01.y, dir01.x); }\n"
    "    let normal = vec2<f32>(-tangent.y, tangent.x);\n"
    "    let p01Norm = vec2<f32>(-dir01.y, dir01.x);\n"
    "    var sigma = sign(dot(d01 + d21, normal));\n"
    "    if (abs(sigma) < 0.001) { sigma = 1.0; }\n"
    "    var dotNP = dot(normal, p01Norm);\n"
    "    let minDot = 0.1;\n"
    "    if (abs(dotNP) < minDot) {\n"
    "        if (dotNP >= 0.0) { dotNP = minDot; } else { dotNP = -minDot; }\n"
    "    }\n"
    "    var offset: vec2<f32>;\n"
    "    let halfWidth = params.line_width * 0.5;\n"
    "    if (sign(pos.y) == -sigma) {\n"
    "        offset = normal * (-sigma) * halfWidth / dotNP;\n"
    "    } else {\n"
    "        let yBasis = vec2<f32>(-dir12.y, dir12.x);\n"
    "        offset = yBasis * params.line_width * pos.y;\n"
    "    }\n"
    "    offset.x = offset.x / params.aspect_ratio;\n"
    "    out.pos = baseClip;\n"
    "    out.pos.x = out.pos.x + offset.x * baseClip.w;\n"
    "    out.pos.y = out.pos.y + offset.y * baseClip.w;\n"
    "    out.color = color;\n"
    "    return out;\n"
    "}\n";

static const char* gcode_intermediate_segment_fs_source =
    "@fragment\n"
    "fn fs_main(@location(0) color: vec4<f32>) -> @location(0) vec4<f32> {\n"
    "    return color;\n"
    "}\n";

static const char* gcode_terminal_segment_vs_source =
    "struct vs_params {\n"
    "    mvp: mat4x4<f32>,\n"
    "    line_width: f32,\n"
    "    aspect_ratio: f32,\n"
    "};\n"
    "@group(0) @binding(0) var<uniform> params: vs_params;\n"
    "struct vs_out {\n"
    "    @builtin(position) pos: vec4<f32>,\n"
    "    @location(0) color: vec4<f32>,\n"
    "};\n"
    "@vertex\n"
    "fn vs_main(\n"
    "    @location(0) template_pos: vec3<f32>,\n"
    "    @location(1) pA: vec3<f32>,\n"
    "    @location(2) pB: vec3<f32>,\n"
    "    @location(3) pC: vec3<f32>,\n"
    "    @location(4) color: vec4<f32>\n"
    ") -> vs_out {\n"
    "    var out: vs_out;\n"
    "    let clipA = params.mvp * vec4<f32>(pA, 1.0);\n"
    "    let clipB = params.mvp * vec4<f32>(pB, 1.0);\n"
    "    let clipC = params.mvp * vec4<f32>(pC, 1.0);\n"
    "    var ndcA = clipA.xy / clipA.w;\n"
    "    var ndcB = clipB.xy / clipB.w;\n"
    "    var ndcC = clipC.xy / clipC.w;\n"
    "    ndcA.x = ndcA.x * params.aspect_ratio;\n"
    "    ndcB.x = ndcB.x * params.aspect_ratio;\n"
    "    ndcC.x = ndcC.x * params.aspect_ratio;\n"
    "    let dAB = ndcB - ndcA;\n"
    "    let lenAB = length(dAB);\n"
    "    var dirAB: vec2<f32>;\n"
    "    if (lenAB > 0.0001) { dirAB = dAB / lenAB; } else { dirAB = vec2<f32>(1.0, 0.0); }\n"
    "    let perp = vec2<f32>(-dirAB.y, dirAB.x);\n"
    "    let halfWidth = params.line_width * 0.5;\n"
    "    var offset: vec2<f32>;\n"
    "    var baseClip: vec4<f32>;\n"
    "    if (template_pos.z < 0.5) {\n"
    "        baseClip = clipA;\n"
    "        offset = perp * params.line_width * template_pos.y;\n"
    "    } else {\n"
    "        baseClip = clipB;\n"
    "        let dBC = ndcC - ndcB;\n"
    "        let lenBC = length(dBC);\n"
    "        var dirBC: vec2<f32>;\n"
    "        if (lenBC > 0.0001) { dirBC = dBC / lenBC; } else { dirBC = vec2<f32>(1.0, 0.0); }\n"
    "        let tangentSum = dirAB + dirBC;\n"
    "        let tangentLen = length(tangentSum);\n"
    "        var tangent: vec2<f32>;\n"
    "        if (tangentLen > 0.0001) { tangent = tangentSum / tangentLen; } else { tangent = vec2<f32>(-dirAB.y, dirAB.x); }\n"
    "        let normal = vec2<f32>(-tangent.y, tangent.x);\n"
    "        let abNorm = vec2<f32>(-dirAB.y, dirAB.x);\n"
    "        let ab = dAB;\n"
    "        let cb = -dBC;\n"
    "        var sigma = sign(dot(ab + cb, normal));\n"
    "        if (abs(sigma) < 0.001) { sigma = 1.0; }\n"
    "        var dotNP = dot(normal, abNorm);\n"
    "        let minDot = 0.1;\n"
    "        if (abs(dotNP) < minDot) {\n"
    "            if (dotNP >= 0.0) { dotNP = minDot; } else { dotNP = -minDot; }\n"
    "        }\n"
    "        let posY = template_pos.y;\n"
    "        if (sign(posY) == -sigma) {\n"
    "            offset = 0.5 * normal * (-sigma) * params.line_width / dotNP;\n"
    "        } else {\n"
    "            let yBasis = vec2<f32>(-dirAB.y, dirAB.x);\n"
    "            offset = yBasis * params.line_width * posY;\n"
    "        }\n"
    "    }\n"
    "    offset.x = offset.x / params.aspect_ratio;\n"
    "    out.pos = baseClip;\n"
    "    out.pos.x = out.pos.x + offset.x * baseClip.w;\n"
    "    out.pos.y = out.pos.y + offset.y * baseClip.w;\n"
    "    out.color = color;\n"
    "    return out;\n"
    "}\n";

static const char* gcode_terminal_segment_fs_source =
    "@fragment\n"
    "fn fs_main(@location(0) color: vec4<f32>) -> @location(0) vec4<f32> {\n"
    "    return color;\n"
    "}\n";

static const char* gcode_pie_join_vs_source =
    "struct vs_params {\n"
    "    mvp: mat4x4<f32>,\n"
    "    line_width: f32,\n"
    "    aspect_ratio: f32,\n"
    "    join_resolution: f32,\n"
    "    miter_angle_limit: f32,\n"
    "};\n"
    "@group(0) @binding(0) var<uniform> params: vs_params;\n"
    "struct vs_out {\n"
    "    @builtin(position) pos: vec4<f32>,\n"
    "    @location(0) color: vec4<f32>,\n"
    "};\n"
    "const PI: f32 = 3.14159265359;\n"
    "@vertex\n"
    "fn vs_main(\n"
    "    @location(0) template_pos: vec3<f32>,\n"
    "    @location(1) pA: vec3<f32>,\n"
    "    @location(2) pB: vec3<f32>,\n"
    "    @location(3) pC: vec3<f32>,\n"
    "    @location(4) color: vec4<f32>\n"
    ") -> vs_out {\n"
    "    var out: vs_out;\n"
    "    let clipA = params.mvp * vec4<f32>(pA, 1.0);\n"
    "    let clipB = params.mvp * vec4<f32>(pB, 1.0);\n"
    "    let clipC = params.mvp * vec4<f32>(pC, 1.0);\n"
    "    var ndcA = clipA.xy / clipA.w;\n"
    "    var ndcB = clipB.xy / clipB.w;\n"
    "    var ndcC = clipC.xy / clipC.w;\n"
    "    ndcA.x = ndcA.x * params.aspect_ratio;\n"
    "    ndcB.x = ndcB.x * params.aspect_ratio;\n"
    "    ndcC.x = ndcC.x * params.aspect_ratio;\n"
    "    let dirBC = ndcC - ndcB;\n"
    "    let dirBA = ndcA - ndcB;\n"
    "    let lenBC = length(dirBC);\n"
    "    let lenBA = length(dirBA);\n"
    "    if (lenBC < 0.0001 || lenBA < 0.0001) {\n"
    "        out.pos = clipB;\n"
    "        out.color = color;\n"
    "        return out;\n"
    "    }\n"
    "    let normBC = dirBC / lenBC;\n"
    "    let normBA = dirBA / lenBA;\n"
    "    let xBasisSum = normBC - normBA;\n"
    "    let xBasisLen = length(xBasisSum);\n"
    "    if (xBasisLen < 0.0001) {\n"
    "        out.pos = clipB;\n"
    "        out.color = color;\n"
    "        return out;\n"
    "    }\n"
    "    let xBasis = xBasisSum / xBasisLen;\n"
    "    let yBasis = vec2<f32>(-xBasis.y, xBasis.x);\n"
    "    let ab = ndcB - ndcA;\n"
    "    let cb = ndcB - ndcC;\n"
    "    let abn = normalize(vec2<f32>(-ab.y, ab.x));\n"
    "    let cbn = -normalize(vec2<f32>(-cb.y, cb.x));\n"
    "    var sigma = sign(dot(ab + cb, yBasis));\n"
    "    if (abs(sigma) < 0.001) {\n"
    "        out.pos = clipB;\n"
    "        out.color = color;\n"
    "        return out;\n"
    "    }\n"
    "    let halfWidth = params.line_width * 0.5;\n"
    "    let id = template_pos.x;\n"
    "    let resolution = params.join_resolution;\n"
    "    var offset: vec2<f32>;\n"
    "    let cosTheta = clamp(dot(abn, cbn), -1.0, 1.0);\n"
    "    let theta = acos(cosTheta);\n"
    "    let useSemicircle = (theta > params.miter_angle_limit);\n"
    "    if (id < 0.5) {\n"
    "        if (useSemicircle) {\n"
    "            offset = vec2<f32>(0.0, 0.0);\n"
    "        } else {\n"
    "            var dotYBasisAbn = dot(yBasis, abn);\n"
    "            if (abs(dotYBasisAbn) < 0.001) { dotYBasisAbn = 0.001; }\n"
    "            offset = -halfWidth * yBasis * sigma / dotYBasisAbn;\n"
    "        }\n"
    "    } else {\n"
    "        let vertexAngle = (sigma * 0.5 * PI) + (-0.5 * theta) + (theta * (id - 1.0) / resolution);\n"
    "        let pos = halfWidth * vec2<f32>(cos(vertexAngle), sin(vertexAngle));\n"
    "        offset = xBasis * pos.x + yBasis * pos.y;\n"
    "    }\n"
    "    offset.x = offset.x / params.aspect_ratio;\n"
    "    out.pos = clipB;\n"
    "    out.pos.x = out.pos.x + offset.x * clipB.w;\n"
    "    out.pos.y = out.pos.y + offset.y * clipB.w;\n"
    "    out.color = color;\n"
    "    return out;\n"
    "}\n";

static const char* gcode_pie_join_fs_source =
    "@fragment\n"
    "fn fs_main(@location(0) color: vec4<f32>) -> @location(0) vec4<f32> {\n"
    "    return color;\n"
    "}\n";

//------------------------------------------------------------------------------
// DirectX 11 HLSL
//------------------------------------------------------------------------------
#elif defined(SOKOL_D3D11)

static const char* gcode_intermediate_segment_vs_source =
    "cbuffer vs_params : register(b0) {\n"
    "    float4x4 mvp;\n"
    "    float line_width;\n"
    "    float aspect_ratio;\n"
    "};\n"
    "struct vs_in {\n"
    "    float3 template_pos : POSITION;\n"
    "    float3 pA : TEXCOORD0;\n"
    "    float3 pB : TEXCOORD1;\n"
    "    float3 pC : TEXCOORD2;\n"
    "    float3 pD : TEXCOORD3;\n"
    "    float4 color : COLOR;\n"
    "};\n"
    "struct vs_out {\n"
    "    float4 color : COLOR;\n"
    "    float4 pos : SV_Position;\n"
    "};\n"
    "vs_out main(vs_in inp) {\n"
    "    vs_out outp;\n"
    "    float4 clipA = mul(mvp, float4(inp.pA, 1.0));\n"
    "    float4 clipB = mul(mvp, float4(inp.pB, 1.0));\n"
    "    float4 clipC = mul(mvp, float4(inp.pC, 1.0));\n"
    "    float4 clipD = mul(mvp, float4(inp.pD, 1.0));\n"
    "    float2 ndcA = clipA.xy / clipA.w;\n"
    "    float2 ndcB = clipB.xy / clipB.w;\n"
    "    float2 ndcC = clipC.xy / clipC.w;\n"
    "    float2 ndcD = clipD.xy / clipD.w;\n"
    "    ndcA.x *= aspect_ratio;\n"
    "    ndcB.x *= aspect_ratio;\n"
    "    ndcC.x *= aspect_ratio;\n"
    "    ndcD.x *= aspect_ratio;\n"
    "    float2 p0, p1, p2;\n"
    "    float2 pos;\n"
    "    float4 baseClip;\n"
    "    if (inp.template_pos.z < 0.5) {\n"
    "        p0 = ndcA; p1 = ndcB; p2 = ndcC;\n"
    "        pos = float2(inp.template_pos.z, inp.template_pos.y);\n"
    "        baseClip = clipB;\n"
    "    } else {\n"
    "        p0 = ndcD; p1 = ndcC; p2 = ndcB;\n"
    "        pos = float2(1.0 - inp.template_pos.z, -inp.template_pos.y);\n"
    "        baseClip = clipC;\n"
    "    }\n"
    "    float2 d01 = p1 - p0;\n"
    "    float2 d21 = p1 - p2;\n"
    "    float2 d12 = p2 - p1;\n"
    "    float len01 = length(d01);\n"
    "    float len12 = length(d12);\n"
    "    float2 dir01 = (len01 > 0.0001) ? d01 / len01 : float2(1.0, 0.0);\n"
    "    float2 dir12 = (len12 > 0.0001) ? d12 / len12 : float2(1.0, 0.0);\n"
    "    float2 tangentSum = dir01 + dir12;\n"
    "    float tangentLen = length(tangentSum);\n"
    "    float2 tangent = (tangentLen > 0.0001) ? tangentSum / tangentLen : float2(-dir01.y, dir01.x);\n"
    "    float2 normal = float2(-tangent.y, tangent.x);\n"
    "    float2 p01Norm = float2(-dir01.y, dir01.x);\n"
    "    float sigma = sign(dot(d01 + d21, normal));\n"
    "    if (abs(sigma) < 0.001) sigma = 1.0;\n"
    "    float dotNP = dot(normal, p01Norm);\n"
    "    float minDot = 0.1;\n"
    "    if (abs(dotNP) < minDot) {\n"
    "        dotNP = (dotNP >= 0.0) ? minDot : -minDot;\n"
    "    }\n"
    "    float2 offset;\n"
    "    float halfWidth = line_width * 0.5;\n"
    "    if (sign(pos.y) == -sigma) {\n"
    "        offset = normal * (-sigma) * halfWidth / dotNP;\n"
    "    } else {\n"
    "        float2 yBasis = float2(-dir12.y, dir12.x);\n"
    "        offset = yBasis * line_width * pos.y;\n"
    "    }\n"
    "    offset.x /= aspect_ratio;\n"
    "    outp.pos = baseClip;\n"
    "    outp.pos.xy += offset * baseClip.w;\n"
    "    outp.color = inp.color;\n"
    "    return outp;\n"
    "}\n";

static const char* gcode_intermediate_segment_fs_source =
    "struct fs_in {\n"
    "    float4 color : COLOR;\n"
    "};\n"
    "float4 main(fs_in inp) : SV_Target0 {\n"
    "    return inp.color;\n"
    "}\n";

static const char* gcode_terminal_segment_vs_source =
    "cbuffer vs_params : register(b0) {\n"
    "    float4x4 mvp;\n"
    "    float line_width;\n"
    "    float aspect_ratio;\n"
    "};\n"
    "struct vs_in {\n"
    "    float3 template_pos : POSITION;\n"
    "    float3 pA : TEXCOORD0;\n"
    "    float3 pB : TEXCOORD1;\n"
    "    float3 pC : TEXCOORD2;\n"
    "    float4 color : COLOR;\n"
    "};\n"
    "struct vs_out {\n"
    "    float4 color : COLOR;\n"
    "    float4 pos : SV_Position;\n"
    "};\n"
    "vs_out main(vs_in inp) {\n"
    "    vs_out outp;\n"
    "    float4 clipA = mul(mvp, float4(inp.pA, 1.0));\n"
    "    float4 clipB = mul(mvp, float4(inp.pB, 1.0));\n"
    "    float4 clipC = mul(mvp, float4(inp.pC, 1.0));\n"
    "    float2 ndcA = clipA.xy / clipA.w;\n"
    "    float2 ndcB = clipB.xy / clipB.w;\n"
    "    float2 ndcC = clipC.xy / clipC.w;\n"
    "    ndcA.x *= aspect_ratio;\n"
    "    ndcB.x *= aspect_ratio;\n"
    "    ndcC.x *= aspect_ratio;\n"
    "    float2 dAB = ndcB - ndcA;\n"
    "    float lenAB = length(dAB);\n"
    "    float2 dirAB = (lenAB > 0.0001) ? dAB / lenAB : float2(1.0, 0.0);\n"
    "    float2 perp = float2(-dirAB.y, dirAB.x);\n"
    "    float halfWidth = line_width * 0.5;\n"
    "    float2 offset;\n"
    "    float4 baseClip;\n"
    "    if (inp.template_pos.z < 0.5) {\n"
    "        baseClip = clipA;\n"
    "        offset = perp * line_width * inp.template_pos.y;\n"
    "    } else {\n"
    "        baseClip = clipB;\n"
    "        float2 dBC = ndcC - ndcB;\n"
    "        float lenBC = length(dBC);\n"
    "        float2 dirBC = (lenBC > 0.0001) ? dBC / lenBC : float2(1.0, 0.0);\n"
    "        float2 tangentSum = dirAB + dirBC;\n"
    "        float tangentLen = length(tangentSum);\n"
    "        float2 tangent = (tangentLen > 0.0001) ? tangentSum / tangentLen : float2(-dirAB.y, dirAB.x);\n"
    "        float2 normal = float2(-tangent.y, tangent.x);\n"
    "        float2 abNorm = float2(-dirAB.y, dirAB.x);\n"
    "        float2 ab = dAB;\n"
    "        float2 cb = -dBC;\n"
    "        float sigma = sign(dot(ab + cb, normal));\n"
    "        if (abs(sigma) < 0.001) sigma = 1.0;\n"
    "        float dotNP = dot(normal, abNorm);\n"
    "        float minDot = 0.1;\n"
    "        if (abs(dotNP) < minDot) {\n"
    "            dotNP = (dotNP >= 0.0) ? minDot : -minDot;\n"
    "        }\n"
    "        float posY = inp.template_pos.y;\n"
    "        if (sign(posY) == -sigma) {\n"
    "            offset = 0.5 * normal * (-sigma) * line_width / dotNP;\n"
    "        } else {\n"
    "            float2 yBasis = float2(-dirAB.y, dirAB.x);\n"
    "            offset = yBasis * line_width * posY;\n"
    "        }\n"
    "    }\n"
    "    offset.x /= aspect_ratio;\n"
    "    outp.pos = baseClip;\n"
    "    outp.pos.xy += offset * baseClip.w;\n"
    "    outp.color = inp.color;\n"
    "    return outp;\n"
    "}\n";

static const char* gcode_terminal_segment_fs_source =
    "struct fs_in {\n"
    "    float4 color : COLOR;\n"
    "};\n"
    "float4 main(fs_in inp) : SV_Target0 {\n"
    "    return inp.color;\n"
    "}\n";

static const char* gcode_pie_join_vs_source =
    "cbuffer vs_params : register(b0) {\n"
    "    float4x4 mvp;\n"
    "    float line_width;\n"
    "    float aspect_ratio;\n"
    "    float join_resolution;\n"
    "    float miter_angle_limit;\n"
    "};\n"
    "struct vs_in {\n"
    "    float3 template_pos : POSITION;\n"
    "    float3 pA : TEXCOORD0;\n"
    "    float3 pB : TEXCOORD1;\n"
    "    float3 pC : TEXCOORD2;\n"
    "    float4 color : COLOR;\n"
    "};\n"
    "struct vs_out {\n"
    "    float4 color : COLOR;\n"
    "    float4 pos : SV_Position;\n"
    "};\n"
    "static const float PI = 3.14159265359;\n"
    "vs_out main(vs_in inp) {\n"
    "    vs_out outp;\n"
    "    float4 clipA = mul(mvp, float4(inp.pA, 1.0));\n"
    "    float4 clipB = mul(mvp, float4(inp.pB, 1.0));\n"
    "    float4 clipC = mul(mvp, float4(inp.pC, 1.0));\n"
    "    float2 ndcA = clipA.xy / clipA.w;\n"
    "    float2 ndcB = clipB.xy / clipB.w;\n"
    "    float2 ndcC = clipC.xy / clipC.w;\n"
    "    ndcA.x *= aspect_ratio;\n"
    "    ndcB.x *= aspect_ratio;\n"
    "    ndcC.x *= aspect_ratio;\n"
    "    float2 dirBC = ndcC - ndcB;\n"
    "    float2 dirBA = ndcA - ndcB;\n"
    "    float lenBC = length(dirBC);\n"
    "    float lenBA = length(dirBA);\n"
    "    if (lenBC < 0.0001 || lenBA < 0.0001) {\n"
    "        outp.pos = clipB;\n"
    "        outp.color = inp.color;\n"
    "        return outp;\n"
    "    }\n"
    "    float2 normBC = dirBC / lenBC;\n"
    "    float2 normBA = dirBA / lenBA;\n"
    "    float2 xBasisSum = normBC - normBA;\n"
    "    float xBasisLen = length(xBasisSum);\n"
    "    if (xBasisLen < 0.0001) {\n"
    "        outp.pos = clipB;\n"
    "        outp.color = inp.color;\n"
    "        return outp;\n"
    "    }\n"
    "    float2 xBasis = xBasisSum / xBasisLen;\n"
    "    float2 yBasis = float2(-xBasis.y, xBasis.x);\n"
    "    float2 ab = ndcB - ndcA;\n"
    "    float2 cb = ndcB - ndcC;\n"
    "    float2 abn = normalize(float2(-ab.y, ab.x));\n"
    "    float2 cbn = -normalize(float2(-cb.y, cb.x));\n"
    "    float sigma = sign(dot(ab + cb, yBasis));\n"
    "    if (abs(sigma) < 0.001) {\n"
    "        outp.pos = clipB;\n"
    "        outp.color = inp.color;\n"
    "        return outp;\n"
    "    }\n"
    "    float halfWidth = line_width * 0.5;\n"
    "    float id = inp.template_pos.x;\n"
    "    float resolution = join_resolution;\n"
    "    float2 offset;\n"
    "    float cosTheta = clamp(dot(abn, cbn), -1.0, 1.0);\n"
    "    float theta = acos(cosTheta);\n"
    "    bool useSemicircle = (theta > miter_angle_limit);\n"
    "    if (id < 0.5) {\n"
    "        if (useSemicircle) {\n"
    "            offset = float2(0.0, 0.0);\n"
    "        } else {\n"
    "            float dotYBasisAbn = dot(yBasis, abn);\n"
    "            if (abs(dotYBasisAbn) < 0.001) dotYBasisAbn = 0.001;\n"
    "            offset = -halfWidth * yBasis * sigma / dotYBasisAbn;\n"
    "        }\n"
    "    } else {\n"
    "        float vertexAngle = (sigma * 0.5 * PI) + (-0.5 * theta) + (theta * (id - 1.0) / resolution);\n"
    "        float2 pos = halfWidth * float2(cos(vertexAngle), sin(vertexAngle));\n"
    "        offset = xBasis * pos.x + yBasis * pos.y;\n"
    "    }\n"
    "    offset.x /= aspect_ratio;\n"
    "    outp.pos = clipB;\n"
    "    outp.pos.xy += offset * clipB.w;\n"
    "    outp.color = inp.color;\n"
    "    return outp;\n"
    "}\n";

static const char* gcode_pie_join_fs_source =
    "struct fs_in {\n"
    "    float4 color : COLOR;\n"
    "};\n"
    "float4 main(fs_in inp) : SV_Target0 {\n"
    "    return inp.color;\n"
    "}\n";

#else
#error "Unknown graphics backend - define SOKOL_GLCORE, SOKOL_METAL, SOKOL_WGPU, or SOKOL_D3D11"
#endif

//------------------------------------------------------------------------------
// Legacy join shader (for full circle joins - keeping for reference)
//------------------------------------------------------------------------------
#if defined(SOKOL_GLCORE)
static const char* gcode_join_vs_source =
    "#version 330\n"
    "uniform mat4 mvp;\n"
    "uniform float line_width;\n"
    "uniform float aspect_ratio;\n"
    "layout(location=0) in vec3 template_pos;\n"
    "layout(location=1) in vec3 point;\n"
    "layout(location=2) in vec4 color;\n"
    "out vec4 v_color;\n"
    "void main() {\n"
    "    vec4 clip_p = mvp * vec4(point, 1.0);\n"
    "    vec2 offset = vec2(template_pos.x / aspect_ratio, template_pos.y) * line_width;\n"
    "    gl_Position = clip_p;\n"
    "    gl_Position.xy += offset * clip_p.w;\n"
    "    v_color = color;\n"
    "}\n";
static const char* gcode_join_fs_source =
    "#version 330\n"
    "in vec4 v_color;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = v_color;\n"
    "}\n";
#elif defined(SOKOL_METAL)
static const char* gcode_join_vs_source =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct vs_in {\n"
    "    float3 template_pos [[attribute(0)]];\n"
    "    float3 point [[attribute(1)]];\n"
    "    float4 color [[attribute(2)]];\n"
    "};\n"
    "struct vs_out {\n"
    "    float4 pos [[position]];\n"
    "    float4 color;\n"
    "};\n"
    "struct vs_params {\n"
    "    float4x4 mvp;\n"
    "    float line_width;\n"
    "    float aspect_ratio;\n"
    "};\n"
    "vertex vs_out vs_main(vs_in in [[stage_in]], constant vs_params& params [[buffer(0)]]) {\n"
    "    vs_out out;\n"
    "    float4 clip_p = params.mvp * float4(in.point, 1.0);\n"
    "    float2 offset = float2(in.template_pos.x / params.aspect_ratio, in.template_pos.y) * params.line_width;\n"
    "    out.pos = clip_p;\n"
    "    out.pos.xy += offset * clip_p.w;\n"
    "    out.color = in.color;\n"
    "    return out;\n"
    "};\n";
static const char* gcode_join_fs_source =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct fs_in {\n"
    "    float4 color;\n"
    "};\n"
    "fragment float4 fs_main(fs_in in [[stage_in]]) {\n"
    "    return in.color;\n"
    "}\n";
#elif defined(SOKOL_WGPU)
static const char* gcode_join_vs_source =
    "struct vs_params {\n"
    "    mvp: mat4x4<f32>,\n"
    "    line_width: f32,\n"
    "    aspect_ratio: f32,\n"
    "};\n"
    "@group(0) @binding(0) var<uniform> params: vs_params;\n"
    "struct vs_out {\n"
    "    @builtin(position) pos: vec4<f32>,\n"
    "    @location(0) color: vec4<f32>,\n"
    "};\n"
    "@vertex\n"
    "fn vs_main(\n"
    "    @location(0) template_pos: vec3<f32>,\n"
    "    @location(1) point: vec3<f32>,\n"
    "    @location(2) color: vec4<f32>\n"
    ") -> vs_out {\n"
    "    var out: vs_out;\n"
    "    let clip_p = params.mvp * vec4<f32>(point, 1.0);\n"
    "    let offset = vec2<f32>(template_pos.x / params.aspect_ratio, template_pos.y) * params.line_width;\n"
    "    out.pos = clip_p;\n"
    "    out.pos.x = out.pos.x + offset.x * clip_p.w;\n"
    "    out.pos.y = out.pos.y + offset.y * clip_p.w;\n"
    "    out.color = color;\n"
    "    return out;\n"
    "}\n";
static const char* gcode_join_fs_source =
    "@fragment\n"
    "fn fs_main(@location(0) color: vec4<f32>) -> @location(0) vec4<f32> {\n"
    "    return color;\n"
    "}\n";
#elif defined(SOKOL_D3D11)
static const char* gcode_join_vs_source =
    "cbuffer vs_params : register(b0) {\n"
    "    float4x4 mvp;\n"
    "    float line_width;\n"
    "    float aspect_ratio;\n"
    "};\n"
    "struct vs_in {\n"
    "    float3 template_pos : POSITION;\n"
    "    float3 point : TEXCOORD0;\n"
    "    float4 color : COLOR;\n"
    "};\n"
    "struct vs_out {\n"
    "    float4 color : COLOR;\n"
    "    float4 pos : SV_Position;\n"
    "};\n"
    "vs_out main(vs_in inp) {\n"
    "    vs_out outp;\n"
    "    float4 clip_p = mul(mvp, float4(inp.point, 1.0));\n"
    "    float2 offset = float2(inp.template_pos.x / aspect_ratio, inp.template_pos.y) * line_width;\n"
    "    outp.pos = clip_p;\n"
    "    outp.pos.xy += offset * clip_p.w;\n"
    "    outp.color = inp.color;\n"
    "    return outp;\n"
    "}\n";
static const char* gcode_join_fs_source =
    "struct fs_in {\n"
    "    float4 color : COLOR;\n"
    "};\n"
    "float4 main(fs_in inp) : SV_Target0 {\n"
    "    return inp.color;\n"
    "}\n";
#endif

//------------------------------------------------------------------------------
// Template geometry generation
//------------------------------------------------------------------------------

// Segment template: rectangle body ONLY (no caps) to prevent overlap at joins
static inline void gcode_generate_segment_template(
    gcode_template_vertex_t* vertices, int* vertex_count,
    uint16_t* indices, int* index_count
) {
    int vi = 0;
    int ii = 0;

    // Main rectangle body only - no caps!
    // This prevents alpha overlap at joins where segments meet
    vertices[vi++] = (gcode_template_vertex_t){ 0.0f, -0.5f, 0.0f };
    vertices[vi++] = (gcode_template_vertex_t){ 0.0f,  0.5f, 0.0f };
    vertices[vi++] = (gcode_template_vertex_t){ 0.0f, -0.5f, 1.0f };
    vertices[vi++] = (gcode_template_vertex_t){ 0.0f,  0.5f, 1.0f };

    indices[ii++] = 0; indices[ii++] = 2; indices[ii++] = 1;
    indices[ii++] = 1; indices[ii++] = 2; indices[ii++] = 3;

    *vertex_count = vi;
    *index_count = ii;
}

// Cap template: semicircle at z=0 (point A side) for terminal vertices
// Used to cap the start and end of the polyline
static inline void gcode_generate_cap_template(
    gcode_template_vertex_t* vertices, int* vertex_count,
    uint16_t* indices, int* index_count,
    int cap_segments
) {
    int vi = 0;
    int ii = 0;

    // Semicircle at z=0 (extends in negative direction from point A)
    // Center vertex
    vertices[vi++] = (gcode_template_vertex_t){ 0.0f, 0.0f, 0.0f };

    // Semicircle edge vertices (from 90° to 270°, i.e., the left half)
    // Radius 0.5: after shader multiplies by width, cap radius = 0.5*width = halfWidth
    // This matches terminal segment edges which are at +-halfWidth from center
    for (int i = 0; i <= cap_segments; i++) {
        float angle = 3.14159265359f * 0.5f + 3.14159265359f * (float)i / (float)cap_segments;
        float x = cosf(angle) * 0.5f;
        float y = sinf(angle) * 0.5f;
        vertices[vi++] = (gcode_template_vertex_t){ x, y, 0.0f };
    }

    // Triangle fan
    for (int i = 0; i < cap_segments; i++) {
        indices[ii++] = 0;
        indices[ii++] = 1 + i;
        indices[ii++] = 2 + i;
    }

    *vertex_count = vi;
    *index_count = ii;
}

// Pie-slice join template: triangle fan for angular fill
// The template_pos.x stores the vertex ID (0=center, 1..n=arc vertices)
// The shader computes the actual angle based on the neighboring segments
static inline void gcode_generate_pie_join_template(
    gcode_template_vertex_t* vertices, int* vertex_count,
    uint16_t* indices, int* index_count,
    int resolution
) {
    int vi = 0;
    int ii = 0;

    // Vertex 0: center (id=0)
    vertices[vi++] = (gcode_template_vertex_t){ 0.0f, 0.0f, 0.0f };

    // Vertices 1..resolution+1: arc points (id=1..resolution+1)
    for (int i = 0; i <= resolution; i++) {
        vertices[vi++] = (gcode_template_vertex_t){ (float)(i + 1), 0.0f, 0.0f };
    }

    // Triangle fan from center
    for (int i = 0; i < resolution; i++) {
        indices[ii++] = 0;           // center
        indices[ii++] = 1 + i;       // current arc vertex
        indices[ii++] = 2 + i;       // next arc vertex
    }

    *vertex_count = vi;
    *index_count = ii;
}

// Legacy full circle join template (kept for reference)
static inline void gcode_generate_join_template(
    gcode_template_vertex_t* vertices, int* vertex_count,
    uint16_t* indices, int* index_count,
    int segments
) {
    int vi = 0;
    int ii = 0;

    // Center vertex
    vertices[vi++] = (gcode_template_vertex_t){ 0.0f, 0.0f, 0.0f };

    // Circle edge vertices
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * 3.14159265359f * (float)i / (float)segments;
        float x = cosf(angle) * 0.5f;
        float y = sinf(angle) * 0.5f;
        vertices[vi++] = (gcode_template_vertex_t){ x, y, 0.0f };
    }

    // Triangle fan
    for (int i = 0; i < segments; i++) {
        indices[ii++] = 0;
        indices[ii++] = 1 + i;
        indices[ii++] = 2 + i;
    }

    *vertex_count = vi;
    *index_count = ii;
}

//------------------------------------------------------------------------------
// Smoothstep helper
//------------------------------------------------------------------------------
static inline float gcode_smoothstep(float edge0, float edge1, float x) {
    float t = (x - edge0) / (edge1 - edge0);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

//------------------------------------------------------------------------------
// Core functions
//------------------------------------------------------------------------------

static inline bool gcode_polyline_init(gcode_polyline_t* gp, const char* gcode_filename) {
    // Initialize path
    gcode_path_init(&gp->path);

    // Load G-code - use embedded data on web/iOS, file on desktop
#if defined(PLATFORM_WEB) || defined(PLATFORM_IOS)
    (void)gcode_filename;  // Unused on web/iOS
    if (!gcode_path_load_from_memory(&gp->path, gcode_benchy_data, gcode_benchy_size)) {
        return false;
    }
#else
    if (!gcode_path_load(&gp->path, gcode_filename)) {
        return false;
    }
#endif

    // Center the path
    gcode_path_center(&gp->path);

    // Initialize parameters
    gp->line_width = GCODE_POLYLINE_DEFAULT_WIDTH;
    gp->scale = GCODE_POLYLINE_DEFAULT_SCALE;

    // Default color: Catppuccin Frappé lavender (#babbf1)
    gp->color_r = 0.729f;
    gp->color_g = 0.733f;
    gp->color_b = 0.945f;

    gp->timeline_position = 0.5f;
    gp->base_alpha = GCODE_POLYLINE_DEFAULT_BASE_ALPHA;
    gp->highlight_width = GCODE_POLYLINE_DEFAULT_HIGHLIGHT_WIDTH;
    gp->miter_angle_limit = 150.0f;  // Default: use semicircle for angles > 150 degrees
    gp->debug_colors = true;

    // Calculate counts
    // For a polyline with n points: n-1 segments, n-2 joins
    // Segments: first (terminal), intermediate (n-3), last (terminal)
    // If n < 4, there are no intermediate segments
    int n = gp->path.count;
    gp->max_intermediate = (n >= 4) ? (n - 3) : 0;  // Segments 1 to n-3 (indices)
    gp->max_pie_joins = (n >= 3) ? (n - 2) : 0;     // Joins at vertices 1 to n-2

    // Allocate instance arrays
    gp->intermediate_segments = NULL;
    if (gp->max_intermediate > 0) {
        gp->intermediate_segments = (gcode_intermediate_segment_t*)malloc(
            gp->max_intermediate * sizeof(gcode_intermediate_segment_t));
    }

    gp->terminal_start = (gcode_terminal_segment_t*)malloc(sizeof(gcode_terminal_segment_t));
    gp->terminal_end = (gcode_terminal_segment_t*)malloc(sizeof(gcode_terminal_segment_t));

    gp->pie_joins = NULL;
    if (gp->max_pie_joins > 0) {
        gp->pie_joins = (gcode_pie_join_instance_t*)malloc(
            gp->max_pie_joins * sizeof(gcode_pie_join_instance_t));
    }

    gp->caps = (gcode_segment_instance_t*)malloc(2 * sizeof(gcode_segment_instance_t));

    gp->intermediate_count = 0;
    gp->pie_join_count = 0;
    gp->cap_count = 0;

    // Generate segment template (rectangle only)
    int max_seg_vertices = 4;
    int max_seg_indices = 6;
    gcode_template_vertex_t* seg_verts = (gcode_template_vertex_t*)malloc(max_seg_vertices * sizeof(gcode_template_vertex_t));
    uint16_t* seg_indices = (uint16_t*)malloc(max_seg_indices * sizeof(uint16_t));

    gcode_generate_segment_template(seg_verts, &gp->intermediate_template_vertex_count,
                                     seg_indices, &gp->intermediate_template_index_count);

    gp->intermediate_template_vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = { .ptr = seg_verts, .size = gp->intermediate_template_vertex_count * sizeof(gcode_template_vertex_t) },
        .label = "gcode-intermediate-template-vbuf"
    });
    gp->intermediate_template_ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = { .ptr = seg_indices, .size = gp->intermediate_template_index_count * sizeof(uint16_t) },
        .label = "gcode-intermediate-template-ibuf"
    });

    // Terminal segments use same template
    gp->terminal_template_vertex_count = gp->intermediate_template_vertex_count;
    gp->terminal_template_index_count = gp->intermediate_template_index_count;
    gp->terminal_template_vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = { .ptr = seg_verts, .size = gp->terminal_template_vertex_count * sizeof(gcode_template_vertex_t) },
        .label = "gcode-terminal-template-vbuf"
    });
    gp->terminal_template_ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = { .ptr = seg_indices, .size = gp->terminal_template_index_count * sizeof(uint16_t) },
        .label = "gcode-terminal-template-ibuf"
    });

    free(seg_verts);
    free(seg_indices);

    // Generate pie-slice join template
    int pie_resolution = GCODE_POLYLINE_JOIN_SEGMENTS;
    int max_pie_vertices = pie_resolution + 2;
    int max_pie_indices = pie_resolution * 3;
    gcode_template_vertex_t* pie_verts = (gcode_template_vertex_t*)malloc(max_pie_vertices * sizeof(gcode_template_vertex_t));
    uint16_t* pie_indices = (uint16_t*)malloc(max_pie_indices * sizeof(uint16_t));

    gcode_generate_pie_join_template(pie_verts, &gp->pie_join_template_vertex_count,
                                      pie_indices, &gp->pie_join_template_index_count,
                                      pie_resolution);

    gp->pie_join_template_vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = { .ptr = pie_verts, .size = gp->pie_join_template_vertex_count * sizeof(gcode_template_vertex_t) },
        .label = "gcode-pie-join-template-vbuf"
    });
    gp->pie_join_template_ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = { .ptr = pie_indices, .size = gp->pie_join_template_index_count * sizeof(uint16_t) },
        .label = "gcode-pie-join-template-ibuf"
    });

    free(pie_verts);
    free(pie_indices);

    // Generate cap template
    int max_cap_vertices = GCODE_POLYLINE_CAP_SEGMENTS + 2;
    int max_cap_indices = GCODE_POLYLINE_CAP_SEGMENTS * 3;
    gcode_template_vertex_t* cap_verts = (gcode_template_vertex_t*)malloc(max_cap_vertices * sizeof(gcode_template_vertex_t));
    uint16_t* cap_indices = (uint16_t*)malloc(max_cap_indices * sizeof(uint16_t));

    gcode_generate_cap_template(cap_verts, &gp->cap_template_vertex_count,
                                 cap_indices, &gp->cap_template_index_count,
                                 GCODE_POLYLINE_CAP_SEGMENTS);

    gp->cap_template_vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = { .ptr = cap_verts, .size = gp->cap_template_vertex_count * sizeof(gcode_template_vertex_t) },
        .label = "gcode-cap-template-vbuf"
    });
    gp->cap_template_ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = { .ptr = cap_indices, .size = gp->cap_template_index_count * sizeof(uint16_t) },
        .label = "gcode-cap-template-ibuf"
    });

    free(cap_verts);
    free(cap_indices);

    // Create instance buffers
    size_t intermediate_buf_size = (gp->max_intermediate > 0) ?
        gp->max_intermediate * sizeof(gcode_intermediate_segment_t) : 64;
    gp->intermediate_instance_buf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .usage.stream_update = true,
        .size = intermediate_buf_size,
        .label = "gcode-intermediate-instance-buf"
    });

    gp->terminal_start_instance_buf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .usage.stream_update = true,
        .size = sizeof(gcode_terminal_segment_t),
        .label = "gcode-terminal-start-instance-buf"
    });

    gp->terminal_end_instance_buf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .usage.stream_update = true,
        .size = sizeof(gcode_terminal_segment_t),
        .label = "gcode-terminal-end-instance-buf"
    });

    size_t pie_join_buf_size = (gp->max_pie_joins > 0) ?
        gp->max_pie_joins * sizeof(gcode_pie_join_instance_t) : 64;
    gp->pie_join_instance_buf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .usage.stream_update = true,
        .size = pie_join_buf_size,
        .label = "gcode-pie-join-instance-buf"
    });

    gp->cap_instance_buf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .usage.stream_update = true,
        .size = 2 * sizeof(gcode_segment_instance_t),
        .label = "gcode-cap-instance-buf"
    });

    // Create shaders
    gp->intermediate_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = gcode_intermediate_segment_vs_source, .entry = "vs_main" },
        .fragment_func = { .source = gcode_intermediate_segment_fs_source, .entry = "fs_main" },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(gcode_polyline_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
        },
        .label = "gcode-intermediate-shader"
    });

    gp->terminal_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = gcode_terminal_segment_vs_source, .entry = "vs_main" },
        .fragment_func = { .source = gcode_terminal_segment_fs_source, .entry = "fs_main" },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(gcode_polyline_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
        },
        .label = "gcode-terminal-shader"
    });

    gp->pie_join_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = gcode_pie_join_vs_source, .entry = "vs_main" },
        .fragment_func = { .source = gcode_pie_join_fs_source, .entry = "fs_main" },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(gcode_pie_join_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
        },
        .label = "gcode-pie-join-shader"
    });

    gp->cap_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = instanced_line_vs_source, .entry = "vs_main" },
        .fragment_func = { .source = instanced_line_fs_source, .entry = "fs_main" },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(gcode_polyline_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
        },
        .label = "gcode-cap-shader"
    });

    // Alpha blending configuration (reused by all pipelines)
    sg_blend_state alpha_blend = {
        .enabled = true,
        .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
        .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        .op_rgb = SG_BLENDOP_ADD,
        .src_factor_alpha = SG_BLENDFACTOR_ONE,
        .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        .op_alpha = SG_BLENDOP_ADD,
    };

    // Intermediate segment pipeline (4 points: pA, pB, pC, pD + color)
    gp->intermediate_pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = gp->intermediate_shd,
        .layout = {
            .buffers = {
                [0] = { .step_func = SG_VERTEXSTEP_PER_VERTEX },
                [1] = { .step_func = SG_VERTEXSTEP_PER_INSTANCE },
            },
            .attrs = {
                [0] = { .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3 },  // template_pos
                [1] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 0 },   // pA
                [2] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 12 },  // pB
                [3] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 24 },  // pC
                [4] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 36 },  // pD
                [5] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT4, .offset = 48 },  // color
            }
        },
        .index_type = SG_INDEXTYPE_UINT16,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .depth = { .compare = SG_COMPAREFUNC_LESS_EQUAL, .write_enabled = false, .pixel_format = SG_PIXELFORMAT_DEPTH },
        .colors[0] = { .pixel_format = SG_PIXELFORMAT_RGBA8, .blend = alpha_blend },
        .cull_mode = SG_CULLMODE_NONE,
        .label = "gcode-intermediate-pipeline"
    });

    // Terminal segment pipeline (3 points: pA, pB, pC + color)
    gp->terminal_pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = gp->terminal_shd,
        .layout = {
            .buffers = {
                [0] = { .step_func = SG_VERTEXSTEP_PER_VERTEX },
                [1] = { .step_func = SG_VERTEXSTEP_PER_INSTANCE },
            },
            .attrs = {
                [0] = { .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3 },  // template_pos
                [1] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 0 },   // pA
                [2] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 12 },  // pB
                [3] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 24 },  // pC
                [4] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT4, .offset = 36 },  // color
            }
        },
        .index_type = SG_INDEXTYPE_UINT16,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .depth = { .compare = SG_COMPAREFUNC_LESS_EQUAL, .write_enabled = false, .pixel_format = SG_PIXELFORMAT_DEPTH },
        .colors[0] = { .pixel_format = SG_PIXELFORMAT_RGBA8, .blend = alpha_blend },
        .cull_mode = SG_CULLMODE_NONE,
        .label = "gcode-terminal-pipeline"
    });

    // Pie-slice join pipeline (3 points: pA, pB, pC + color)
    gp->pie_join_pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = gp->pie_join_shd,
        .layout = {
            .buffers = {
                [0] = { .step_func = SG_VERTEXSTEP_PER_VERTEX },
                [1] = { .step_func = SG_VERTEXSTEP_PER_INSTANCE },
            },
            .attrs = {
                [0] = { .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3 },  // template_pos (id)
                [1] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 0 },   // pA
                [2] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 12 },  // pB
                [3] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 24 },  // pC
                [4] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT4, .offset = 36 },  // color
            }
        },
        .index_type = SG_INDEXTYPE_UINT16,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .depth = { .compare = SG_COMPAREFUNC_LESS_EQUAL, .write_enabled = false, .pixel_format = SG_PIXELFORMAT_DEPTH },
        .colors[0] = { .pixel_format = SG_PIXELFORMAT_RGBA8, .blend = alpha_blend },
        .cull_mode = SG_CULLMODE_NONE,
        .label = "gcode-pie-join-pipeline"
    });

    // Cap pipeline (2 points: pA, pB + color) - uses standard instanced line shader
    gp->cap_pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = gp->cap_shd,
        .layout = {
            .buffers = {
                [0] = { .step_func = SG_VERTEXSTEP_PER_VERTEX },
                [1] = { .step_func = SG_VERTEXSTEP_PER_INSTANCE },
            },
            .attrs = {
                [0] = { .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3 },  // template_pos
                [1] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 0 },   // point_a
                [2] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 12 },  // point_b
                [3] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT4, .offset = 24 },  // color
            }
        },
        .index_type = SG_INDEXTYPE_UINT16,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .depth = { .compare = SG_COMPAREFUNC_LESS_EQUAL, .write_enabled = false, .pixel_format = SG_PIXELFORMAT_DEPTH },
        .colors[0] = { .pixel_format = SG_PIXELFORMAT_RGBA8, .blend = alpha_blend },
        .cull_mode = SG_CULLMODE_NONE,
        .label = "gcode-cap-pipeline"
    });

    return true;
}

// Helper to scale a point
static inline void gcode_scale_point(gcode_point_t* p, float scale, float* ox, float* oy, float* oz) {
    *ox = p->x * scale;
    *oy = p->z * scale;  // Swap Y and Z for proper orientation
    *oz = p->y * scale;
}

static inline void gcode_polyline_update(gcode_polyline_t* gp) {
    if (gp->path.count < 2) return;

    gp->intermediate_count = 0;
    gp->pie_join_count = 0;
    gp->cap_count = 0;

    float total_len = gp->path.total_length;
    if (total_len < 0.001f) total_len = 1.0f;

    int n = gp->path.count;

    // Helper to compute alpha at a position
    #define COMPUTE_ALPHA(pos) ({ \
        float distance = fabsf((pos) - gp->timeline_position); \
        float fade_zone = gp->highlight_width; \
        if (fade_zone < 0.001f) fade_zone = 0.001f; \
        float highlight = 1.0f - gcode_smoothstep(0.0f, fade_zone, distance); \
        gp->base_alpha + (1.0f - gp->base_alpha) * highlight; \
    })

    // First segment (terminal start): P0-P1 with neighbor P2
    // For terminal shader: pA=P0, pB=P1, pC=P2
    if (n >= 2) {
        gcode_terminal_segment_t* ts = gp->terminal_start;
        float seg_pos = (gp->path.cumulative_lengths[0] + gp->path.cumulative_lengths[1]) * 0.5f / total_len;
        float alpha = COMPUTE_ALPHA(seg_pos);

        gcode_scale_point(&gp->path.points[0], gp->scale, &ts->pAx, &ts->pAy, &ts->pAz);
        gcode_scale_point(&gp->path.points[1], gp->scale, &ts->pBx, &ts->pBy, &ts->pBz);
        if (n >= 3) {
            gcode_scale_point(&gp->path.points[2], gp->scale, &ts->pCx, &ts->pCy, &ts->pCz);
        } else {
            // No neighbor, use P1 as dummy
            ts->pCx = ts->pBx; ts->pCy = ts->pBy; ts->pCz = ts->pBz;
        }

        // Debug: CYAN for start terminal
        if (gp->debug_colors) {
            ts->r = 0.0f; ts->g = 1.0f; ts->b = 1.0f;
        } else {
            ts->r = gp->color_r; ts->g = gp->color_g; ts->b = gp->color_b;
        }
        ts->a = alpha;
    }

    // Last segment (terminal end): P(n-2)-P(n-1) with neighbor P(n-3)
    // For terminal shader: pA = cap end (z=0), pB = miter end (z=1), pC = neighbor
    // We want the cap at P(n-1) and miter at P(n-2), so we REVERSE the point order:
    //   pA = P(n-1) (cap end, flat)
    //   pB = P(n-2) (miter end, joins with intermediate segments)
    //   pC = P(n-3) (neighbor for miter calculation)
    // This draws the segment "backwards" but that's intentional for correct miter placement
    if (n >= 2) {
        gcode_terminal_segment_t* te = gp->terminal_end;
        int last = n - 1;
        float seg_pos = (gp->path.cumulative_lengths[last-1] + gp->path.cumulative_lengths[last]) * 0.5f / total_len;
        float alpha = COMPUTE_ALPHA(seg_pos);

        // pA = P(n-1) - cap end (where the end cap goes)
        gcode_scale_point(&gp->path.points[last], gp->scale, &te->pAx, &te->pAy, &te->pAz);
        // pB = P(n-2) - miter end (joins with previous segment)
        gcode_scale_point(&gp->path.points[last-1], gp->scale, &te->pBx, &te->pBy, &te->pBz);
        // pC = P(n-3) - neighbor for miter calculation (or P(n-2) if n < 3)
        if (n >= 3) {
            gcode_scale_point(&gp->path.points[last-2], gp->scale, &te->pCx, &te->pCy, &te->pCz);
        } else {
            // Only 2 points, no neighbor - use pB as dummy
            te->pCx = te->pBx; te->pCy = te->pBy; te->pCz = te->pBz;
        }

        // Debug: MAGENTA for end terminal
        if (gp->debug_colors) {
            te->r = 1.0f; te->g = 0.0f; te->b = 1.0f;
        } else {
            te->r = gp->color_r; te->g = gp->color_g; te->b = gp->color_b;
        }
        te->a = alpha;
    }

    // Intermediate segments: P(i)-P(i+1) for i = 1 to n-3
    // Each needs: pA=P(i-1), pB=P(i), pC=P(i+1), pD=P(i+2)
    for (int i = 1; i <= n - 3; i++) {
        gcode_intermediate_segment_t* seg = &gp->intermediate_segments[gp->intermediate_count++];
        float seg_pos = (gp->path.cumulative_lengths[i] + gp->path.cumulative_lengths[i+1]) * 0.5f / total_len;
        float alpha = COMPUTE_ALPHA(seg_pos);

        gcode_scale_point(&gp->path.points[i-1], gp->scale, &seg->pAx, &seg->pAy, &seg->pAz);
        gcode_scale_point(&gp->path.points[i], gp->scale, &seg->pBx, &seg->pBy, &seg->pBz);
        gcode_scale_point(&gp->path.points[i+1], gp->scale, &seg->pCx, &seg->pCy, &seg->pCz);
        gcode_scale_point(&gp->path.points[i+2], gp->scale, &seg->pDx, &seg->pDy, &seg->pDz);

        // Debug: RED for intermediate segments
        if (gp->debug_colors) {
            seg->r = 1.0f; seg->g = 0.3f; seg->b = 0.3f;
        } else {
            seg->r = gp->color_r; seg->g = gp->color_g; seg->b = gp->color_b;
        }
        seg->a = alpha;
    }

    // Pie-slice joins at interior vertices: P(i) for i = 1 to n-2
    // Each needs: pA=P(i-1), pB=P(i), pC=P(i+1)
    for (int i = 1; i <= n - 2; i++) {
        gcode_pie_join_instance_t* join = &gp->pie_joins[gp->pie_join_count++];
        float pos = gp->path.cumulative_lengths[i] / total_len;
        float alpha = COMPUTE_ALPHA(pos);

        gcode_scale_point(&gp->path.points[i-1], gp->scale, &join->pAx, &join->pAy, &join->pAz);
        gcode_scale_point(&gp->path.points[i], gp->scale, &join->pBx, &join->pBy, &join->pBz);
        gcode_scale_point(&gp->path.points[i+1], gp->scale, &join->pCx, &join->pCy, &join->pCz);

        // Debug: YELLOW for joins
        if (gp->debug_colors) {
            join->r = 1.0f; join->g = 1.0f; join->b = 0.3f;
        } else {
            join->r = gp->color_r; join->g = gp->color_g; join->b = gp->color_b;
        }
        join->a = alpha;
    }

    // Caps at start (P0) and end (P(n-1))
    // Start cap
    {
        gcode_segment_instance_t* cap = &gp->caps[gp->cap_count++];
        float alpha = COMPUTE_ALPHA(0.0f);

        gcode_scale_point(&gp->path.points[0], gp->scale, &cap->ax, &cap->ay, &cap->az);
        gcode_scale_point(&gp->path.points[1], gp->scale, &cap->bx, &cap->by, &cap->bz);

        // Debug: BLUE for start cap
        if (gp->debug_colors) {
            cap->r = 0.3f; cap->g = 0.5f; cap->b = 1.0f;
        } else {
            cap->r = gp->color_r; cap->g = gp->color_g; cap->b = gp->color_b;
        }
        cap->a = alpha;
    }

    // End cap
    {
        gcode_segment_instance_t* cap = &gp->caps[gp->cap_count++];
        float alpha = COMPUTE_ALPHA(1.0f);

        gcode_scale_point(&gp->path.points[n-1], gp->scale, &cap->ax, &cap->ay, &cap->az);
        gcode_scale_point(&gp->path.points[n-2], gp->scale, &cap->bx, &cap->by, &cap->bz);

        // Debug: GREEN for end cap
        if (gp->debug_colors) {
            cap->r = 0.3f; cap->g = 1.0f; cap->b = 0.3f;
        } else {
            cap->r = gp->color_r; cap->g = gp->color_g; cap->b = gp->color_b;
        }
        cap->a = alpha;
    }

    #undef COMPUTE_ALPHA

    // Upload instance data
    if (gp->intermediate_count > 0) {
        sg_update_buffer(gp->intermediate_instance_buf, &(sg_range){
            .ptr = gp->intermediate_segments,
            .size = gp->intermediate_count * sizeof(gcode_intermediate_segment_t)
        });
    }

    sg_update_buffer(gp->terminal_start_instance_buf, &(sg_range){
        .ptr = gp->terminal_start,
        .size = sizeof(gcode_terminal_segment_t)
    });

    sg_update_buffer(gp->terminal_end_instance_buf, &(sg_range){
        .ptr = gp->terminal_end,
        .size = sizeof(gcode_terminal_segment_t)
    });

    if (gp->pie_join_count > 0) {
        sg_update_buffer(gp->pie_join_instance_buf, &(sg_range){
            .ptr = gp->pie_joins,
            .size = gp->pie_join_count * sizeof(gcode_pie_join_instance_t)
        });
    }

    sg_update_buffer(gp->cap_instance_buf, &(sg_range){
        .ptr = gp->caps,
        .size = gp->cap_count * sizeof(gcode_segment_instance_t)
    });
}

static inline void gcode_polyline_draw(gcode_polyline_t* gp, mat4_t mvp, float aspect_ratio) {
    if (gp->path.count < 2) return;

    gcode_polyline_params_t params = {
        .mvp = mvp,
        .line_width = gp->line_width,
        .aspect_ratio = aspect_ratio,
    };

    gcode_pie_join_params_t pie_params = {
        .mvp = mvp,
        .line_width = gp->line_width,
        .aspect_ratio = aspect_ratio,
        .join_resolution = (float)GCODE_POLYLINE_JOIN_SEGMENTS,
        .miter_angle_limit = gp->miter_angle_limit * 3.14159265359f / 180.0f,  // Convert degrees to radians
    };

    // Draw terminal start segment
    sg_apply_pipeline(gp->terminal_pip);
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers = { [0] = gp->terminal_template_vbuf, [1] = gp->terminal_start_instance_buf },
        .index_buffer = gp->terminal_template_ibuf,
    });
    sg_apply_uniforms(0, &SG_RANGE(params));
    sg_draw(0, gp->terminal_template_index_count, 1);

    // Draw terminal end segment
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers = { [0] = gp->terminal_template_vbuf, [1] = gp->terminal_end_instance_buf },
        .index_buffer = gp->terminal_template_ibuf,
    });
    sg_apply_uniforms(0, &SG_RANGE(params));
    sg_draw(0, gp->terminal_template_index_count, 1);

    // Draw intermediate segments
    if (gp->intermediate_count > 0) {
        sg_apply_pipeline(gp->intermediate_pip);
        sg_apply_bindings(&(sg_bindings){
            .vertex_buffers = { [0] = gp->intermediate_template_vbuf, [1] = gp->intermediate_instance_buf },
            .index_buffer = gp->intermediate_template_ibuf,
        });
        sg_apply_uniforms(0, &SG_RANGE(params));
        sg_draw(0, gp->intermediate_template_index_count, gp->intermediate_count);
    }

    // Draw pie-slice joins
    if (gp->pie_join_count > 0) {
        sg_apply_pipeline(gp->pie_join_pip);
        sg_apply_bindings(&(sg_bindings){
            .vertex_buffers = { [0] = gp->pie_join_template_vbuf, [1] = gp->pie_join_instance_buf },
            .index_buffer = gp->pie_join_template_ibuf,
        });
        sg_apply_uniforms(0, &SG_RANGE(pie_params));
        sg_draw(0, gp->pie_join_template_index_count, gp->pie_join_count);
    }

    // Draw caps
    if (gp->cap_count > 0) {
        sg_apply_pipeline(gp->cap_pip);
        sg_apply_bindings(&(sg_bindings){
            .vertex_buffers = { [0] = gp->cap_template_vbuf, [1] = gp->cap_instance_buf },
            .index_buffer = gp->cap_template_ibuf,
        });
        sg_apply_uniforms(0, &SG_RANGE(params));
        sg_draw(0, gp->cap_template_index_count, gp->cap_count);
    }
}

static inline void gcode_polyline_shutdown(gcode_polyline_t* gp) {
    sg_destroy_pipeline(gp->intermediate_pip);
    sg_destroy_pipeline(gp->terminal_pip);
    sg_destroy_pipeline(gp->pie_join_pip);
    sg_destroy_pipeline(gp->cap_pip);

    sg_destroy_shader(gp->intermediate_shd);
    sg_destroy_shader(gp->terminal_shd);
    sg_destroy_shader(gp->pie_join_shd);
    sg_destroy_shader(gp->cap_shd);

    sg_destroy_buffer(gp->intermediate_template_vbuf);
    sg_destroy_buffer(gp->intermediate_template_ibuf);
    sg_destroy_buffer(gp->intermediate_instance_buf);

    sg_destroy_buffer(gp->terminal_template_vbuf);
    sg_destroy_buffer(gp->terminal_template_ibuf);
    sg_destroy_buffer(gp->terminal_start_instance_buf);
    sg_destroy_buffer(gp->terminal_end_instance_buf);

    sg_destroy_buffer(gp->pie_join_template_vbuf);
    sg_destroy_buffer(gp->pie_join_template_ibuf);
    sg_destroy_buffer(gp->pie_join_instance_buf);

    sg_destroy_buffer(gp->cap_template_vbuf);
    sg_destroy_buffer(gp->cap_template_ibuf);
    sg_destroy_buffer(gp->cap_instance_buf);

    if (gp->intermediate_segments) {
        free(gp->intermediate_segments);
        gp->intermediate_segments = NULL;
    }
    if (gp->terminal_start) {
        free(gp->terminal_start);
        gp->terminal_start = NULL;
    }
    if (gp->terminal_end) {
        free(gp->terminal_end);
        gp->terminal_end = NULL;
    }
    if (gp->pie_joins) {
        free(gp->pie_joins);
        gp->pie_joins = NULL;
    }
    if (gp->caps) {
        free(gp->caps);
        gp->caps = NULL;
    }

    gcode_path_shutdown(&gp->path);
}

#endif // GCODE_POLYLINE_H
