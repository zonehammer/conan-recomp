// Conan's button prompt sheets redrawn with keyboard / mouse labels
// (prompts.cpp). Plain functions with no runtime dependencies.
//
// Every button icon the game shows comes from two DXT5 sheets in
// textures\hud: UI_HUD_Controller_XB.dds (1024x512: A / B / X / Y buttons in
// four sizes, LB / RB, LT / RT, the stick-rotate icons, Back) and
// UI_HUD_Sticks_XB.dds (512x512: the LS / RS stick heads at different angles).
// The disc has the PlayStation versions too (_PS), so PlayStation prompts are
// just those files. For the keyboard, each icon keeps its look - the coloured
// button, the silver bumper - and only its letters are replaced: the letters are
// found in a window over the icon's face (pale on the coloured buttons, dark on
// the bumpers and triggers, white on the stick heads), painted over from the
// surrounding face, and the bound key (or a mouse) drawn in their colour.
#pragma once
#include "prompt_art.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <vector>

namespace wet::conan_prompt_art
{
    using prompt_art::Box;
    using prompt_art::Label;
    using prompt_art::Rgba;
    using prompt_art::Seg;
    using prompt_art::Symbol;

    enum class Glyph { Pale, Dark, Bright };

    struct Icon
    {
        int x0, y0, x1, y1;           // the icon on the sheet
        Symbol sym;                   // the pad button it shows
        Glyph glyph;                  // how its letters look
        float wx0, wy0, wx1, wy1;     // where its letters are (fractions of the icon)
        float labelW;                 // label width as a fraction of the icon
    };

    // UI_HUD_Controller_XB.dds
    inline std::vector<Icon> ControllerIcons()
    {
        std::vector<Icon> icons;
        static const int kRows[4][2] = { { 16, 86 }, { 122, 188 }, { 233, 291 }, { 336, 394 } };
        static const int kCols[4][2] = { { 1, 104 }, { 104, 207 }, { 207, 310 }, { 310, 413 } };
        static const Symbol kSyms[4] = { Symbol::Cross, Symbol::Circle, Symbol::Square, Symbol::Triangle };
        for (auto& r : kRows)
            for (int c = 0; c < 4; c++)
                icons.push_back({ kCols[c][0], r[0], kCols[c][1], r[1], kSyms[c], Glyph::Pale, 0.2f, 0.02f, 0.8f, 0.7f,
                                  0.62f });
        icons.push_back({ 487, 377, 594, 429, Symbol::L1, Glyph::Dark, 0.18f, 0.14f, 0.93f, 0.80f, 0.62f });
        icons.push_back({ 598, 376, 705, 428, Symbol::R1, Glyph::Dark, 0.18f, 0.14f, 0.93f, 0.80f, 0.62f });
        icons.push_back({ 837, 344, 895, 453, Symbol::L2, Glyph::Dark, 0.12f, 0.25f, 0.92f, 0.80f, 0.70f });
        icons.push_back({ 945, 344, 1003, 453, Symbol::R2, Glyph::Dark, 0.12f, 0.25f, 0.92f, 0.80f, 0.70f });
        icons.push_back({ 855, 17, 985, 114, Symbol::LStick, Glyph::Bright, 0.28f, 0.10f, 0.72f, 0.50f, 0.42f });
        icons.push_back({ 855, 125, 985, 221, Symbol::RStick, Glyph::Bright, 0.28f, 0.10f, 0.72f, 0.50f, 0.42f });
        return icons;
    }

    // UI_HUD_Sticks_XB.dds: 4 x 5 stick heads, LS and RS (see the sheet).
    inline std::vector<Icon> StickIcons()
    {
        static const struct { int x0, y0, x1, y1; bool right; } kHeads[] = {
            { 1, 12, 73, 84, false },     { 112, 4, 193, 77, false },   { 222, 4, 303, 76, false },   { 342, 11, 415, 83, false },
            { 2, 105, 74, 177, true },    { 113, 105, 194, 178, true }, { 224, 106, 305, 178, true }, { 343, 104, 416, 176, true },
            { 12, 204, 97, 271, false },  { 116, 191, 201, 257, false }, { 218, 204, 303, 271, true }, { 322, 190, 407, 257, true },
            { 17, 295, 96, 360, false },  { 116, 287, 200, 366, false }, { 223, 295, 302, 360, true }, { 322, 287, 406, 366, true },
            { 17, 391, 96, 452, false },  { 114, 389, 199, 463, false }, { 223, 391, 302, 452, true }, { 320, 389, 405, 463, true },
        };
        std::vector<Icon> icons;
        for (auto& h : kHeads)
            icons.push_back({ h.x0, h.y0, h.x1, h.y1, h.right ? Symbol::RStick : Symbol::LStick, Glyph::Bright, 0.2f, 0.18f,
                              0.8f, 0.8f, 0.62f });
        return icons;
    }

    // UI_HUD_RenderText.dds (1024x256, 64-pixel cells): the icons inside text
    // ({ok}, {pickup}, ...), Xbox ones in columns 0-3 and the PlayStation ones in
    // columns 12-15 of the same sheet; the game always uses the Xbox ones.
    inline std::vector<Icon> RenderTextIcons()
    {
        return {
            { 19, 7, 56, 59, Symbol::R2, Glyph::Dark, 0.1f, 0.2f, 0.9f, 0.85f, 0.85f },
            { 82, 7, 119, 59, Symbol::L2, Glyph::Dark, 0.1f, 0.2f, 0.9f, 0.85f, 0.85f },
            { 135, 17, 192, 52, Symbol::R1, Glyph::Dark, 0.12f, 0.12f, 0.88f, 0.88f, 0.75f },
            { 199, 17, 256, 52, Symbol::L1, Glyph::Dark, 0.12f, 0.12f, 0.88f, 0.88f, 0.75f },
            { 8, 119, 64, 167, Symbol::Cross, Glyph::Pale, 0.15f, 0.08f, 0.85f, 0.8f, 0.7f },
            { 71, 118, 129, 168, Symbol::Circle, Glyph::Pale, 0.15f, 0.08f, 0.85f, 0.8f, 0.7f },
            { 135, 118, 193, 168, Symbol::Triangle, Glyph::Pale, 0.15f, 0.08f, 0.85f, 0.8f, 0.7f },
            { 200, 118, 256, 168, Symbol::Square, Glyph::Pale, 0.15f, 0.08f, 0.85f, 0.8f, 0.7f },
            { 9, 176, 63, 226, Symbol::LStick, Glyph::Dark, 0.12f, 0.15f, 0.88f, 0.85f, 0.8f },
            { 73, 176, 127, 226, Symbol::RStick, Glyph::Dark, 0.12f, 0.15f, 0.88f, 0.85f, 0.8f },
            { 135, 176, 193, 226, Symbol::Circle, Glyph::Bright, 0.15f, 0.1f, 0.85f, 0.85f, 0.7f },
        };
    }

    inline void Hsv(const Rgba& p, float& s, float& v)
    {
        int mx = std::max({ p.r, p.g, p.b }), mn = std::min({ p.r, p.g, p.b });
        v = mx / 255.0f;
        s = mx ? float(mx - mn) / mx : 0.0f;
    }

    // The letters: clearly lighter and greyer than the face (coloured buttons),
    // darker (bumpers, triggers), or white (stick heads); medS / medV are the
    // face's median saturation and brightness.
    inline bool IsGlyph(Glyph g, const Rgba& p, float medS, float medV)
    {
        if (p.a < 128)
            return false;
        float s, v;
        Hsv(p, s, v);
        switch (g)
        {
        case Glyph::Pale: return v > medV + 0.08f && s < medS * 0.65f;
        case Glyph::Dark: return v < medV - 0.35f;
        default: return v > std::max(0.62f, medV + 0.3f) && s < 0.3f;
        }
    }

    // Redraws one icon in place (px is the whole sheet, `stride` pixels wide).
    inline bool RedrawIcon(Rgba* px, int stride, const Icon& icon, const Label& label)
    {
        int iw = icon.x1 - icon.x0, ih = icon.y1 - icon.y0;
        int wx0 = icon.x0 + int(icon.wx0 * iw), wx1 = icon.x0 + int(icon.wx1 * iw);
        int wy0 = icon.y0 + int(icon.wy0 * ih), wy1 = icon.y0 + int(icon.wy1 * ih);
        int ww = wx1 - wx0, wh = wy1 - wy0;
        if (ww <= 4 || wh <= 4)
            return false;
        float medS = 0, medV = 0;
        {
            std::vector<float> ss, vs;
            for (int y = wy0; y < wy1; y++)
                for (int x = wx0; x < wx1; x++)
                    if (px[y * stride + x].a >= 128)
                    {
                        float s, v;
                        Hsv(px[y * stride + x], s, v);
                        ss.push_back(s);
                        vs.push_back(v);
                    }
            if (ss.size() < 16)
                return false;
            std::nth_element(ss.begin(), ss.begin() + ss.size() / 2, ss.end());
            std::nth_element(vs.begin(), vs.begin() + vs.size() / 2, vs.end());
            medS = ss[ss.size() / 2];
            medV = vs[vs.size() / 2];
        }
        std::vector<uint8_t> mask(size_t(ww) * wh, 0);
        double gr = 0, gg = 0, gb = 0, sx = 0, sy = 0;
        int n = 0;
        for (int y = 0; y < wh; y++)
            for (int x = 0; x < ww; x++)
            {
                const Rgba& p = px[(wy0 + y) * stride + wx0 + x];
                if (IsGlyph(icon.glyph, p, medS, medV))
                {
                    mask[y * ww + x] = 1;
                    gr += p.r; gg += p.g; gb += p.b; sx += x; sy += y; n++;
                }
            }
        if (n < 12)
            return false;
        // Its colour, and where it is.
        Rgba ink{ uint8_t(gr / n), uint8_t(gg / n), uint8_t(gb / n), 255 };
        float cx = wx0 + float(sx / n) + 0.5f, cy = wy0 + float(sy / n) + 0.5f;
        if (icon.glyph == Glyph::Pale)
        {
            // Lighter still, so short labels stay readable.
            ink.r = uint8_t(std::min(255, ink.r + 25));
            ink.g = uint8_t(std::min(255, ink.g + 25));
            ink.b = uint8_t(std::min(255, ink.b + 25));
        }
        // Grow for the anti-aliased edges (and the dark outline the coloured
        // buttons' letters have).
        std::vector<uint8_t> grown(mask);
        int grow = icon.glyph == Glyph::Pale ? 2 : 1;
        for (int y = 0; y < wh; y++)
            for (int x = 0; x < ww; x++)
                if (mask[y * ww + x])
                    for (int dy = -grow; dy <= grow; dy++)
                        for (int dx = -grow; dx <= grow; dx++)
                        {
                            int nx = x + dx, ny = y + dy;
                            if (nx >= 0 && ny >= 0 && nx < ww && ny < wh)
                                grown[ny * ww + nx] = 1;
                        }
        // Paint over the letters from the face around them: each pass fills the
        // masked pixels next to known ones with their average.
        for (int pass = 0; pass < ww + wh; pass++)
        {
            std::vector<uint8_t> next(grown);
            bool any = false;
            for (int y = 0; y < wh; y++)
                for (int x = 0; x < ww; x++)
                {
                    if (!grown[y * ww + x])
                        continue;
                    int r = 0, g = 0, b = 0, k = 0;
                    for (int dy = -1; dy <= 1; dy++)
                        for (int dx = -1; dx <= 1; dx++)
                        {
                            int nx = x + dx, ny = y + dy;
                            if ((!dx && !dy) || nx < 0 || ny < 0 || nx >= ww || ny >= wh || grown[ny * ww + nx])
                                continue;
                            const Rgba& q = px[(wy0 + ny) * stride + wx0 + nx];
                            if (q.a < 128)
                                continue;
                            r += q.r; g += q.g; b += q.b; k++;
                        }
                    if (!k)
                    {
                        any = true;
                        continue;
                    }
                    Rgba& p = px[(wy0 + y) * stride + wx0 + x];
                    p.r = uint8_t(r / k); p.g = uint8_t(g / k); p.b = uint8_t(b / k);
                    next[y * ww + x] = 0;
                }
            grown.swap(next);
            if (!any)
                break;
        }
        // The label, centred where the letters were.
        std::vector<Seg> segs;
        std::vector<Box> fills;
        float width = 1.5f;
        float bw = icon.labelW * iw, bh = float(wh) * (label.kind == Label::Kind::Mouse ? 0.72f : 0.86f);
        if (label.kind == Label::Kind::Text && label.text.size() <= 1)
            bw = std::min(bw, bh);
        prompt_art::LabelStrokes(label, cx, cy, bw, bh, segs, fills, width);
        width *= 1.15f;
        // A thin shadow under bright text on the coloured buttons keeps it crisp.
        bool shadow = icon.glyph != Glyph::Dark;
        int lx0 = std::max(icon.x0, int(cx - bw)), lx1 = std::min(icon.x1, int(cx + bw) + 1);
        int ly0 = std::max(icon.y0, int(cy - bh)), ly1 = std::min(icon.y1, int(cy + bh) + 1);
        for (int y = ly0; y < ly1; y++)
            for (int x = lx0; x < lx1; x++)
            {
                Rgba& p = px[y * stride + x];
                if (p.a < 128)
                    continue;
                float c = prompt_art::Coverage(float(x), float(y), segs, width, cx, cy, 0, &fills);
                if (shadow && c < 1)
                {
                    float sc = prompt_art::Coverage(float(x - 1), float(y - 1), segs, width, cx, cy, 0, &fills);
                    if (sc > c)
                    {
                        float k = (sc - c) * 0.35f;
                        p.r = uint8_t(p.r * (1 - k)); p.g = uint8_t(p.g * (1 - k)); p.b = uint8_t(p.b * (1 - k));
                    }
                }
                if (c <= 0)
                    continue;
                p.r = uint8_t(p.r + (ink.r - p.r) * c);
                p.g = uint8_t(p.g + (ink.g - p.g) * c);
                p.b = uint8_t(p.b + (ink.b - p.b) * c);
            }
        return true;
    }

    // Redraws every icon of a sheet that `labelFor` gives a keyboard / mouse
    // label for; the rest keep their pad art. Returns how many were redrawn.
    inline int RedrawSheet(Rgba* px, int w, int h, const std::vector<Icon>& icons,
                           const std::function<bool(Symbol, Label&)>& labelFor)
    {
        int redrawn = 0;
        for (auto& icon : icons)
        {
            if (icon.x1 > w || icon.y1 > h)
                continue;
            Label label;
            if (labelFor(icon.sym, label) && RedrawIcon(px, w, icon, label))
                redrawn++;
        }
        return redrawn;
    }
    // ---- PlayStation icons fitted over the Xbox ones ----
    // The game cuts each icon out of its sheet by the Xbox icon's rectangle, and
    // the PlayStation art doesn't always sit (or fit) in the same place, so each
    // PlayStation icon is centred on the Xbox one's spot, shrunk if it's bigger.

    struct Rect { int x0, y0, x1, y1; };

    // The opaque part of `search`; false if there's none.
    inline bool AlphaBounds(const Rgba* px, int w, Rect search, Rect& out)
    {
        out = { search.x1, search.y1, search.x0, search.y0 };
        for (int y = search.y0; y < search.y1; y++)
            for (int x = search.x0; x < search.x1; x++)
                if (px[size_t(y) * w + x].a > 8)
                {
                    out.x0 = std::min(out.x0, x); out.x1 = std::max(out.x1, x + 1);
                    out.y0 = std::min(out.y0, y); out.y1 = std::max(out.y1, y + 1);
                }
        return out.x1 > out.x0;
    }

    // Clears the Xbox icon (dstBox, its exact bounds) and puts the PlayStation
    // icon (srcBox in src) in its place. src may be dst if the boxes don't overlap.
    inline void PasteFitted(const Rgba* src, Rect srcBox, Rgba* dst, int w, Rect dstBox, Rect clip)
    {
        int sw = srcBox.x1 - srcBox.x0, sh = srcBox.y1 - srcBox.y0;
        float k = std::min({ 1.0f, float(dstBox.x1 - dstBox.x0) / sw, float(dstBox.y1 - dstBox.y0) / sh });
        int iw = std::max(1, int(sw * k + 0.5f)), ih = std::max(1, int(sh * k + 0.5f));
        std::vector<Rgba> icon(size_t(iw) * ih);
        for (int y = 0; y < ih; y++)
            for (int x = 0; x < iw; x++)
            {
                float fx = std::clamp((x + 0.5f) / k - 0.5f, 0.0f, float(sw - 1));
                float fy = std::clamp((y + 0.5f) / k - 0.5f, 0.0f, float(sh - 1));
                int ix = int(fx), iy = int(fy), jx = std::min(ix + 1, sw - 1), jy = std::min(iy + 1, sh - 1);
                float tx = fx - ix, ty = fy - iy;
                const Rgba* q[4] = { &src[size_t(srcBox.y0 + iy) * w + srcBox.x0 + ix], &src[size_t(srcBox.y0 + iy) * w + srcBox.x0 + jx],
                                     &src[size_t(srcBox.y0 + jy) * w + srcBox.x0 + ix], &src[size_t(srcBox.y0 + jy) * w + srcBox.x0 + jx] };
                float wq[4] = { (1 - tx) * (1 - ty), tx * (1 - ty), (1 - tx) * ty, tx * ty };
                float r = 0, g = 0, b = 0, a = 0, wsum = 0;
                for (int i = 0; i < 4; i++)
                {
                    // Colour weighted by alpha, so transparent edges don't darken it.
                    float wa = wq[i] * q[i]->a;
                    r += q[i]->r * wa; g += q[i]->g * wa; b += q[i]->b * wa;
                    a += q[i]->a * wq[i];
                    wsum += wa;
                }
                Rgba& o = icon[size_t(y) * iw + x];
                o.r = uint8_t(wsum > 0 ? r / wsum : 0);
                o.g = uint8_t(wsum > 0 ? g / wsum : 0);
                o.b = uint8_t(wsum > 0 ? b / wsum : 0);
                o.a = uint8_t(std::min(255.0f, a + 0.5f));
            }
        for (int y = dstBox.y0; y < dstBox.y1; y++)
            for (int x = dstBox.x0; x < dstBox.x1; x++)
                dst[size_t(y) * w + x] = { 0, 0, 0, 0 };
        int ox = (dstBox.x0 + dstBox.x1 - iw) / 2, oy = (dstBox.y0 + dstBox.y1 - ih) / 2;
        for (int y = 0; y < ih; y++)
            for (int x = 0; x < iw; x++)
            {
                int tx = ox + x, ty = oy + y;
                if (tx >= clip.x0 && ty >= clip.y0 && tx < clip.x1 && ty < clip.y1)
                    dst[size_t(ty) * w + tx] = icon[size_t(y) * iw + x];
            }
    }

    // Where to look for each icon: (on the Xbox sheet, on the PlayStation sheet).
    struct IconPair { Rect xb, ps; };

    inline std::vector<IconPair> ControllerPlayStationPairs()
    {
        std::vector<IconPair> pairs;
        for (auto& i : ControllerIcons())
        {
            Rect xb{ i.x0 - 1, i.y0 - 1, i.x1 + 1, i.y1 + 1 };
            Rect ps = xb;
            switch (i.sym)
            {
            case Symbol::L1: ps = { 478, 356, 592, 436 }; break;
            case Symbol::R1: ps = { 592, 356, 708, 436 }; break;
            case Symbol::L2: ps = { 816, 338, 920, 452 }; break;
            case Symbol::R2: ps = { 921, 338, 1024, 452 }; break;
            default: break;   // the same place on both sheets
            }
            pairs.push_back({ xb, ps });
        }
        pairs.push_back({ { 12, 418, 77, 480 }, { 12, 418, 77, 480 } });     // the glow behind a button
        pairs.push_back({ { 77, 418, 136, 480 }, { 77, 418, 136, 480 } });   // Back / Select
        return pairs;
    }

    inline std::vector<IconPair> StickPlayStationPairs()
    {
        std::vector<IconPair> pairs;
        for (auto& i : StickIcons())
            pairs.push_back({ { i.x0 - 1, i.y0 - 1, i.x1 + 1, i.y1 + 1 }, { i.x0 - 8, i.y0 - 8, i.x1 + 8, i.y1 + 8 } });
        return pairs;
    }

    // UI_HUD_RenderText.dds: Xbox icon -> the PlayStation icon for the same
    // button, in the PlayStation columns of the same sheet. The icons aren't on
    // an exact 64-pixel grid; these bands hold one each.
    inline std::vector<IconPair> RenderTextPlayStationPairs()
    {
        static const int kRows[4][2] = { { 0, 59 }, { 59, 115 }, { 115, 172 }, { 172, 232 } };
        static const int kXb[4][2] = { { 0, 66 }, { 66, 131 }, { 131, 196 }, { 196, 262 } };
        static const int kPs[4][2] = { { 764, 829 }, { 829, 893 }, { 893, 957 }, { 957, 1024 } };
        static const struct { int xb, row, ps; } kCells[] = {
            { 0, 0, 0 }, { 1, 0, 1 }, { 2, 0, 2 }, { 3, 0, 3 },   // RT LT RB LB -> R2 L2 R1 L1
            { 0, 1, 0 }, { 1, 1, 1 }, { 2, 1, 2 }, { 3, 1, 3 },   // d-pad, stick, back, start
            { 0, 2, 0 }, { 1, 2, 1 }, { 2, 2, 3 }, { 3, 2, 2 },   // A B Y X -> cross circle triangle square
            { 0, 3, 2 }, { 1, 3, 3 }, { 2, 3, 1 },                // LS RS, dark B -> dark circle
        };
        std::vector<IconPair> pairs;
        for (auto& c : kCells)
            pairs.push_back({ { kXb[c.xb][0], kRows[c.row][0], kXb[c.xb][1], kRows[c.row][1] },
                              { kPs[c.ps][0], kRows[c.row][0], kPs[c.ps][1], kRows[c.row][1] } });
        return pairs;
    }

    // The PlayStation 3 Select button in place of the Xbox Back icon (the
    // PlayStation art has only a plain dark rectangle there): a dark pill with a
    // light rim and SELECT written on it, centred in `box` (the Xbox icon's
    // bounds, cleared first).
    inline void DrawSelectButton(Rgba* px, int w, Rect box)
    {
        for (int y = box.y0; y < box.y1; y++)
            for (int x = box.x0; x < box.x1; x++)
                px[size_t(y) * w + x] = { 0, 0, 0, 0 };
        float bw = float(box.x1 - box.x0), bh = std::min(float(box.y1 - box.y0), bw * 0.46f);
        float cx = (box.x0 + box.x1) * 0.5f, cy = (box.y0 + box.y1) * 0.5f;
        float hw = bw * 0.5f - 0.5f, hh = bh * 0.5f, r = hh;
        Label label;
        label.text = "SELECT";
        std::vector<Seg> segs;
        std::vector<Box> fills;
        float width = 1.2f;
        prompt_art::LabelStrokes(label, cx, cy, (hw - r * 0.35f) * 2, hh * 1.25f, segs, fills, width);
        width = std::max(1.0f, width * 0.9f);
        for (int y = int(cy - hh) - 1; y <= int(cy + hh) + 1; y++)
            for (int x = box.x0; x < box.x1; x++)
            {
                if (y < box.y0 || y >= box.y1)
                    continue;
                // Distance outside the rounded rectangle (negative inside), 4x4 samples.
                float cover = 0, rim = 0;
                for (int sy = 0; sy < 4; sy++)
                    for (int sx = 0; sx < 4; sx++)
                    {
                        float fx = x + (sx + 0.5f) / 4 - cx, fy = y + (sy + 0.5f) / 4 - cy;
                        float qx = std::max(std::fabs(fx) - (hw - r), 0.0f), qy = std::max(std::fabs(fy) - (hh - r), 0.0f);
                        float d = std::sqrt(qx * qx + qy * qy) - r;
                        if (d <= 0)
                        {
                            cover += 1;
                            if (d > -1.3f)
                                rim += 1;
                        }
                    }
                cover /= 16;
                rim /= 16;
                if (cover <= 0)
                    continue;
                // Glossy black: lighter towards the top.
                float t = std::clamp((y + 0.5f - (cy - hh)) / (2 * hh), 0.0f, 1.0f);
                float base = 62 - 44 * t;
                float c = base + (190 - base) * (rim / std::max(cover, 1e-3f)) * 0.8f;
                float text = prompt_art::Coverage(float(x), float(y), segs, width, cx, cy, 0, &fills);
                c = c + (235 - c) * text;
                Rgba& p = px[size_t(y) * w + x];
                p.r = p.g = p.b = uint8_t(std::clamp(c, 0.0f, 255.0f));
                p.a = uint8_t(std::clamp(cover * 255.0f, 0.0f, 255.0f));
            }
    }

    // dst: the Xbox sheet, src: where the PlayStation icons are (the _PS sheet, or
    // the same sheet). Returns how many icons were placed.
    inline int FitPlayStationIcons(const Rgba* src, Rgba* dst, int w, int h, const std::vector<IconPair>& pairs)
    {
        int placed = 0;
        auto clamp = [&](Rect r) {
            return Rect{ std::max(0, r.x0), std::max(0, r.y0), std::min(w, r.x1), std::min(h, r.y1) };
        };
        for (auto& p : pairs)
        {
            Rect a, b, xb = clamp(p.xb), ps = clamp(p.ps);
            if (!AlphaBounds(dst, w, xb, a) || !AlphaBounds(src, w, ps, b))
                continue;
            PasteFitted(src, b, dst, w, a, xb);
            placed++;
        }
        return placed;
    }
}
