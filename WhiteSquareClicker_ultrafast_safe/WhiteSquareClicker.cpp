#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

// WhiteSquareClicker UltraFast Safe
// F6: set scan area first corner at current mouse position
// F7: set scan area second corner at current mouse position
// F8: start / pause. It will NOT start until F6 and F7 are both set.
// F9: emergency pause
// ESC: exit

static const int WHITE_THRESHOLD = 245;
static const int MIN_SIZE = 12;
static const int MAX_SIZE = 400;
static const int SCAN_STEP = 60;          // 2/3 = more accurate, 4/5/6 = faster
static const int LOOP_DELAY_MS = 0;      // 0 = fastest
static const int CLICK_COOLDOWN_MS = 0;  // 0 = fastest
static const bool PRINT_CLICKS = false;

struct RectI {
    int left = 0;
    int top = 0;
    int width = 0;
    int height = 0;
};

struct Target {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int cells = 0;
    bool valid = false;
};

class ScreenCapture {
public:
    ScreenCapture() {
        screenDC = GetDC(nullptr);
        memDC = CreateCompatibleDC(screenDC);
    }

    ~ScreenCapture() {
        if (dib) {
            SelectObject(memDC, oldObj);
            DeleteObject(dib);
        }
        if (memDC) DeleteDC(memDC);
        if (screenDC) ReleaseDC(nullptr, screenDC);
    }

    bool resize(int w, int h) {
        if (w <= 0 || h <= 0) return false;
        if (w == width && h == height && bits) return true;

        if (dib) {
            SelectObject(memDC, oldObj);
            DeleteObject(dib);
            dib = nullptr;
            bits = nullptr;
            oldObj = nullptr;
        }

        BITMAPINFO bmi{};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = w;
        bmi.bmiHeader.biHeight = -h;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        dib = CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (!dib || !bits) return false;

        oldObj = SelectObject(memDC, dib);
        width = w;
        height = h;
        return true;
    }

    bool capture(const RectI& r) {
        if (!resize(r.width, r.height)) return false;
        return BitBlt(memDC, 0, 0, r.width, r.height, screenDC, r.left, r.top, SRCCOPY | CAPTUREBLT) != 0;
    }

    unsigned char* data() const { return static_cast<unsigned char*>(bits); }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    HDC screenDC = nullptr;
    HDC memDC = nullptr;
    HBITMAP dib = nullptr;
    HGDIOBJ oldObj = nullptr;
    void* bits = nullptr;
    int width = 0;
    int height = 0;
};

static bool isWhitePixel(const unsigned char* p) {
    // DIB 32-bit order is B, G, R, X
    return p[0] >= WHITE_THRESHOLD && p[1] >= WHITE_THRESHOLD && p[2] >= WHITE_THRESHOLD;
}

static Target findWhiteSquare(ScreenCapture& cap, const RectI& scanRect) {
    Target best;
    if (scanRect.width <= 0 || scanRect.height <= 0) return best;
    if (!cap.capture(scanRect)) return best;

    const int w = cap.getWidth();
    const int h = cap.getHeight();
    unsigned char* pixels = cap.data();

    const int gw = (w + SCAN_STEP - 1) / SCAN_STEP;
    const int gh = (h + SCAN_STEP - 1) / SCAN_STEP;
    const int total = gw * gh;

    static std::vector<unsigned char> white;
    static std::vector<unsigned char> visited;
    static std::vector<int> stack;
    white.assign(total, 0);
    visited.assign(total, 0);
    stack.clear();

    for (int gy = 0; gy < gh; ++gy) {
        int y = gy * SCAN_STEP;
        if (y >= h) y = h - 1;
        const unsigned char* row = pixels + y * w * 4;
        for (int gx = 0; gx < gw; ++gx) {
            int x = gx * SCAN_STEP;
            if (x >= w) x = w - 1;
            if (isWhitePixel(row + x * 4)) white[gy * gw + gx] = 1;
        }
    }

    const int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};

    for (int gy = 0; gy < gh; ++gy) {
        for (int gx = 0; gx < gw; ++gx) {
            int idx = gy * gw + gx;
            if (!white[idx] || visited[idx]) continue;

            int minX = gx, maxX = gx, minY = gy, maxY = gy;
            int count = 0;
            visited[idx] = 1;
            stack.push_back(idx);

            while (!stack.empty()) {
                int cur = stack.back();
                stack.pop_back();
                ++count;
                int cy = cur / gw;
                int cx = cur - cy * gw;
                if (cx < minX) minX = cx;
                if (cx > maxX) maxX = cx;
                if (cy < minY) minY = cy;
                if (cy > maxY) maxY = cy;

                for (const auto& d : dirs) {
                    int nx = cx + d[0];
                    int ny = cy + d[1];
                    if (nx < 0 || nx >= gw || ny < 0 || ny >= gh) continue;
                    int ni = ny * gw + nx;
                    if (white[ni] && !visited[ni]) {
                        visited[ni] = 1;
                        stack.push_back(ni);
                    }
                }
            }

            int bw = (maxX - minX + 1) * SCAN_STEP;
            int bh = (maxY - minY + 1) * SCAN_STEP;
            if (bw < MIN_SIZE || bh < MIN_SIZE || bw > MAX_SIZE || bh > MAX_SIZE) continue;

            double ratio = static_cast<double>(bw) / static_cast<double>(bh);
            if (ratio < 0.65 || ratio > 1.55) continue;

            double filled = static_cast<double>(count) / static_cast<double>((maxX - minX + 1) * (maxY - minY + 1));
            if (filled < 0.35) continue;

            if (!best.valid || count > best.cells) {
                best.valid = true;
                best.cells = count;
                best.x = scanRect.left + ((minX + maxX + 1) * SCAN_STEP) / 2;
                best.y = scanRect.top + ((minY + maxY + 1) * SCAN_STEP) / 2;
                best.w = bw;
                best.h = bh;
            }
        }
    }

    return best;
}

static void clickAt(int x, int y) {
    SetCursorPos(x, y);
    INPUT inputs[2]{};
    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(2, inputs, sizeof(INPUT));
}

static bool pressedOnce(int vk) {
    static bool prev[256]{};
    bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    bool hit = down && !prev[vk];
    prev[vk] = down;
    return hit;
}

static void normalizeRectFromPoints(POINT a, POINT b, RectI& out) {
    out.left = std::min(a.x, b.x);
    out.top = std::min(a.y, b.y);
    out.width = std::abs(a.x - b.x) + 1;
    out.height = std::abs(a.y - b.y) + 1;
}

int main() {
    SetProcessDPIAware();
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    POINT p1{0, 0};
    POINT p2{0, 0};
    bool haveP1 = false;
    bool haveP2 = false;
    bool regionReady = false;
    RectI scanRect{};

    std::cout << "WhiteSquareClicker UltraFast Safe\n";
    std::cout << "IMPORTANT: set scan area first. It will not run before F6 and F7.\n";
    std::cout << "F6 = set first corner at mouse\n";
    std::cout << "F7 = set second corner at mouse\n";
    std::cout << "F8 = start / pause\n";
    std::cout << "F9 = emergency pause\n";
    std::cout << "ESC = exit\n";

    bool running = false;
    ScreenCapture cap;
    DWORD lastClick = 0;

    while (true) {
        if (pressedOnce(VK_ESCAPE)) break;

        if (pressedOnce(VK_F9)) {
            running = false;
            std::cout << "Emergency paused\n";
        }

        if (pressedOnce(VK_F6)) {
            running = false;
            GetCursorPos(&p1);
            haveP1 = true;
            if (haveP2) {
                normalizeRectFromPoints(p1, p2, scanRect);
                regionReady = scanRect.width >= MIN_SIZE && scanRect.height >= MIN_SIZE;
            }
            std::cout << "Corner 1 set: " << p1.x << "," << p1.y << "\n";
        }

        if (pressedOnce(VK_F7)) {
            running = false;
            GetCursorPos(&p2);
            haveP2 = true;
            if (haveP1) {
                normalizeRectFromPoints(p1, p2, scanRect);
                regionReady = scanRect.width >= MIN_SIZE && scanRect.height >= MIN_SIZE;
            }
            std::cout << "Corner 2 set: " << p2.x << "," << p2.y << "\n";
            if (regionReady) {
                std::cout << "Scan area ready: " << scanRect.left << "," << scanRect.top
                          << " " << scanRect.width << "x" << scanRect.height << "\n";
            } else {
                std::cout << "Scan area is too small or not ready. Set F6 and F7 again.\n";
            }
        }

        if (pressedOnce(VK_F8)) {
            if (!regionReady) {
                running = false;
                std::cout << "Not started: set scan area with F6 and F7 first.\n";
            } else {
                running = !running;
                std::cout << (running ? "Running\n" : "Paused\n");
            }
        }

        if (running) {
            Target t = findWhiteSquare(cap, scanRect);
            DWORD now = GetTickCount();
            if (t.valid && now - lastClick >= static_cast<DWORD>(CLICK_COOLDOWN_MS)) {
                clickAt(t.x, t.y);
                lastClick = now;
                if (PRINT_CLICKS) {
                    std::cout << "Click " << t.x << "," << t.y << " size=" << t.w << "x" << t.h << "\n";
                }
            }
        }

        if (LOOP_DELAY_MS > 0) Sleep(LOOP_DELAY_MS);
        else Sleep(0);
    }

    std::cout << "Exit.\n";
    return 0;
}
