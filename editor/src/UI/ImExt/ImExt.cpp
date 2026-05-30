#include "edpch.h"
#include "ImExt.h"

#include <Ignis/Math/Quat.h>
#include "UI/SceneEditor/SceneEditorContext.h"

#include <imgui.h>

using namespace Ignis;

namespace
{
struct GizmoDragState
{
    int         axis   = -1;
    Math::Vec3f pos0   = {};
    Math::Quatf rot0   = Math::Quatf::identity();
    Math::Vec3f scl0   = Math::Vec3f::one();
    ImVec2      mouse0 = {};
    // rotation-specific
    float  angle_accum = 0.0f;
    float  grab_angle  = 0.0f;
    ImVec2 tangent_ss  = {};
    float  ring_r_ss   = 1.0f;
};

static GizmoDragState g_drag;

static const Math::Vec3f k_axes[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
// Ring tangent pairs: axis-i ring lies in tan1[i]/tan2[i] plane
static const Math::Vec3f k_tan1[3] = {{0, 1, 0}, {1, 0, 0}, {1, 0, 0}};
static const Math::Vec3f k_tan2[3] = {{0, 0, 1}, {0, 0, 1}, {0, 1, 0}};

static const ImU32 k_col[3] = {IM_COL32(220, 60, 60, 210), IM_COL32(60, 210, 60, 210), IM_COL32(60, 120, 220, 210)};
static const ImU32 k_col_bright[3] = {IM_COL32(255, 90, 90, 255), IM_COL32(90, 255, 90, 255),
                                      IM_COL32(90, 160, 255, 255)};
static const ImU32 k_col_dim[3]  = {IM_COL32(150, 40, 40, 100), IM_COL32(40, 140, 40, 100), IM_COL32(40, 80, 150, 100)};
static const ImU32 k_arc_fill[3] = {IM_COL32(220, 60, 60, 45), IM_COL32(60, 210, 60, 45), IM_COL32(60, 120, 220, 45)};
static const ImU32 k_hover       = IM_COL32(255, 220, 30, 255);
static const ImU32 k_hover_dim   = IM_COL32(200, 170, 20, 170);

static constexpr float k_hit_px  = 10.0f;
static constexpr float k_hit_rot = 16.0f;
static constexpr float k_line_w  = 2.5f;
static constexpr float k_line_wh = 3.5f;
static constexpr float k_cap_r   = 5.0f;
} // namespace

namespace Utils
{
static float len2(float x, float y)
{
    return Math::sqrt(x * x + y * y);
}

static ImVec2 world_to_screen(const Math::Vec3f& p, const Math::Mat4f& vp, ImVec2 vp_min, ImVec2 vp_size,
                              bool* in_front = nullptr)
{
    Math::Vec4f clip = vp * Math::Vec4f{p.x, p.y, p.z, 1.0f};
    if (in_front)
    {
        *in_front = clip.w > 0.0f;
    }
    if (clip.w == 0.0f)
    {
        clip.w = 1e-5f;
    }
    const float nx = clip.x / clip.w;
    const float ny = clip.y / clip.w;
    return {vp_min.x + (nx * 0.5f + 0.5f) * vp_size.x, vp_min.y + (0.5f - ny * 0.5f) * vp_size.y};
}

static float seg_dist(ImVec2 p, ImVec2 a, ImVec2 b)
{
    const float dx = b.x - a.x, dy = b.y - a.y;
    const float len_sq = dx * dx + dy * dy;
    if (len_sq < 1e-6f)
    {
        return len2(p.x - a.x, p.y - a.y);
    }
    const float t = Math::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / len_sq, 0.0f, 1.0f);
    return len2(p.x - (a.x + t * dx), p.y - (a.y + t * dy));
}

static ImU32 pick_col(int i, bool hov, bool active)
{
    if (active)
    {
        return k_col_bright[i];
    }
    if (hov)
    {
        return k_hover;
    }
    return k_col[i];
}

static bool draw_translate(Math::Transformf& t, const ImExt::GizmoCtx& ctx, ImVec2 ent_ss, ImDrawList* dl, ImGuiIO& io,
                           float handle_len)
{
    ImVec2 cap[3];
    bool   vis[3];
    for (int i = 0; i < 3; ++i)
    {
        cap[i] = world_to_screen(t.position + k_axes[i] * handle_len, ctx.view_proj, ctx.vp_min, ctx.vp_size, &vis[i]);
    }

    int hov = -1;
    if (g_drag.axis == -1)
    {
        for (int i = 0; i < 3; ++i)
        {
            if (!vis[i])
            {
                continue;
            }
            // Allow grabbing the line itself, not just the tip
            if (seg_dist(io.MousePos, ent_ss, cap[i]) < k_hit_px)
            {
                hov = i;
                break;
            }
        }
    }

    if (hov != -1 && io.MouseClicked[0])
    {
        g_drag        = GizmoDragState{};
        g_drag.axis   = hov;
        g_drag.pos0   = t.position;
        g_drag.mouse0 = io.MousePos;
    }

    bool modified = false;
    if (g_drag.axis != -1 && io.MouseDown[0])
    {
        const int    i    = g_drag.axis;
        const ImVec2 o_ss = world_to_screen(t.position, ctx.view_proj, ctx.vp_min, ctx.vp_size);
        const ImVec2 c_ss =
            world_to_screen(t.position + k_axes[i] * handle_len, ctx.view_proj, ctx.vp_min, ctx.vp_size);
        const float ss_len = Utils::len2(c_ss.x - o_ss.x, c_ss.y - o_ss.y);
        if (ss_len > 0.5f)
        {
            const float dir_x  = (c_ss.x - o_ss.x) / ss_len;
            const float dir_y  = (c_ss.y - o_ss.y) / ss_len;
            const float dot_px = io.MouseDelta.x * dir_x + io.MouseDelta.y * dir_y;
            t.position += k_axes[i] * (dot_px * (handle_len / ss_len));
            modified = true;

            // Update screen space origin for visual consistency this frame
            ent_ss = world_to_screen(t.position, ctx.view_proj, ctx.vp_min, ctx.vp_size);
        }
    }

    // Draw ghost origin and connecting line during drag
    if (g_drag.axis != -1 && io.MouseDown[0])
    {
        bool   start_vis = false;
        ImVec2 start_ss  = world_to_screen(g_drag.pos0, ctx.view_proj, ctx.vp_min, ctx.vp_size, &start_vis);

        if (start_vis)
        {
            // Semi-transparent line from the start position to current position
            dl->AddLine(start_ss, ent_ss, IM_COL32(255, 255, 255, 120), 2.0f);

            // Ghost dot at original position
            dl->AddCircleFilled(start_ss, 3.0f, IM_COL32(255, 255, 255, 150));
            dl->AddCircle(start_ss, 5.0f, IM_COL32(255, 255, 255, 200));

            // Show delta distance
            char  buf[32];
            float delta_dist = (t.position - g_drag.pos0).length();
            snprintf(buf, sizeof(buf), "d: %.2f", delta_dist);
            dl->AddText({io.MousePos.x + 14.0f, io.MousePos.y - 8.0f}, IM_COL32(255, 255, 255, 220), buf);
        }
    }

    // Draw central origin dot
    dl->AddCircleFilled(ent_ss, 3.0f, IM_COL32(255, 255, 255, 255));

    for (int i = 0; i < 3; ++i)
    {
        if (!vis[i])
        {
            continue;
        }

        // Recalculate caps visually if transformed
        ImVec2 draw_cap =
            modified ? world_to_screen(t.position + k_axes[i] * handle_len, ctx.view_proj, ctx.vp_min, ctx.vp_size)
                     : cap[i];

        const bool  active = (i == g_drag.axis);
        const bool  hover  = (i == hov);
        const ImU32 col    = pick_col(i, hover, active);
        const float lw     = (hover || active) ? k_line_wh : k_line_w;

        // Draw axis line
        dl->AddLine(ent_ss, draw_cap, col, lw);

        // Draw directional arrows instead of circles
        const float dx  = draw_cap.x - ent_ss.x;
        const float dy  = draw_cap.y - ent_ss.y;
        const float len = Utils::len2(dx, dy);

        if (len > 0.1f)
        {
            const float dir_x       = dx / len;
            const float dir_y       = dy / len;
            const float arrow_size  = (hover || active) ? 14.0f : 10.0f;
            const float arrow_width = arrow_size * 0.4f;

            ImVec2 p1 = {draw_cap.x + dir_x * arrow_size, draw_cap.y + dir_y * arrow_size};
            ImVec2 p2 = {draw_cap.x - dir_x * arrow_size * 0.5f - dir_y * arrow_width,
                         draw_cap.y - dir_y * arrow_size * 0.5f + dir_x * arrow_width};
            ImVec2 p3 = {draw_cap.x - dir_x * arrow_size * 0.5f + dir_y * arrow_width,
                         draw_cap.y - dir_y * arrow_size * 0.5f - dir_x * arrow_width};

            dl->AddTriangleFilled(p1, p2, p3, col);

            if (active)
            {
                dl->AddTriangle(p1, p2, p3, IM_COL32(255, 255, 255, 150), 1.5f);
            }
        }
    }
    return modified;
}

static bool draw_rotate(Math::Transformf& t, const ImExt::GizmoCtx& ctx, ImVec2 ent_ss, ImDrawList* dl, ImGuiIO& io,
                        float handle_len)
{
    constexpr int k_seg = 48;

    // Camera forward from view matrix
    const Math::Vec3f cam_fwd = {ctx.view.m[2], ctx.view.m[6], ctx.view.m[10]};

    int hov     = -1;
    int hov_seg = 0;

    if (g_drag.axis == -1)
    {
        for (int ax = 0; ax < 3 && hov == -1; ++ax)
        {
            for (int s = 0; s < k_seg; ++s)
            {
                const float       a0 = float(s) / float(k_seg) * Math::tau;
                const float       a1 = float(s + 1) / float(k_seg) * Math::tau;
                const Math::Vec3f p0 =
                    t.position + k_tan1[ax] * (Math::cos(a0) * handle_len) + k_tan2[ax] * (Math::sin(a0) * handle_len);
                const Math::Vec3f p1 =
                    t.position + k_tan1[ax] * (Math::cos(a1) * handle_len) + k_tan2[ax] * (Math::sin(a1) * handle_len);
                const ImVec2 s0 = world_to_screen(p0, ctx.view_proj, ctx.vp_min, ctx.vp_size);
                const ImVec2 s1 = world_to_screen(p1, ctx.view_proj, ctx.vp_min, ctx.vp_size);
                if (seg_dist(io.MousePos, s0, s1) < k_hit_rot)
                {
                    hov     = ax;
                    hov_seg = s;
                    break;
                }
            }
        }
    }

    if (hov != -1 && io.MouseClicked[0])
    {
        const int   ax = hov;
        const float ga = (float(hov_seg) + 0.5f) / float(k_seg) * Math::tau;

        g_drag             = GizmoDragState{};
        g_drag.axis        = ax;
        g_drag.rot0        = t.rotation;
        g_drag.grab_angle  = ga;
        g_drag.angle_accum = 0.0f;

        const Math::Vec3f grab_ws =
            t.position + k_tan1[ax] * (Math::cos(ga) * handle_len) + k_tan2[ax] * (Math::sin(ga) * handle_len);
        const Math::Vec3f tang_ws = k_tan2[ax] * Math::cos(ga) - k_tan1[ax] * Math::sin(ga);

        const ImVec2 grab_ss = world_to_screen(grab_ws, ctx.view_proj, ctx.vp_min, ctx.vp_size);
        const ImVec2 tang_tip =
            world_to_screen(grab_ws + tang_ws * (handle_len * 0.3f), ctx.view_proj, ctx.vp_min, ctx.vp_size);
        const float tx    = tang_tip.x - grab_ss.x;
        const float ty    = tang_tip.y - grab_ss.y;
        const float tlen  = len2(tx, ty);
        g_drag.tangent_ss = tlen > 0.01f ? ImVec2{tx / tlen, ty / tlen} : ImVec2{1.0f, 0.0f};
        g_drag.ring_r_ss  = Math::max(len2(grab_ss.x - ent_ss.x, grab_ss.y - ent_ss.y), 1.0f);
    }

    bool modified = false;
    if (g_drag.axis != -1 && io.MouseDown[0])
    {
        const float dot_px = io.MouseDelta.x * g_drag.tangent_ss.x + io.MouseDelta.y * g_drag.tangent_ss.y;
        g_drag.angle_accum += dot_px / g_drag.ring_r_ss;
        t.rotation = Math::Quatf::from_axis_angle(k_axes[g_drag.axis], g_drag.angle_accum) * g_drag.rot0;
        modified   = true;
    }

    for (int ax = 0; ax < 3; ++ax)
    {
        const bool active = (ax == g_drag.axis);
        const bool hover  = (ax == hov);

        for (int s = 0; s < k_seg; ++s)
        {
            const float a0    = float(s) / float(k_seg) * Math::tau;
            const float a1    = float(s + 1) / float(k_seg) * Math::tau;
            const float a_mid = (a0 + a1) * 0.5f;

            const float nx     = k_tan1[ax].x * Math::cos(a_mid) + k_tan2[ax].x * Math::sin(a_mid);
            const float ny     = k_tan1[ax].y * Math::cos(a_mid) + k_tan2[ax].y * Math::sin(a_mid);
            const float nz     = k_tan1[ax].z * Math::cos(a_mid) + k_tan2[ax].z * Math::sin(a_mid);
            const float facing = nx * cam_fwd.x + ny * cam_fwd.y + nz * cam_fwd.z;

            const Math::Vec3f p0_ws =
                t.position + k_tan1[ax] * (Math::cos(a0) * handle_len) + k_tan2[ax] * (Math::sin(a0) * handle_len);
            const Math::Vec3f p1_ws =
                t.position + k_tan1[ax] * (Math::cos(a1) * handle_len) + k_tan2[ax] * (Math::sin(a1) * handle_len);
            const ImVec2 ss0 = world_to_screen(p0_ws, ctx.view_proj, ctx.vp_min, ctx.vp_size);
            const ImVec2 ss1 = world_to_screen(p1_ws, ctx.view_proj, ctx.vp_min, ctx.vp_size);

            const bool front = facing >= 0.0f;
            ImU32      col;
            float      lw;
            if (active)
            {
                col = front ? k_col_bright[ax] : k_col_dim[ax];
                lw  = front ? 3.5f : 1.5f;
            }
            else if (hover)
            {
                col = front ? k_hover : k_hover_dim;
                lw  = front ? 3.0f : 1.5f;
            }
            else
            {
                col = front ? k_col[ax] : k_col_dim[ax];
                lw  = front ? 2.0f : 1.2f;
            }

            dl->AddLine(ss0, ss1, col, lw);
        }
    }

    if (g_drag.axis != -1 && io.MouseDown[0] && Math::abs(g_drag.angle_accum) > 0.001f)
    {
        const int     ax      = g_drag.axis;
        const float   a_start = g_drag.grab_angle;
        const float   a_end   = g_drag.grab_angle + g_drag.angle_accum;
        constexpr int k_arc   = 32;

        for (int k = 0; k < k_arc; ++k)
        {
            const float       a0 = a_start + (a_end - a_start) * float(k) / float(k_arc);
            const float       a1 = a_start + (a_end - a_start) * float(k + 1) / float(k_arc);
            const Math::Vec3f w0 =
                t.position + k_tan1[ax] * (Math::cos(a0) * handle_len) + k_tan2[ax] * (Math::sin(a0) * handle_len);
            const Math::Vec3f w1 =
                t.position + k_tan1[ax] * (Math::cos(a1) * handle_len) + k_tan2[ax] * (Math::sin(a1) * handle_len);
            dl->AddTriangleFilled(ent_ss, world_to_screen(w0, ctx.view_proj, ctx.vp_min, ctx.vp_size),
                                  world_to_screen(w1, ctx.view_proj, ctx.vp_min, ctx.vp_size), k_arc_fill[ax]);
        }

        // Wrap rotation readout to [0, 360)
        float deg = Math::degrees(g_drag.angle_accum);
        deg       = Math::fmod(deg, 360.0f);
        if (deg < 0.0f)
        {
            deg += 360.0f;
        }

        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f\xC2\xB0", deg);
        dl->AddText({io.MousePos.x + 14.0f, io.MousePos.y - 8.0f}, IM_COL32(255, 255, 255, 220), buf);
    }

    return modified;
}

static bool draw_scale(Math::Transformf& t, const ImExt::GizmoCtx& ctx, ImVec2 ent_ss, ImDrawList* dl, ImGuiIO& io,
                       float handle_len)
{
    constexpr float k_box  = 5.0f;
    constexpr float k_sens = 0.006f;

    Math::Vec3f local[3];
    for (int i = 0; i < 3; ++i)
    {
        local[i] = t.rotation.rotate(k_axes[i]);
    }

    ImVec2 cap[3];
    bool   vis[3];
    for (int i = 0; i < 3; ++i)
    {
        cap[i] = world_to_screen(t.position + local[i] * handle_len, ctx.view_proj, ctx.vp_min, ctx.vp_size, &vis[i]);
    }

    int hov = -1;
    if (g_drag.axis == -1)
    {
        for (int i = 0; i < 3; ++i)
        {
            if (!vis[i])
            {
                continue;
            }
            if (Math::abs(io.MousePos.x - cap[i].x) < k_hit_px && Math::abs(io.MousePos.y - cap[i].y) < k_hit_px)
            {
                hov = i;
                break;
            }
        }
    }

    if (hov != -1 && io.MouseClicked[0])
    {
        g_drag        = GizmoDragState{};
        g_drag.axis   = hov;
        g_drag.pos0   = t.position;
        g_drag.rot0   = t.rotation;
        g_drag.scl0   = t.scale;
        g_drag.mouse0 = io.MousePos;
    }

    bool modified = false;
    if (g_drag.axis != -1 && io.MouseDown[0])
    {
        const int         i    = g_drag.axis;
        const Math::Vec3f la   = g_drag.rot0.rotate(k_axes[i]);
        const ImVec2      o_ss = world_to_screen(t.position, ctx.view_proj, ctx.vp_min, ctx.vp_size);
        const ImVec2      c_ss = world_to_screen(t.position + la * handle_len, ctx.view_proj, ctx.vp_min, ctx.vp_size);
        const float       ss_len = len2(c_ss.x - o_ss.x, c_ss.y - o_ss.y);
        if (ss_len > 0.5f)
        {
            const float dir_x = (c_ss.x - o_ss.x) / ss_len;
            const float dir_y = (c_ss.y - o_ss.y) / ss_len;
            const float cumul = (io.MousePos.x - g_drag.mouse0.x) * dir_x + (io.MousePos.y - g_drag.mouse0.y) * dir_y;
            t.scale[i]        = Math::max(g_drag.scl0[i] * (1.0f + cumul * k_sens), 0.01f);
            modified          = true;
        }
    }

    for (int i = 0; i < 3; ++i)
    {
        if (!vis[i])
        {
            continue;
        }

        // Recalculate cap visually if scaled
        ImVec2 draw_cap = modified
                              ? world_to_screen(t.position + local[i] * (handle_len * (t.scale[i] / g_drag.scl0[i])),
                                                ctx.view_proj, ctx.vp_min, ctx.vp_size)
                              : cap[i];

        const bool  active = (i == g_drag.axis);
        const bool  hover  = (i == hov);
        const ImU32 col    = pick_col(i, hover, active);
        const float lw     = (hover || active) ? k_line_wh : k_line_w;
        dl->AddLine(ent_ss, draw_cap, col, lw);
        dl->AddRectFilled({draw_cap.x - k_box, draw_cap.y - k_box}, {draw_cap.x + k_box, draw_cap.y + k_box}, col);
        if (active)
        {
            dl->AddRect({draw_cap.x - k_box - 2.0f, draw_cap.y - k_box - 2.0f},
                        {draw_cap.x + k_box + 2.0f, draw_cap.y + k_box + 2.0f}, IM_COL32(255, 255, 255, 110), 0.0f, 0,
                        1.5f);
        }
    }

    // Show scale change tooltip while dragging
    if (g_drag.axis != -1 && io.MouseDown[0])
    {
        const int i = g_drag.axis;
        char      buf[64];

        float ratio = t.scale[i] / g_drag.scl0[i];
        snprintf(buf, sizeof(buf), "Scale: %.2f (%.2fx)", t.scale[i], ratio);

        ImVec2 text_size = ImGui::CalcTextSize(buf);
        ImVec2 text_pos  = {io.MousePos.x + 14.0f, io.MousePos.y - 8.0f};

        dl->AddRectFilled({text_pos.x - 2.0f, text_pos.y - 2.0f},
                          {text_pos.x + text_size.x + 4.0f, text_pos.y + text_size.y + 2.0f}, IM_COL32(20, 20, 20, 200),
                          2.0f);
        dl->AddText(text_pos, IM_COL32(255, 255, 255, 255), buf);
    }

    return modified;
}
} // namespace Utils

namespace ImExt
{
bool ManipulateTransform(Math::Transformf& transform, GizmoMode mode, const GizmoCtx& ctx)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGuiIO&    io = ImGui::GetIO();

    if (!io.MouseDown[0])
    {
        g_drag.axis = -1;
    }

    bool   in_front = false;
    ImVec2 ent_ss   = Utils::world_to_screen(transform.position, ctx.view_proj, ctx.vp_min, ctx.vp_size, &in_front);
    if (!in_front)
    {
        return false;
    }

    const float dist       = (transform.position - ctx.cam_pos).length();
    const float handle_len = Math::max(dist * 0.12f, 0.5f);

    switch (mode)
    {
        case GizmoMode::Translate:
            return Utils::draw_translate(transform, ctx, ent_ss, dl, io, handle_len);
        case GizmoMode::Rotate:
            return Utils::draw_rotate(transform, ctx, ent_ss, dl, io, handle_len);
        case GizmoMode::Scale:
            return Utils::draw_scale(transform, ctx, ent_ss, dl, io, handle_len);
    }
    return false;
}

int DrawCameraOrientation(const Math::Mat4f& view, ImVec2 center, float size)
{
    const float sx[3]    = {view.m[0], view.m[4], view.m[8]};
    const float sy[3]    = {view.m[1], view.m[5], view.m[9]};
    const float depth[3] = {view.m[2], view.m[6], view.m[10]};

    // Brighter colors for better contrast against dark UI
    static const ImU32 k_full[3] = {IM_COL32(240, 80, 80, 255), IM_COL32(80, 240, 80, 255),
                                    IM_COL32(80, 140, 255, 255)};
    static const ImU32 k_dim2[3] = {IM_COL32(140, 50, 50, 180), IM_COL32(50, 140, 50, 180), IM_COL32(50, 90, 160, 180)};
    static const char* k_lbl[3]  = {"X", "Y", "Z"};

    constexpr float k_neg_scale = 0.55f;
    constexpr float k_tip_r     = 12.0f;

    ImVec2 pos_tip[3], neg_tip[3];
    for (int i = 0; i < 3; ++i)
    {
        pos_tip[i] = {center.x + sx[i] * size, center.y - sy[i] * size};
        neg_tip[i] = {center.x - sx[i] * size * k_neg_scale, center.y + sy[i] * size * k_neg_scale};
    }

    ImGuiIO&    io = ImGui::GetIO();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Hover detection
    int hover = -1;
    for (int i = 0; i < 3 && hover == -1; ++i)
    {
        const float dx = io.MousePos.x - pos_tip[i].x;
        const float dy = io.MousePos.y - pos_tip[i].y;
        if (dx * dx + dy * dy < k_tip_r * k_tip_r)
        {
            hover = i;
        }
    }
    for (int i = 0; i < 3 && hover == -1; ++i)
    {
        const float dx = io.MousePos.x - neg_tip[i].x;
        const float dy = io.MousePos.y - neg_tip[i].y;
        if (dx * dx + dy * dy < k_tip_r * k_tip_r)
        {
            hover = i + 3;
        }
    }

    const int clicked = (hover >= 0 && io.MouseClicked[0]) ? hover : -1;

    // Unified background for the widget
    dl->AddCircleFilled(center, size * 1.2f, IM_COL32(30, 30, 30, 150));
    dl->AddCircle(center, size * 1.2f, IM_COL32(100, 100, 100, 80), 32, 1.0f);

    // Back-to-front draw order
    int order[3] = {0, 1, 2};
    if (depth[order[0]] < depth[order[1]])
    {
        std::swap(order[0], order[1]);
    }
    if (depth[order[1]] < depth[order[2]])
    {
        std::swap(order[1], order[2]);
    }
    if (depth[order[0]] < depth[order[1]])
    {
        std::swap(order[0], order[1]);
    }

    // Negative tips
    for (int i = 0; i < 3; ++i)
    {
        const bool  hov_neg = (hover == i + 3);
        const ImU32 col     = hov_neg ? IM_COL32(255, 255, 255, 200) : k_dim2[i];
        dl->AddLine(center, neg_tip[i], col, hov_neg ? 2.5f : 1.5f);
        dl->AddCircleFilled(neg_tip[i], hov_neg ? 6.0f : 4.0f, col);
    }

    // Positive axes back-to-front
    for (int pass = 0; pass < 3; ++pass)
    {
        const int   i     = order[pass];
        const bool  hov   = (hover == i);
        const bool  front = depth[i] >= 0.0f;
        const ImU32 col   = hov ? IM_COL32(255, 240, 50, 255) : (front ? k_full[i] : k_dim2[i]);
        const float lw    = hov ? 3.5f : 2.5f;
        const float cr    = hov ? k_tip_r + 2.0f : k_tip_r;

        dl->AddLine(center, pos_tip[i], col, lw);
        dl->AddCircleFilled(pos_tip[i], cr, col);

        // Centered text inside the circle
        ImVec2 text_size = ImGui::CalcTextSize(k_lbl[i]);
        ImVec2 text_pos  = {pos_tip[i].x - text_size.x * 0.5f, pos_tip[i].y - text_size.y * 0.5f};
        ImU32  text_col  = hov ? IM_COL32(0, 0, 0, 255) : IM_COL32(0, 0, 0, 255);
        dl->AddText(text_pos, text_col, k_lbl[i]);
    }

    return clicked;
}
} // namespace ImExt
