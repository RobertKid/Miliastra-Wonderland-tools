#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <algorithm>
#include <cmath>

static const int ROWS = 4;
static const int COLS = 4;
static const int TEMPLATE_W = 16;
static const int TEMPLATE_H = 24;

// Higher value includes more gray shadow. If OCR misses numbers, try 155 or 170.
static const int DARK_THRESHOLD = 145;

// If best digit score is higher than this, OCR is considered unsafe.
// If your font/scaling is slightly different and it refuses to click, try 320.
static const int MAX_OCR_SCORE = 280;

// Delay after every click. 3ms keeps the clicks fast but gives the game time to respond.
static const int CLICK_DELAY_MS = 0;

// Continuous detection interval while auto mode is ON. Lower = faster, higher = less CPU.
static const int DETECT_INTERVAL_MS = 1;

// Wait briefly after finishing one 1-16 round before scanning again.
static const int AFTER_ROUND_WAIT_MS = 80;

struct Point2D { int x; int y; };
struct Pixel { unsigned char b, g, r, a; };
struct ImageBGRA { int w = 0, h = 0; std::vector<Pixel> px; };
struct DigitTemplate { int digit; const char* bits; };
struct RecognizedCell { int value = -1; int score = 9999; };

static Point2D g_topLeft{0, 0};
static Point2D g_bottomRight{0, 0};
static bool g_hasTopLeft = false;
static bool g_hasBottomRight = false;

static const DigitTemplate DIGIT_TEMPLATES[] = {
    {0,
        "....########...."
        "...##########..."
        "..############.."
        "..#####...#####."
        ".#####....#####."
        ".#####.....####."
        "#####......#####"
        "#####......#####"
        "#####......#####"
        "#####......#####"
        "#####......#####"
        "#####......#####"
        "#####......#####"
        "#####......#####"
        "#####......#####"
        "#####......#####"
        "#####......#####"
        "#####......#####"
        ".#####.....####."
        ".#####....#####."
        "..####....#####."
        "..############.."
        "...##########..."
        "....########...."
    },
    {1,
        "............####"
        ".........#######"
        "......##########"
        "..##############"
        "################"
        "################"
        "......##########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......########."
        ".......######..."
    },
    {1,
        "............####"
        ".........#######"
        "......##########"
        "..##############"
        "################"
        "..##############"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......########."
        ".......######..."
    },
    {1,
        "............####"
        ".........#######"
        "......##########"
        "..##############"
        "################"
        "...#############"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......########."
        ".......######..."
    },
    {1,
        "............####"
        ".........#######"
        ".....###########"
        "..##############"
        "################"
        "...#############"
        "......##########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#######.."
        ".......######..."
    },
    {1,
        "............####"
        ".........#######"
        "......##########"
        "..##############"
        "################"
        "...#############"
        ".......#########"
        ".......#########"
        "........########"
        "........########"
        "........########"
        "........########"
        "........########"
        "........########"
        "........########"
        "........########"
        "........########"
        "........########"
        "........########"
        "........########"
        "........########"
        "........########"
        "........#######."
        "........#####..."
    },
    {1,
        "............####"
        ".........#######"
        "......##########"
        "...#############"
        "################"
        "...#############"
        ".......#########"
        ".......#########"
        "........########"
        "........########"
        "........########"
        "........########"
        "........########"
        "........########"
        "........########"
        "........########"
        "........########"
        "........########"
        "........########"
        "........########"
        "........########"
        "........########"
        "........#######."
        "........#####..."
    },
    {1,
        "............####"
        ".........#######"
        "......##########"
        "..##############"
        "################"
        "...#############"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......#########"
        ".......########."
        "........#####..."
    },
    {2,
        "....########...."
        "...##########..."
        "..############.."
        ".#####...######."
        ".####.....#####."
        "#####.....######"
        "#####......#####"
        "#####......#####"
        "#####.....######"
        "..........#####."
        ".........######."
        "........######.."
        ".......######..."
        "......######...."
        ".....######....."
        "...#######......"
        "..######........"
        "..#####........."
        ".######........."
        "#######........#"
        "################"
        "################"
        "################"
        "################"
    },
    {2,
        "....########...."
        "...###########.."
        "..#############."
        ".#####....#####."
        ".####.....######"
        "#####......#####"
        "#####......#####"
        "#####......#####"
        "#####.....######"
        "..........#####."
        ".........######."
        "........######.."
        ".......#######.."
        "......#######..."
        ".....######....."
        "...#######......"
        "..######........"
        "..#####........."
        ".######........."
        "#######........#"
        "################"
        "################"
        "################"
        "################"
    },
    {3,
        "...#########...."
        "..###########..."
        ".#############.."
        ".######.#######."
        "#####.....#####."
        "#####.....######"
        "####.......#####"
        "####......#####."
        "..........#####."
        ".........######."
        ".....#########.."
        "....#########..."
        ".....##########."
        ".........######."
        "..........######"
        "...........#####"
        "####.......#####"
        "####.......#####"
        "####.......#####"
        "#####.....######"
        "#######.########"
        ".##############."
        "..############.."
        "...##########..."
    },
    {4,
        "........####...."
        ".......######..."
        ".......######..."
        "......#######..."
        "......#######..."
        ".....########..."
        ".....########..."
        "....###.#####..."
        "....###.#####..."
        "...###..#####..."
        "...###..#####..."
        "..###...#####..."
        "..###...#####..."
        ".###....#####..."
        ".###############"
        "################"
        "################"
        "################"
        ".###############"
        "........#####..."
        "........#####..."
        "........####...."
        "........####...."
        "........###....."
    },
    {4,
        "........####...."
        ".......######..."
        ".......######..."
        "......#######..."
        "......#######..."
        ".....########..."
        ".....########..."
        "....###.#####..."
        "....###.#####..."
        "...###..#####..."
        "...###..#####..."
        "..###...#####..."
        "..###...#####..."
        ".###....#####..."
        ".###############"
        "################"
        "################"
        "################"
        "........#####..."
        "........#####..."
        "........#####..."
        "........####...."
        "........####...."
        "........###....."
    },
    {5,
        "...#############"
        "...############."
        "..#############."
        "..############.."
        "..##########...."
        "..####.........."
        "..####.........."
        "..####.........."
        ".#####.###......"
        ".############..."
        ".#############.."
        ".##############."
        ".###......######"
        "..........######"
        "...........#####"
        "...........#####"
        "####.......#####"
        "####.......#####"
        "####.......#####"
        "#####.....######"
        ".#####...######."
        ".##############."
        "..############.."
        "...#########...."
    },
    {5,
        "...############."
        "...############."
        "..#############."
        "..############.."
        "..##########...."
        "..####.........."
        "..####.........."
        "..####.........."
        ".#####.........."
        ".############..."
        ".#############.."
        ".##############."
        ".###.....#######"
        "..........######"
        "...........#####"
        "...........#####"
        "####.......#####"
        "####.......#####"
        "####.......#####"
        "#####.....######"
        ".#####...######."
        ".##############."
        "..############.."
        "...#########...."
    },
    {6,
        ".........###...."
        ".......#####...."
        "......#####....."
        ".....#####......"
        "....#####......."
        "...#####........"
        "..#####........."
        ".######........."
        ".#####.........."
        ".############..."
        "##############.."
        "###############."
        "######....######"
        "#####......#####"
        "#####......#####"
        "#####......#####"
        "#####......#####"
        "#####......#####"
        "#####......#####"
        ".#####.....#####"
        ".#####....######"
        "..#############."
        "..############.."
        "....#########..."
    },
    {6,
        ".........###...."
        ".......#####...."
        "......#####....."
        ".....#####......"
        "....#####......."
        "...#####........"
        "..#####........."
        "..#####........."
        ".#####.........."
        ".############..."
        "##############.."
        "###############."
        "######....######"
        "#####......#####"
        "#####......#####"
        "#####......#####"
        "#####......#####"
        "#####......#####"
        "#####......#####"
        ".#####.....#####"
        ".#####....######"
        "..#############."
        "..############.."
        "....#########..."
    },
    {7,
        "################"
        "################"
        "###############."
        "###############."
        "##.........###.."
        "..........####.."
        ".........####..."
        "........#####..."
        "........#####..."
        ".......#####...."
        ".......#####...."
        "......######...."
        "......#####....."
        "......#####....."
        ".....######....."
        ".....#####......"
        ".....#####......"
        ".....#####......"
        "....######......"
        "....######......"
        "....######......"
        "....#####......."
        "....#####......."
        "....####........"
    },
    {8,
        "....########...."
        "...###########.."
        "..############.."
        ".######...#####."
        ".#####.....####."
        ".#####.....#####"
        ".#####.....#####"
        ".#####.....####."
        ".######...#####."
        ".##############."
        "..############.."
        "...##########..."
        "..############.."
        ".##############."
        ".#####...#######"
        "#####......#####"
        "#####......#####"
        "#####.......####"
        "#####.......####"
        "#####......#####"
        "######.....#####"
        ".##############."
        "..############.."
        "...##########..."
    },
    {9,
        "....########...."
        "..############.."
        ".#############.."
        ".#####....#####."
        "######....######"
        "#####......#####"
        "#####......#####"
        "#####......#####"
        "#####......#####"
        "#####......#####"
        "#####......#####"
        ".#####....######"
        ".###############"
        "..##############"
        "...############."
        "..........#####."
        "..........#####."
        ".........#####.."
        "........#####..."
        "........#####..."
        ".......#####...."
        "......#####....."
        "....######......"
        "....####........"
    },
};

bool IsPressed(int vk) {
    return (GetAsyncKeyState(vk) & 1) != 0;
}

bool IsDarkPixel(const Pixel& p) {
    int gray = (299 * p.r + 587 * p.g + 114 * p.b) / 1000;
    return gray < DARK_THRESHOLD && p.r < 190 && p.g < 190 && p.b < 190;
}

bool CaptureScreenRect(int left, int top, int width, int height, ImageBGRA& out) {
    if (width <= 0 || height <= 0) return false;

    HDC screenDC = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(screenDC);
    HBITMAP bmp = CreateCompatibleBitmap(screenDC, width, height);
    if (!bmp) {
        DeleteDC(memDC);
        ReleaseDC(nullptr, screenDC);
        return false;
    }

    HGDIOBJ oldObj = SelectObject(memDC, bmp);
    BOOL ok = BitBlt(memDC, 0, 0, width, height, screenDC, left, top, SRCCOPY | CAPTUREBLT);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // top-down image
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    out.w = width;
    out.h = height;
    out.px.assign(width * height, Pixel{});

    int got = 0;
    if (ok) {
        got = GetDIBits(memDC, bmp, 0, height, out.px.data(), &bmi, DIB_RGB_COLORS);
    }

    SelectObject(memDC, oldObj);
    DeleteObject(bmp);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);

    return ok && got == height;
}

void LeftClick(int x, int y) {
    SetCursorPos(x, y);
    Sleep(1);

    INPUT inputs[2] = {};
    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(2, inputs, sizeof(INPUT));
}

std::vector<unsigned char> NormalizeDigit(const std::vector<unsigned char>& mask, int mw, int mh, int x1, int x2) {
    int y1 = mh, y2 = -1;
    for (int y = 0; y < mh; ++y) {
        for (int x = x1; x <= x2; ++x) {
            if (mask[y * mw + x]) {
                y1 = std::min(y1, y);
                y2 = std::max(y2, y);
            }
        }
    }

    if (y2 < y1 || x2 < x1) {
        return std::vector<unsigned char>(TEMPLATE_W * TEMPLATE_H, 0);
    }

    int sw = x2 - x1 + 1;
    int sh = y2 - y1 + 1;
    std::vector<unsigned char> out(TEMPLATE_W * TEMPLATE_H, 0);

    for (int ty = 0; ty < TEMPLATE_H; ++ty) {
        int sy0 = y1 + (ty * sh) / TEMPLATE_H;
        int sy1 = y1 + ((ty + 1) * sh) / TEMPLATE_H;
        if (sy1 <= sy0) sy1 = sy0 + 1;
        sy1 = std::min(sy1, y2 + 1);

        for (int tx = 0; tx < TEMPLATE_W; ++tx) {
            int sx0 = x1 + (tx * sw) / TEMPLATE_W;
            int sx1 = x1 + ((tx + 1) * sw) / TEMPLATE_W;
            if (sx1 <= sx0) sx1 = sx0 + 1;
            sx1 = std::min(sx1, x2 + 1);

            int total = 0;
            int dark = 0;
            for (int sy = sy0; sy < sy1; ++sy) {
                for (int sx = sx0; sx < sx1; ++sx) {
                    ++total;
                    if (mask[sy * mw + sx]) ++dark;
                }
            }
            out[ty * TEMPLATE_W + tx] = (total > 0 && dark * 100 >= total * 15) ? 1 : 0;
        }
    }

    return out;
}

int CompareToTemplate(const std::vector<unsigned char>& norm, const char* bits) {
    int xors = 0;
    int uni = 0;
    int n = TEMPLATE_W * TEMPLATE_H;
    for (int i = 0; i < n; ++i) {
        int a = norm[i] ? 1 : 0;
        int b = (bits[i] == '#') ? 1 : 0;
        if (a != b) ++xors;
        if (a || b) ++uni;
    }
    if (uni == 0) return 9999;
    return (xors * 1000) / uni;
}

std::pair<int, int> RecognizeOneDigit(const std::vector<unsigned char>& norm) {
    int bestDigit = -1;
    int bestScore = 9999;
    for (const auto& t : DIGIT_TEMPLATES) {
        int score = CompareToTemplate(norm, t.bits);
        if (score < bestScore) {
            bestScore = score;
            bestDigit = t.digit;
        }
    }
    return {bestDigit, bestScore};
}

std::vector<std::pair<int, int>> FindDigitRuns(const std::vector<unsigned char>& mask, int mw, int mh) {
    std::vector<int> col(mw, 0);
    for (int y = 0; y < mh; ++y) {
        for (int x = 0; x < mw; ++x) {
            if (mask[y * mw + x]) ++col[x];
        }
    }

    std::vector<std::pair<int, int>> runs;
    bool inRun = false;
    int start = 0;
    int activeThreshold = std::max(1, mh / 150);

    for (int x = 0; x < mw; ++x) {
        bool active = col[x] > activeThreshold;
        if (active && !inRun) {
            start = x;
            inRun = true;
        } else if (!active && inRun) {
            runs.push_back({start, x - 1});
            inRun = false;
        }
    }
    if (inRun) runs.push_back({start, mw - 1});

    int mergeGap = std::max(2, mw / 40);
    int minWidth = std::max(2, mw / 50);
    std::vector<std::pair<int, int>> merged;
    for (auto r : runs) {
        if (!merged.empty() && r.first - merged.back().second - 1 <= mergeGap) {
            merged.back().second = r.second;
        } else {
            merged.push_back(r);
        }
    }

    std::vector<std::pair<int, int>> filtered;
    for (auto r : merged) {
        if (r.second - r.first + 1 >= minWidth) filtered.push_back(r);
    }

    // The game only contains 1..16, so valid cells have one or two digits.
    // If tiny noise caused extra runs, keep the two widest runs.
    if (filtered.size() > 2) {
        std::sort(filtered.begin(), filtered.end(), [](auto a, auto b) {
            return (a.second - a.first) > (b.second - b.first);
        });
        filtered.resize(2);
        std::sort(filtered.begin(), filtered.end());
    }

    return filtered;
}

RecognizedCell RecognizeCell(const ImageBGRA& img, int x0, int y0, int x1, int y1) {
    x0 = std::max(0, std::min(x0, img.w - 1));
    x1 = std::max(0, std::min(x1, img.w - 1));
    y0 = std::max(0, std::min(y0, img.h - 1));
    y1 = std::max(0, std::min(y1, img.h - 1));
    if (x1 < x0) std::swap(x0, x1);
    if (y1 < y0) std::swap(y0, y1);

    // Ignore a little border so rounded white card edges do not matter.
    int cw = x1 - x0 + 1;
    int ch = y1 - y0 + 1;
    int mx = std::max(2, cw / 12);
    int my = std::max(2, ch / 12);
    x0 += mx; x1 -= mx; y0 += my; y1 -= my;

    int bx0 = img.w, by0 = img.h, bx1 = -1, by1 = -1;
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            const Pixel& p = img.px[y * img.w + x];
            if (IsDarkPixel(p)) {
                bx0 = std::min(bx0, x);
                bx1 = std::max(bx1, x);
                by0 = std::min(by0, y);
                by1 = std::max(by1, y);
            }
        }
    }

    if (bx1 < bx0 || by1 < by0) return {-1, 9999};

    int mw = bx1 - bx0 + 1;
    int mh = by1 - by0 + 1;
    std::vector<unsigned char> mask(mw * mh, 0);
    for (int y = 0; y < mh; ++y) {
        for (int x = 0; x < mw; ++x) {
            const Pixel& p = img.px[(by0 + y) * img.w + (bx0 + x)];
            mask[y * mw + x] = IsDarkPixel(p) ? 1 : 0;
        }
    }

    auto runs = FindDigitRuns(mask, mw, mh);
    if (runs.empty() || runs.size() > 2) return {-1, 9999};

    int value = 0;
    int worstScore = 0;
    for (auto r : runs) {
        auto norm = NormalizeDigit(mask, mw, mh, r.first, r.second);
        auto [digit, score] = RecognizeOneDigit(norm);
        if (digit < 0) return {-1, 9999};
        value = value * 10 + digit;
        worstScore = std::max(worstScore, score);
    }

    return {value, worstScore};
}

bool DetectBoard(std::array<std::array<RecognizedCell, COLS>, ROWS>& board,
                 std::array<Point2D, 17>& centers,
                 bool verbose = true) {
    if (!g_hasTopLeft || !g_hasBottomRight) {
        if (verbose) std::cout << "Set region first: move mouse to grid top-left and press F6, then bottom-right and press F7.\n";
        return false;
    }

    int left = std::min(g_topLeft.x, g_bottomRight.x);
    int right = std::max(g_topLeft.x, g_bottomRight.x);
    int top = std::min(g_topLeft.y, g_bottomRight.y);
    int bottom = std::max(g_topLeft.y, g_bottomRight.y);
    int width = right - left + 1;
    int height = bottom - top + 1;

    ImageBGRA img;
    if (!CaptureScreenRect(left, top, width, height, img)) {
        if (verbose) std::cout << "Screen capture failed. Try running as administrator.\n";
        return false;
    }

    double cellW = static_cast<double>(width) / COLS;
    double cellH = static_cast<double>(height) / ROWS;

    std::array<int, 17> counts{};
    centers = {};

    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            int x0 = static_cast<int>(std::round(c * cellW));
            int x1 = static_cast<int>(std::round((c + 1) * cellW)) - 1;
            int y0 = static_cast<int>(std::round(r * cellH));
            int y1 = static_cast<int>(std::round((r + 1) * cellH)) - 1;
            board[r][c] = RecognizeCell(img, x0, y0, x1, y1);

            int screenCx = static_cast<int>(left + (c + 0.5) * cellW);
            int screenCy = static_cast<int>(top + (r + 0.5) * cellH);
            int v = board[r][c].value;
            if (v >= 1 && v <= 16) {
                counts[v]++;
                centers[v] = {screenCx, screenCy};
            }
        }
    }

    if (verbose) {
        std::cout << "Detected grid:\n";
        for (int r = 0; r < ROWS; ++r) {
            for (int c = 0; c < COLS; ++c) {
                std::cout << board[r][c].value << "(" << board[r][c].score << ")\t";
            }
            std::cout << "\n";
        }
    }

    bool ok = true;
    for (int n = 1; n <= 16; ++n) {
        if (counts[n] != 1) ok = false;
    }
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            int v = board[r][c].value;
            if (v < 1 || v > 16 || board[r][c].score > MAX_OCR_SCORE) ok = false;
        }
    }

    if (!ok) {
        if (verbose) {
            std::cout << "OCR result is not safe. No clicks made.\n";
            std::cout << "Tips: set F6/F7 closer to the 4x4 board edges, or try DARK_THRESHOLD=155, MAX_OCR_SCORE=320.\n";
        }
        return false;
    }

    return true;
}

std::string BoardSignature(const std::array<std::array<RecognizedCell, COLS>, ROWS>& board) {
    std::string s;
    s.reserve(64);
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            s += std::to_string(board[r][c].value);
            s += ',';
        }
    }
    return s;
}

bool ClickSequence(const std::array<Point2D, 17>& centers, bool verbose = true) {
    if (verbose) std::cout << "Clicking 1 to 16. Press F9 to emergency stop.\n";
    for (int n = 1; n <= 16; ++n) {
        if (GetAsyncKeyState(VK_F9) & 0x8000) {
            if (verbose) std::cout << "Stopped by F9.\n";
            return false;
        }
        LeftClick(centers[n].x, centers[n].y);
        if (CLICK_DELAY_MS > 0) Sleep(CLICK_DELAY_MS);
    }
    return true;
}

bool ClickOneRound() {
    std::array<std::array<RecognizedCell, COLS>, ROWS> board{};
    std::array<Point2D, 17> centers{};

    if (!DetectBoard(board, centers, true)) return false;
    bool ok = ClickSequence(centers, true);
    if (ok) std::cout << "Done. Press F8 to start/stop continuous detection.\n";
    return ok;
}

int main() {
    SetProcessDPIAware();

    std::cout << "RandomNumberSequenceClicker continuous OCR version\n";
    std::cout << "Hotkeys:\n";
    std::cout << "F6 = set 4x4 board top-left\n";
    std::cout << "F7 = set 4x4 board bottom-right\n";
    std::cout << "F8 = start/stop continuous detection and clicking\n";
    std::cout << "F9 = emergency pause/stop\n";
    std::cout << "F10 = click one round once\n";
    std::cout << "ESC = exit\n\n";
    std::cout << "Use F6/F7 to frame only the 4x4 white cards, then press F8.\n";

    bool autoMode = false;
    std::string lastClickedSignature;
    int missCount = 0;
    ULONGLONG lastDetectTime = 0;
    ULONGLONG lastRoundTime = 0;

    while (true) {
        if (IsPressed(VK_ESCAPE)) {
            std::cout << "Exit.\n";
            break;
        }

        if (IsPressed(VK_F6)) {
            POINT p; GetCursorPos(&p);
            g_topLeft = {p.x, p.y};
            g_hasTopLeft = true;
            lastClickedSignature.clear();
            std::cout << "Top-left set to (" << p.x << ", " << p.y << ")\n";
        }

        if (IsPressed(VK_F7)) {
            POINT p; GetCursorPos(&p);
            g_bottomRight = {p.x, p.y};
            g_hasBottomRight = true;
            lastClickedSignature.clear();
            std::cout << "Bottom-right set to (" << p.x << ", " << p.y << ")\n";
        }

        if (IsPressed(VK_F8)) {
            if (!g_hasTopLeft || !g_hasBottomRight) {
                std::cout << "Set region first with F6 and F7.\n";
            } else {
                autoMode = !autoMode;
                lastClickedSignature.clear();
                missCount = 0;
                std::cout << (autoMode ? "Continuous detection ON.\n" : "Continuous detection OFF.\n");
            }
        }

        if (IsPressed(VK_F9)) {
            autoMode = false;
            std::cout << "Paused by F9.\n";
        }

        if (IsPressed(VK_F10)) {
            ClickOneRound();
        }

        if (autoMode) {
            ULONGLONG now = GetTickCount64();
            if (now - lastDetectTime >= DETECT_INTERVAL_MS && now - lastRoundTime >= AFTER_ROUND_WAIT_MS) {
                lastDetectTime = now;

                std::array<std::array<RecognizedCell, COLS>, ROWS> board{};
                std::array<Point2D, 17> centers{};
                if (DetectBoard(board, centers, false)) {
                    missCount = 0;
                    std::string sig = BoardSignature(board);
                    if (sig != lastClickedSignature) {
                        std::cout << "New valid board detected. ";
                        lastClickedSignature = sig;
                        if (!ClickSequence(centers, true)) {
                            autoMode = false;
                            std::cout << "Continuous detection OFF.\n";
                        }
                        lastRoundTime = GetTickCount64();
                    }
                } else {
                    ++missCount;
                    if (missCount >= 5) {
                        lastClickedSignature.clear();
                    }
                }
            }
        }

        Sleep(1);
    }

    return 0;
}
