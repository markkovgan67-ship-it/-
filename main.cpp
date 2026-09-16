#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <cmath>
#include <cstdio>
#include <cwchar>
#include <cstring>
#include <cstdlib>

// model
namespace Model {
    enum Standard { D65, D50, E_STD };
    enum GamutStrategy { CLIP, SCALE };
    struct RGB { double r, g, b; };
    struct XYZ { double x, y, z; };
    struct HLS { double h, l, s; };

    static Standard g_standard = D65;
    static GamutStrategy g_strategy = CLIP;

    static double mx3(double a, double b, double c) { double m = a;if (b > m)m = b;if (c > m)m = c;return m; }
    static double mn3(double a, double b, double c) { double m = a;if (b < m)m = b;if (c < m)m = c;return m; }
    static double cl(double v, double lo, double hi) { return v < lo ? lo : (v > hi ? hi : v); }
    static double gd(double c) { return c <= 0.04045 ? c / 12.92 : pow((c + 0.055) / 1.055, 2.4); }
    static double gc(double c) { return c <= 0.0031308 ? 12.92 * c : 1.055 * pow(c, 1.0 / 2.4) - 0.055; }

    static XYZ rgbToXyz(const RGB& c) {
        double r = gd(c.r / 255), g = gd(c.g / 255), b = gd(c.b / 255);
        double m[3][3];
        if (g_standard == D65) {
            double t[3][3] = { {0.4124564,0.3575761,0.1804375},
                            {0.2126729,0.7151522,0.0721750},
                            {0.0193339,0.1191920,0.9503041} };
            memcpy(m, t, sizeof t);
        }
        else if (g_standard == D50) {
            double t[3][3] = { {0.4360747,0.3850649,0.1430804},
                            {0.2225045,0.7168786,0.0606169},
                            {0.0139322,0.0971045,0.7141733} };
            memcpy(m, t, sizeof t);
        }
        else {
            double t[3][3] = { {0.497,0.339,0.164},
                            {0.256,0.678,0.066},
                            {0.023,0.113,0.864} };
            memcpy(m, t, sizeof t);
        }
        XYZ o;
        o.x = (m[0][0] * r + m[0][1] * g + m[0][2] * b) * 100;
        o.y = (m[1][0] * r + m[1][1] * g + m[1][2] * b) * 100;
        o.z = (m[2][0] * r + m[2][1] * g + m[2][2] * b) * 100;
        return o;
    }
    static RGB xyzToRgb(const XYZ& c) {
        double x = c.x / 100, y = c.y / 100, z = c.z / 100, m[3][3];
        if (g_standard == D65) {
            double t[3][3] = { {3.2404542,-1.5371385,-0.4985314},
                            {-0.9692660,1.8760108,0.0415560},
                            {0.0556434,-0.2040259,1.0572252} };
            memcpy(m, t, sizeof t);
        }
        else if (g_standard == D50) {
            double t[3][3] = { {3.1338561,-1.6168667,-0.4906146},
                            {-0.9787684,1.9161415,0.0334540},
                            {0.0719453,-0.2289914,1.4052427} };
            memcpy(m, t, sizeof t);
        }
        else {
            double t[3][3] = { {2.3655,-1.1462,-0.3239},
                            {-0.8975,1.7305,0.0443},
                            {0.0546,-0.1957,1.0323} };
            memcpy(m, t, sizeof t);
        }
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
                if (N < 0) {
                    double s = -N;o.r += s;o.g += s;o.b += s;
                    double M2 = mx3(o.r, o.g, o.b);
                    if (M2 > 255) { double k = 255 / M2;o.r *= k;o.g *= k;o.b *= k; }
                }
            }
        }
        return o;
    }
    static HLS rgbToHls(const RGB& c) {
        double r = c.r / 255, g = c.g / 255, b = c.b / 255, M = mx3(r, g, b), N = mn3(r, g, b), h = 0, s = 0, l = (M + N) / 2;
        if (M != N) {
            double d = M - N;s = l > 0.5 ? d / (2 - M - N) : d / (M + N);
            if (M == r)h = (g - b) / d + (g < b ? 6 : 0);
            else if (M == g)h = (b - r) / d + 2;
            else h = (r - g) / d + 4;
            h /= 6;
        }
        HLS o;o.h = h * 360;o.l = l * 100;o.s = s * 100;return o;
    }
    static double h2r(double p, double q, double t) {
        if (t < 0)t += 1;if (t > 1)t -= 1;
        if (t < 1.0 / 6)return p + (q - p) * 6 * t;
        if (t < 1.0 / 2)return q;
        if (t < 2.0 / 3)return p + (q - p) * (2.0 / 3 - t) * 6;
        return p;
    }
    static RGB hlsToRgb(const HLS& c) {
        double h = c.h / 360, l = c.l / 100, s = c.s / 100, r, g, b;
        if (s == 0) { r = g = b = l; }
        else {
            double q = l < 0.5 ? l * (1 + s) : l + s - l * s, p = 2 * l - q;
            r = h2r(p, q, h + 1.0 / 3);g = h2r(p, q, h);b = h2r(p, q, h - 1.0 / 3);
        }
        RGB o;o.r = r * 255;o.g = g * 255;o.b = b * 255;return o;
    }
}
using namespace Model;

// view
#define ID_R 101
#define ID_G 102
#define ID_B 103
#define ID_X 111
#define ID_Y 112
#define ID_Z 113
#define ID_H 121
#define ID_L 122
#define ID_S 123
#define ID_STD 131
#define ID_GAM 132
#define ID_PREV 140
#define ID_WARN 141
#define ID_MAP_RGB 160
#define ID_MAP_XYZ 161
#define ID_MAP_HLS 162

static HWND hR, hG, hB, hX, hY, hZ, hH, hL, hS;
static HWND hRE, hGE, hBE, hXE, hYE, hZE, hHE, hLE, hSE;
static HWND hStd, hGam, hPrev, hWarn;
static HWND hMapRGB, hMapXYZ, hMapHLS;
static bool g_up = false;
static RGB g_rgb = { 255,0,0 };

static void SetI(HWND h, int v) { wchar_t b[16];swprintf(b, 16, L"%d", v);SetWindowTextW(h, b); }
static void SetD(HWND h, double v) { wchar_t b[16];swprintf(b, 16, L"%.2f", v);SetWindowTextW(h, b); }
static int GetI(HWND h) { wchar_t b[16];GetWindowTextW(h, b, 16);return _wtoi(b); }
static double GetD(HWND h) { wchar_t b[16];GetWindowTextW(h, b, 16);return _wtof(b); }

static void UpdateAll();

static LRESULT CALLBACK PrevProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_ERASEBKGND) return 1;
    if (m == WM_PAINT) {
        PAINTSTRUCT p;HDC d = BeginPaint(h, &p);RECT r;GetClientRect(h, &r);
        HBRUSH b = CreateSolidBrush(RGB((int)g_rgb.r, (int)g_rgb.g, (int)g_rgb.b));
        FillRect(d, &r, b);DeleteObject(b);
        FrameRect(d, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
        EndPaint(h, &p);return 0;
    }
    return DefWindowProc(h, m, w, l);
}

//карты 
#define MAP_W 220
#define MAP_H 220
static unsigned char* g_mapBuf[3] = { 0,0,0 };
static bool g_mapValid[3] = { false,false,false };
static int  g_lastB_for_rgb = -1;
static int  g_lastZ_for_xyz = -1;
static Standard g_cached_std_for_xyz = D65;
static GamutStrategy g_cached_gam_for_xyz = CLIP;

static void EnsureBuffers() {
    for (int i = 0;i < 3;i++) {
        if (!g_mapBuf[i]) g_mapBuf[i] = (unsigned char*)malloc(MAP_W * MAP_H * 3);
    }
}

static void BuildMap(int mode) {
    unsigned char* buf = g_mapBuf[mode];
    for (int py = 0; py < MAP_H; ++py) {
        for (int px = 0; px < MAP_W; ++px) {
            int r = 0, g = 0, b = 0;
            if (mode == 0) {
                r = px * 255 / (MAP_W - 1);
                g = (MAP_H - 1 - py) * 255 / (MAP_H - 1);
                b = (int)(g_rgb.b + 0.5);
            }
            else if (mode == 1) {
                XYZ xyz;
                xyz.x = px * 100.0 / (MAP_W - 1);
                xyz.y = (MAP_H - 1 - py) * 100.0 / (MAP_H - 1);
                xyz.z = rgbToXyz(g_rgb).z;
                RGB c = xyzToRgb(xyz);
                r = (int)(c.r + 0.5);g = (int)(c.g + 0.5);b = (int)(c.b + 0.5);
            }
            else {
                HLS hls;
                hls.h = px * 360.0 / (MAP_W - 1);
                hls.l = (MAP_H - 1 - py) * 100.0 / (MAP_H - 1);
                hls.s = 100.0;
                RGB c = hlsToRgb(hls);
                r = (int)(c.r + 0.5);g = (int)(c.g + 0.5);b = (int)(c.b + 0.5);
            }
            if (r < 0)r = 0;if (r > 255)r = 255;
            if (g < 0)g = 0;if (g > 255)g = 255;
            if (b < 0)b = 0;if (b > 255)b = 255;
            int idx = (py * MAP_W + px) * 3;
            buf[idx + 0] = (unsigned char)b;
            buf[idx + 1] = (unsigned char)g;
            buf[idx + 2] = (unsigned char)r;
        }
    }
    g_mapValid[mode] = true;
}

static void InvalidateMapIfNeeded(int mode) {
    if (mode == 0) {
        int curB = (int)(g_rgb.b + 0.5);
        if (!g_mapValid[0] || curB != g_lastB_for_rgb) {
            BuildMap(0);
            g_lastB_for_rgb = curB;
        }
    }
    else if (mode == 1) {
        int curZ = (int)(rgbToXyz(g_rgb).z + 0.5);
        if (!g_mapValid[1] || curZ != g_lastZ_for_xyz
            || g_cached_std_for_xyz != Model::g_standard
            || g_cached_gam_for_xyz != Model::g_strategy) {
            BuildMap(1);
            g_lastZ_for_xyz = curZ;
            g_cached_std_for_xyz = Model::g_standard;
            g_cached_gam_for_xyz = Model::g_strategy;
        }
    }
    else {
        if (!g_mapValid[2]) BuildMap(2);
    }
}

static void DrawMapFromBuffer(HDC hdc, int mode, int w, int h) {
    EnsureBuffers();
    InvalidateMapIfNeeded(mode);

    // Внутренняя область карты (с отступом 1 пиксель по каждой стороне — под рамку)
    int innerW = w - 2;
    int innerH = h - 2;
    if (innerW < 1) innerW = 1;
    if (innerH < 1) innerH = 1;

    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = MAP_W;
    bmi.bmiHeader.biHeight = MAP_H;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    // COLORONCOLOR — не сглаживать и не «тянуть» крайние пиксели
    int oldMode = SetStretchBltMode(hdc, COLORONCOLOR);
    StretchDIBits(hdc, 1, 1, innerW, innerH, 0, 0, MAP_W, MAP_H,
        g_mapBuf[mode], &bmi, DIB_RGB_COLORS, SRCCOPY);
    SetStretchBltMode(hdc, oldMode);

    // Позиция маркера во внутренних координатах окна (со сдвигом 1)
    int mx = 0, my = 0;
    if (mode == 0) {
        mx = 1 + (int)(g_rgb.r / 255.0 * (innerW - 1));
        my = 1 + innerH - 1 - (int)(g_rgb.g / 255.0 * (innerH - 1));
    }
    else if (mode == 1) {
        XYZ xyz = rgbToXyz(g_rgb);
        mx = 1 + (int)(xyz.x / 100.0 * (innerW - 1));
        my = 1 + innerH - 1 - (int)(xyz.y / 100.0 * (innerH - 1));
    }
    else {
        HLS hls = rgbToHls(g_rgb);
        mx = 1 + (int)(hls.h / 360.0 * (innerW - 1));
        my = 1 + innerH - 1 - (int)(hls.l / 100.0 * (innerH - 1));
    }

    // Границы внутренней области
    int left = 1, right = w - 2, top = 1, bottom = h - 2;
    if (mx < left)mx = left; if (mx > right)mx = right;
    if (my < top)my = top; if (my > bottom)my = bottom;

    int x1 = mx - 6; if (x1 < left) x1 = left;
    int x2 = mx + 6; if (x2 > right) x2 = right;
    int y1 = my - 6; if (y1 < top) y1 = top;
    int y2 = my + 6; if (y2 > bottom) y2 = bottom;

    HPEN pen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
    HPEN old = (HPEN)SelectObject(hdc, pen);
    MoveToEx(hdc, x1, my, 0); LineTo(hdc, x2, my);
    MoveToEx(hdc, mx, y1, 0); LineTo(hdc, mx, y2);
    SelectObject(hdc, old);DeleteObject(pen);
}

static void SyncFromMap() {
    SetI(hRE, (int)(g_rgb.r + 0.5));
    SetI(hGE, (int)(g_rgb.g + 0.5));
    SetI(hBE, (int)(g_rgb.b + 0.5));
    SendMessage(hR, TBM_SETPOS, TRUE, (LPARAM)(int)(g_rgb.r + 0.5));
    SendMessage(hG, TBM_SETPOS, TRUE, (LPARAM)(int)(g_rgb.g + 0.5));
    SendMessage(hB, TBM_SETPOS, TRUE, (LPARAM)(int)(g_rgb.b + 0.5));
}

static void ApplyMapPoint(int mode, int w, int h, int mx, int my) {
    if (mx < 0)mx = 0;if (mx > w - 1)mx = w - 1;
    if (my < 0)my = 0;if (my > h - 1)my = h - 1;
    if (mode == 0) {
        g_rgb.r = mx * 255.0 / (w - 1);
        g_rgb.g = (h - 1 - my) * 255.0 / (h - 1);
    }
    else if (mode == 1) {
        XYZ xyz;
        xyz.x = mx * 100.0 / (w - 1);
        xyz.y = (h - 1 - my) * 100.0 / (h - 1);
        XYZ cur = rgbToXyz(g_rgb);
        xyz.z = cur.z;
        g_rgb = xyzToRgb(xyz);
    }
    else {
        HLS hls;
        hls.h = mx * 360.0 / (w - 1);
        hls.l = (h - 1 - my) * 100.0 / (h - 1);
        hls.s = 100.0;
        g_rgb = hlsToRgb(hls);
    }
}

static LRESULT CALLBACK MapProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_ERASEBKGND) return 1;

    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;GetClientRect(hwnd, &rc);
        int mode = (int)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        DrawMapFromBuffer(hdc, mode, rc.right, rc.bottom);
        EndPaint(hwnd, &ps);
        return 0;
    }
    if (msg == WM_LBUTTONDOWN) {
        SetCapture(hwnd);
        int mode = (int)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        RECT rc;GetClientRect(hwnd, &rc);
        int iw = rc.right - 2, ih = rc.bottom - 2;
        int mx = LOWORD(lParam) - 1, my = HIWORD(lParam) - 1;
        ApplyMapPoint(mode, iw, ih, mx, my);
        SyncFromMap();
        UpdateAll();
        return 0;
    }
    if (msg == WM_MOUSEMOVE) {
        if (wParam & MK_LBUTTON) {
            int mode = (int)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            RECT rc;GetClientRect(hwnd, &rc);
            int iw = rc.right - 2, ih = rc.bottom - 2;
            int mx = LOWORD(lParam) - 1, my = HIWORD(lParam) - 1;
            ApplyMapPoint(mode, iw, ih, mx, my);
            SyncFromMap();
            UpdateAll();
        }
        return 0;
    }
    if (msg == WM_LBUTTONUP) {
        ReleaseCapture();
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void UpdateAll() {
    g_up = true;
    XYZ x = rgbToXyz(g_rgb);
    SendMessage(hX, TBM_SETPOS, TRUE, (LPARAM)(x.x * 10));
    SendMessage(hY, TBM_SETPOS, TRUE, (LPARAM)(x.y * 10));
    SendMessage(hZ, TBM_SETPOS, TRUE, (LPARAM)(x.z * 10));
    SetD(hXE, x.x);SetD(hYE, x.y);SetD(hZE, x.z);

    HLS s = rgbToHls(g_rgb);
    SendMessage(hH, TBM_SETPOS, TRUE, (LPARAM)(int)(s.h + 0.5));
    SendMessage(hL, TBM_SETPOS, TRUE, (LPARAM)(int)(s.l + 0.5));
    SendMessage(hS, TBM_SETPOS, TRUE, (LPARAM)(int)(s.s + 0.5));
    SetI(hHE, (int)(s.h + 0.5));SetI(hLE, (int)(s.l + 0.5));SetI(hSE, (int)(s.s + 0.5));

    RGB c = xyzToRgb(x);
    bool oob = fabs(c.r - g_rgb.r) > 1 || fabs(c.g - g_rgb.g) > 1 || fabs(c.b - g_rgb.b) > 1;
    ShowWindow(hWarn, oob ? SW_SHOW : SW_HIDE);

    InvalidateRect(hPrev, 0, TRUE);
    InvalidateRect(hMapRGB, 0, TRUE);
    InvalidateRect(hMapXYZ, 0, TRUE);
    InvalidateRect(hMapHLS, 0, TRUE);
    g_up = false;
}

static void OnRgb() { if (g_up)return;g_rgb.r = GetI(hRE);g_rgb.g = GetI(hGE);g_rgb.b = GetI(hBE);UpdateAll(); }
static void OnXyz() {
    if (g_up)return;
    XYZ x;x.x = GetD(hXE);x.y = GetD(hYE);x.z = GetD(hZE);
    g_rgb = xyzToRgb(x);
    g_up = true;
    SetI(hRE, (int)(g_rgb.r + 0.5));SetI(hGE, (int)(g_rgb.g + 0.5));SetI(hBE, (int)(g_rgb.b + 0.5));
    SendMessage(hR, TBM_SETPOS, TRUE, (LPARAM)(int)(g_rgb.r + 0.5));
    SendMessage(hG, TBM_SETPOS, TRUE, (LPARAM)(int)(g_rgb.g + 0.5));
    SendMessage(hB, TBM_SETPOS, TRUE, (LPARAM)(int)(g_rgb.b + 0.5));
    g_up = false;UpdateAll();
}
static void OnHls() {
    if (g_up)return;
    HLS h;h.h = GetD(hHE);h.l = GetD(hLE);h.s = GetD(hSE);
    g_rgb = hlsToRgb(h);
    g_up = true;
    SetI(hRE, (int)(g_rgb.r + 0.5));SetI(hGE, (int)(g_rgb.g + 0.5));SetI(hBE, (int)(g_rgb.b + 0.5));
    SendMessage(hR, TBM_SETPOS, TRUE, (LPARAM)(int)(g_rgb.r + 0.5));
    SendMessage(hG, TBM_SETPOS, TRUE, (LPARAM)(int)(g_rgb.g + 0.5));
    SendMessage(hB, TBM_SETPOS, TRUE, (LPARAM)(int)(g_rgb.b + 0.5));
    g_up = false;UpdateAll();
}

static void Row(HWND h, HINSTANCE I, int y, const wchar_t* n, int idR, int idE, int mx, HWND* outR, HWND* outE) {
    CreateWindowW(L"STATIC", n, WS_VISIBLE | WS_CHILD, 10, y, 20, 20, h, 0, I, 0);
    HWND r = CreateWindowW(L"msctls_trackbar32", 0,
        WS_VISIBLE | WS_CHILD | TBS_HORZ | TBS_NOTICKS,
        35, y, 320, 25, h, (HMENU)(INT_PTR)idR, I, 0);
    SendMessage(r, TBM_SETRANGE, TRUE, MAKELPARAM(0, mx));
    HWND e = CreateWindowW(L"EDIT", L"0",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_RIGHT | ES_NUMBER,
        365, y, 60, 22, h, (HMENU)(INT_PTR)idE, I, 0);
    *outR = r;*outE = e;
}

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE: {
        HINSTANCE I = ((LPCREATESTRUCT)l)->hInstance;
        int y = 8;

        CreateWindowW(L"STATIC", L"Стандарт:", WS_VISIBLE | WS_CHILD, 10, y, 70, 20, h, 0, I, 0);
        hStd = CreateWindowW(L"COMBOBOX", 0, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
            80, y, 90, 150, h, (HMENU)(INT_PTR)ID_STD, I, 0);
        SendMessage(hStd, CB_ADDSTRING, 0, (LPARAM)L"D65");
        SendMessage(hStd, CB_ADDSTRING, 0, (LPARAM)L"D50");
        SendMessage(hStd, CB_ADDSTRING, 0, (LPARAM)L"E");
        SendMessage(hStd, CB_SETCURSEL, 0, 0);

        CreateWindowW(L"STATIC", L"Gamut:", WS_VISIBLE | WS_CHILD, 180, y, 55, 20, h, 0, I, 0);
        hGam = CreateWindowW(L"COMBOBOX", 0, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
            235, y, 120, 150, h, (HMENU)(INT_PTR)ID_GAM, I, 0);
        SendMessage(hGam, CB_ADDSTRING, 0, (LPARAM)L"Clipping");
        SendMessage(hGam, CB_ADDSTRING, 0, (LPARAM)L"Scaling");
        SendMessage(hGam, CB_SETCURSEL, 0, 0);
        y += 32;

        hPrev = CreateWindowW(L"STATIC", 0, WS_VISIBLE | WS_CHILD, 10, y, 80, 80, h, (HMENU)(INT_PTR)ID_PREV, I, 0);
        SetWindowLongPtr(hPrev, GWLP_WNDPROC, (LONG_PTR)PrevProc);
        hWarn = CreateWindowW(L"STATIC", L"! Вне диапазона",
            WS_CHILD, 100, y + 30, 150, 20, h, (HMENU)(INT_PTR)ID_WARN, I, 0);
        y += 90;

        CreateWindowW(L"STATIC", L"RGB (X=R, Y=G)", WS_VISIBLE | WS_CHILD, 10, y, 220, 16, h, 0, I, 0);
        CreateWindowW(L"STATIC", L"XYZ (X=X, Y=Y)", WS_VISIBLE | WS_CHILD, 240, y, 220, 16, h, 0, I, 0);
        CreateWindowW(L"STATIC", L"HLS (X=H, Y=L)", WS_VISIBLE | WS_CHILD, 470, y, 220, 16, h, 0, I, 0);
        y += 18;

        hMapRGB = CreateWindowW(L"STATIC", 0, WS_VISIBLE | WS_CHILD | SS_NOTIFY,
            10, y, 220, 220, h, (HMENU)(INT_PTR)ID_MAP_RGB, I, 0);
        SetWindowLongPtr(hMapRGB, GWLP_USERDATA, 0);
        SetWindowLongPtr(hMapRGB, GWLP_WNDPROC, (LONG_PTR)MapProc);

        hMapXYZ = CreateWindowW(L"STATIC", 0, WS_VISIBLE | WS_CHILD | SS_NOTIFY,
            240, y, 220, 220, h, (HMENU)(INT_PTR)ID_MAP_XYZ, I, 0);
        SetWindowLongPtr(hMapXYZ, GWLP_USERDATA, 1);
        SetWindowLongPtr(hMapXYZ, GWLP_WNDPROC, (LONG_PTR)MapProc);

        hMapHLS = CreateWindowW(L"STATIC", 0, WS_VISIBLE | WS_CHILD | SS_NOTIFY,
            470, y, 220, 220, h, (HMENU)(INT_PTR)ID_MAP_HLS, I, 0);
        SetWindowLongPtr(hMapHLS, GWLP_USERDATA, 2);
        SetWindowLongPtr(hMapHLS, GWLP_WNDPROC, (LONG_PTR)MapProc);
        y += 228;

        Row(h, I, y, L"R:", ID_R, 104, 255, &hR, &hRE); y += 28;
        Row(h, I, y, L"G:", ID_G, 105, 255, &hG, &hGE); y += 28;
        Row(h, I, y, L"B:", ID_B, 106, 255, &hB, &hBE); y += 30;
        Row(h, I, y, L"X:", ID_X, 114, 1000, &hX, &hXE); y += 28;
        Row(h, I, y, L"Y:", ID_Y, 115, 1000, &hY, &hYE); y += 28;
        Row(h, I, y, L"Z:", ID_Z, 116, 1000, &hZ, &hZE); y += 30;
        Row(h, I, y, L"H:", ID_H, 124, 360, &hH, &hHE); y += 28;
        Row(h, I, y, L"L:", ID_L, 125, 100, &hL, &hLE); y += 28;
        Row(h, I, y, L"S:", ID_S, 126, 100, &hS, &hSE);

        SetI(hRE, 255);SetI(hGE, 0);SetI(hBE, 0);
        SendMessage(hR, TBM_SETPOS, TRUE, 255);
        UpdateAll();
        break;
    }

    case WM_HSCROLL: {
        HWND s = (HWND)l;if (!s)break;
        int id = GetDlgCtrlID(s);
        int p = (int)SendMessage(s, TBM_GETPOS, 0, 0);
        if (id == ID_R) { SetI(hRE, p);OnRgb(); }
        else if (id == ID_G) { SetI(hGE, p);OnRgb(); }
        else if (id == ID_B) { SetI(hBE, p);OnRgb(); }
        else if (id == ID_X) { SetD(hXE, p / 10.0);OnXyz(); }
        else if (id == ID_Y) { SetD(hYE, p / 10.0);OnXyz(); }
        else if (id == ID_Z) { SetD(hZE, p / 10.0);OnXyz(); }
        else if (id == ID_H) { SetI(hHE, p);OnHls(); }
        else if (id == ID_L) { SetI(hLE, p);OnHls(); }
        else if (id == ID_S) { SetI(hSE, p);OnHls(); }
        break;
    }

    case WM_COMMAND: {
        int id = LOWORD(w), c = HIWORD(w);
        if (id == ID_STD && c == CBN_SELCHANGE) {
            int s = (int)SendMessage(hStd, CB_GETCURSEL, 0, 0);
            Model::g_standard = (s == 0) ? D65 : (s == 1) ? D50 : E_STD;
            g_mapValid[1] = false;
            UpdateAll();
        }
        else if (id == ID_GAM && c == CBN_SELCHANGE) {
            int s = (int)SendMessage(hGam, CB_GETCURSEL, 0, 0);
            Model::g_strategy = (s == 0) ? CLIP : SCALE;
            g_mapValid[1] = false;
            UpdateAll();
        }
        else if (c == EN_KILLFOCUS) {
            if (id == 104 || id == 105 || id == 106) {
                SendMessage(hR, TBM_SETPOS, TRUE, GetI(hRE));
                SendMessage(hG, TBM_SETPOS, TRUE, GetI(hGE));
                SendMessage(hB, TBM_SETPOS, TRUE, GetI(hBE));
                OnRgb();
            }
            else if (id == 114 || id == 115 || id == 116) {
                SendMessage(hX, TBM_SETPOS, TRUE, (LPARAM)(GetD(hXE) * 10));
                SendMessage(hY, TBM_SETPOS, TRUE, (LPARAM)(GetD(hYE) * 10));
                SendMessage(hZ, TBM_SETPOS, TRUE, (LPARAM)(GetD(hZE) * 10));
                OnXyz();
            }
            else if (id == 124 || id == 125 || id == 126) {
                SendMessage(hH, TBM_SETPOS, TRUE, GetI(hHE));
                SendMessage(hL, TBM_SETPOS, TRUE, GetI(hLE));
                SendMessage(hS, TBM_SETPOS, TRUE, GetI(hSE));
                OnHls();
            }
        }
        break;
    }

    case WM_CTLCOLORSTATIC:
        if ((HWND)l == hWarn) {
            SetTextColor((HDC)w, RGB(200, 0, 0));
            SetBkMode((HDC)w, TRANSPARENT);
            return (LRESULT)GetStockObject(NULL_BRUSH);
        }
        break;

    case WM_DESTROY:
        for (int i = 0;i < 3;i++) if (g_mapBuf[i]) free(g_mapBuf[i]);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(h, m, w, l);
}

int WINAPI WinMain(HINSTANCE I, HINSTANCE, LPSTR, int n) {
    InitCommonControls();
    WNDCLASSW c = { 0 };
    c.lpfnWndProc = WndProc;
    c.hInstance = I;
    c.lpszClassName = L"Lab1";
    c.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    c.hCursor = LoadCursor(0, IDC_ARROW);
    RegisterClassW(&c);

    HWND h = CreateWindowW(L"Lab1",
        L"Лаба 1 (Вариант 12) — RGB/XYZ/HLS + карты",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 760, 1050,
        0, 0, I, 0);
    ShowWindow(h, n);
    UpdateWindow(h);

    MSG msg;
    while (GetMessage(&msg, 0, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}