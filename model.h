#pragma once
namespace Model {
    enum Standard { D65, D50, E_STD };
    enum GamutStrategy { CLIP, SCALE };
    struct RGB { double r, g, b; };
    struct XYZ { double x, y, z; };
    struct HLS { double h, l, s; };
    extern Standard g_standard;
    extern GamutStrategy g_strategy;
    XYZ rgbToXyz(const RGB&);
    RGB xyzToRgb(const XYZ&);
    HLS rgbToHls(const RGB&);
    RGB hlsToRgb(const HLS&);
}