#include "model.h"
#include <cmath>
#include <cstring>
namespace Model {
    Standard g_standard = D65;
    GamutStrategy g_strategy = CLIP;

    static double mx3(double a, double b, double c) { double m = a;if (b > m)m = b;if (c > m)m = c;return m; }
    static double mn3(double a, double b, double c) { double m = a;if (b < m)m = b;if (c < m)m = c;return m; }
    static double cl(double v, double lo, double hi) { return v < lo ? lo : (v > hi ? hi : v); }
    static double gd(double c) { return c <= 0.04045 ? c / 12.92 : pow((c + 0.055) / 1.055, 2.4); }
    static double gc(double c) { return c <= 0.0031308 ? 12.92 * c : 1.055 * pow(c, 1.0 / 2.4) - 0.055; }

    XYZ rgbToXyz(const RGB& c) {
        double r = gd(c.r / 255), g = gd(c.g / 255), b = gd(c.b / 255);
        double m[3][3];
        if (g_standard == D65) { double t[3][3] = { {0.4124564,0.3575761,0.1804375},{0.2126729,0.7151522,0.0721750},{0.0193339,0.1191920,0.9503041} };memcpy(m, t, sizeof t); }
        else if (g_standard == D50) { double t[3][3] = { {0.4360747,0.3850649,0.1430804},{0.2225045,0.7168786,0.0606169},{0.0139322,0.0971045,0.7141733} };memcpy(m, t, sizeof t); }
        else { double t[3][3] = { {0.497,0.339,0.164},{0.256,0.678,0.066},{0.023,0.113,0.864} };memcpy(m, t, sizeof t); }
        XYZ o;
        o.x = (m[0][0] * r + m[0][1] * g + m[0][2] * b) * 100;
        o.y = (m[1][0] * r + m[1][1] * g + m[1][2] * b) * 100;
        o.z = (m[2][0] * r + m[2][1] * g + m[2][2] * b) * 100;
        return o;
    }
    RGB xyzToRgb(const XYZ& c) {
        double x = c.x / 100, y = c.y / 100, z = c.z / 100, m[3][3];
        if (g_standard == D65) { double t[3][3] = { {3.2404542,-1.5371385,-0.4985314},{-0.9692660,1.8760108,0.0415560},{0.0556434,-0.2040259,1.0572252} };memcpy(m, t, sizeof t); }
        else if (g_standard == D50) { double t[3][3] = { {3.1338561,-1.6168667,-0.4906146},{-0.9787684,1.9161415,0.0334540},{0.0719453,-0.2289914,1.4052427} };memcpy(m, t, sizeof t); }
        else { double t[3][3] = { {2.3655,-1.1462,-0.3239},{-0.8975,1.7305,0.0443},{0.0546,-0.1957,1.0323} };memcpy(m, t, sizeof t); }
        RGB o;
        o.r = gc(m[0][0] * x + m[0][1] * y + m[0][2] * z) * 255;
        o.g = gc(m[1][0] * x + m[1][1] * y + m[1][2] * z) * 255;
        o.b = gc(m[2][0] * x + m[2][1] * y + m[2][2] * z) * 255;
        bool oob = o.r < 0 || o.r>255 || o.g < 0 || o.g>255 || o.b < 0 || o.b>255;
        if (oob) {
            if (g_strategy == CLIP) { o.r = cl(o.r, 0, 255);o.g = cl(o.g, 0, 255);o.b = cl(o.b, 0, 255); }
            else {
                double M = mx3(o.r, o.g, o.b), N = mn3(o.r, o.g, o.b);
                if (M > 255) { double s = 255 / M;o.r *= s;o.g *= s;o.b *= s; }
                if (N < 0) { double s = -N;o.r += s;o.g += s;o.b += s;double M2 = mx3(o.r, o.g, o.b);if (M2 > 255) { double k = 255 / M2;o.r *= k;o.g *= k;o.b *= k; } }
            }
        }
        return o;
    }
    HLS rgbToHls(const RGB& c) {
        double r = c.r / 255, g = c.g / 255, b = c.b / 255, M = mx3(r, g, b), N = mn3(r, g, b), h = 0, s = 0, l = (M + N) / 2;
        if (M != N) {
            double d = M - N;s = l > 0.5 ? d / (2 - M - N) : d / (M + N);
            if (M == r)h = (g - b) / d + (g < b ? 6 : 0);else if (M == g)h = (b - r) / d + 2;else h = (r - g) / d + 4;h /= 6;
        }
        HLS o;o.h = h * 360;o.l = l * 100;o.s = s * 100;return o;
    }
    static double h2r(double p, double q, double t) { if (t < 0)t += 1;if (t > 1)t -= 1;if (t < 1.0 / 6)return p + (q - p) * 6 * t;if (t < 1.0 / 2)return q;if (t < 2.0 / 3)return p + (q - p) * (2.0 / 3 - t) * 6;return p; }
    RGB hlsToRgb(const HLS& c) {
        double h = c.h / 360, l = c.l / 100, s = c.s / 100, r, g, b;
        if (s == 0) { r = g = b = l; }
        else {
            double q = l < 0.5 ? l * (1 + s) : l + s - l * s, p = 2 * l - q;
            r = h2r(p, q, h + 1.0 / 3);g = h2r(p, q, h);b = h2r(p, q, h - 1.0 / 3);
        }
        RGB o;o.r = r * 255;o.g = g * 255;o.b = b * 255;return o;
    }
}