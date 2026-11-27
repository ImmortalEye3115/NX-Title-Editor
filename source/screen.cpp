// NX Title Editor
// Copyright (C) 2026 ImmortalEye
// SPDX-License-Identifier: GPL-3.0-or-later

#include "screen.hpp"

#include <switch.h>

#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <deque>
#include <sys/stat.h>
#include <string>

#include "qrcodegen.h"

namespace screen {
namespace {

// 80x45 cells of 16x16 px. Rows/cols below are 1-based (ANSI).
//
// The console font is heavy and its line-drawing characters don't join up,
// so the layout uses solid colour bars and spacing instead of ASCII lines,
// and text sits on every other row (back-to-back rows touch in this font).
constexpr int kCols = 80;
constexpr int kHeaderRows = 3;   // bar: rows 1-3, text on row 2
constexpr int kTopRow = 6;       // QR code and the right column start here
constexpr int kFooterTop = 43;   // bar: rows 43-45, keys on row 44
constexpr int kMaxLog = 24;

// 24-bit colours matching the web page's dark mode
const char* kText = "\x1b[38;2;231;230;226m";
const char* kMuted = "\x1b[38;2;130;132;136m";
const char* kAccent = "\x1b[38;2;0;255;199m";   // mint, as on the web page
const char* kOk = "\x1b[38;2;0;255;199m";       // same mint: one green on screen
const char* kErr = "\x1b[38;2;226;105;91m";
const char* kBarBg = "\x1b[48;2;30;32;36m";
const char* kBandBg = "\x1b[48;2;44;47;52m";   // table header band
const char* kChipBg = "\x1b[48;2;0;255;199m";   // footer key chips, progress bar
const char* kChipFg = "\x1b[38;2;0;0;0m";
const char* kRedBg = "\x1b[48;2;226;105;91m";
const char* kRedFg = "\x1b[38;2;26;10;8m";
const char* kReset = "\x1b[0m";

std::string g_version;
std::string g_status = "Starting";
bool g_busy = true;
int g_net = -1;  // -1 not checked yet, 0 down, 1 connected
int g_progDone = -1, g_progTotal = 0;  // scan progress (-1 = show g_games text)
std::string g_games;
std::string g_url, g_note;
std::string g_err1, g_err2;
int g_infoCol = 3;  // left edge of the right-hand column (moves right of the QR)
int g_qrCells = 0;  // QR size in cells (0 = no QR)
int g_spin = 0;
u64 g_lastSpinNs = 0;
Mode g_mode = Mode::Normal;
unsigned g_logCount = 0;

struct LogLine {
    std::string time;
    LogKind kind;
    std::string action, title, item;
    bool spans;  // message row: title holds the message, no Item column
};
std::deque<LogLine> g_log;

void at(int row, int col) { printf("\x1b[%d;%dH", row, col); }

// Fill `width` cells from (row, col) with a background colour (or plain black).
void fill(int row, int col, int width, const char* bg = nullptr) {
    if (width <= 0) return;
    at(row, col);
    printf("%s%s%*s%s", kReset, bg ? bg : "", width, "", kReset);
}
void blank(int row, int col, int width) { fill(row, col, width); }

std::string clip(const std::string& s, int width) {
    if (width < 4) width = 4;
    if ((int)s.size() <= width) return s;
    return s.substr(0, width - 3) + "...";
}

// Right column runs to column 79, leaving a one-cell margin.
int textWidth() { return kCols - g_infoCol; }

u64 nowNs() { return armTicksToNs(armGetSystemTick()); }

std::string clockNow() {
    TimeCalendarTime cal{};
    TimeCalendarAdditionalInfo info{};
    time_t t = time(nullptr);
    char buf[8];
    if (R_SUCCEEDED(timeToCalendarTimeWithMyRule((u64)t, &cal, &info)))
        snprintf(buf, sizeof(buf), "%02d:%02d", cal.hour, cal.minute);
    else
        snprintf(buf, sizeof(buf), "--:--");
    return buf;
}

// ---- Log file: sdmc:/config/nx-title-editor/log.txt ----
// The full text of every row goes here (the screen clips long messages).
// On start, the previous log becomes log_old.txt and the one before that is
// deleted, so at most two runs are kept.
const char* kLogDir = "sdmc:/config/nx-title-editor";
const char* kLogPath = "sdmc:/config/nx-title-editor/log.txt";
const char* kOldLogPath = "sdmc:/config/nx-title-editor/log_old.txt";

std::string stampNow() {
    TimeCalendarTime cal{};
    TimeCalendarAdditionalInfo info{};
    char buf[32];
    if (R_SUCCEEDED(timeToCalendarTimeWithMyRule((u64)time(nullptr), &cal, &info)))
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d", cal.year, cal.month, cal.day, cal.hour,
                 cal.minute, cal.second);
    else
        snprintf(buf, sizeof(buf), "----------  --:--:--");
    return buf;
}

std::string clockSecs() {
    std::string s = stampNow();  // "YYYY-MM-DD HH:MM:SS"
    return s.substr(s.size() - 8);
}

void fileWrite(const std::string& text) {
    FILE* f = fopen(kLogPath, "a");
    if (!f) return;
    fputs(text.c_str(), f);
    fclose(f);
}

// "[13:33:05] [Wrote] text"
void fileLine(const std::string& action, const std::string& text) {
    std::string a = action;
    if (!a.empty()) a[0] = (char)toupper((unsigned char)a[0]);
    fileWrite("[" + clockSecs() + "] [" + a + "] " + text + "\n");
}

// The Item column written out: Home -> "Home Screen icon" and so on.
void fileItem(const std::string& action, const std::string& item, const std::string& title, const std::string& id) {
    std::string who = id.empty() ? title : title + " (" + id + ")";
    if (item == "Both") {
        fileLine(action, "[Home Screen icon] " + who);
        fileLine(action, "[All Software icon] " + who);
        return;
    }
    std::string what = item == "Home" ? "Home Screen icon" : item == "Apps" ? "All Software icon"
                     : item == "Info" ? "Details" : item;
    fileLine(action, "[" + what + "] " + who);
}

void startLogFile(const std::string& version) {
    mkdir("sdmc:/config", 0777);
    mkdir(kLogDir, 0777);
    remove(kOldLogPath);
    rename(kLogPath, kOldLogPath);
    fileWrite("NX Title Editor " + version + "\nLog started " + stampNow() + "\n\n");
}

const char spinnerChars[] = {'|', '/', '-', '\\'};

void drawHeader() {
    for (int r = 1; r <= kHeaderRows; r++) fill(r, 1, kCols, kBarBg);
    at(2, 3);
    printf("%s%sNX Title Editor %s%s%s", kBarBg, kAccent, kMuted, g_version.c_str(), kReset);
}

void drawHeaderStatus() {
    // Right-aligned on the bar: "<status>  <spinner>" ending at column 78
    std::string st = clip(g_status, 46);
    fill(2, kCols - 50, 50, kBarBg);
    at(2, kCols - 2 - (int)st.size() - 2);
    printf("%s%s%s  %s%c%s", kBarBg, g_busy ? kAccent : kText, st.c_str(), kText, spinnerChars[g_spin], kReset);
}

void drawSpinnerOnly() {
    at(2, kCols - 2);
    printf("%s%s%c%s", kBarBg, kText, spinnerChars[g_spin], kReset);
}

// Footer keys as orange chips: " A " Full rescan   " - " Reboot   " + " Exit
void drawKeys(const char* const* keys, const char* const* labels, int n, int startCol) {
    int col = startCol;
    for (int i = 0; i < n; i++) {
        at(kFooterTop + 1, col);
        printf("%s%s %s %s%s %s%s", kChipBg, kChipFg, keys[i], kBarBg, kText, labels[i], kReset);
        col += 3 + 1 + (int)strlen(labels[i]) + 4;
    }
}

void drawFooter() {
    for (int r = kFooterTop; r <= kFooterTop + 2; r++) fill(r, 1, kCols, kBarBg);
    static const char* const keys[] = {"A", "-", "+"};
    static const char* const labels[] = {"Full rescan", "Reboot", "Exit"};
    drawKeys(keys, labels, 3, 3);
}

void drawQr(const std::string& url) {
    static uint8_t qr[qrcodegen_BUFFER_LEN_FOR_VERSION(8)];
    static uint8_t tmp[qrcodegen_BUFFER_LEN_FOR_VERSION(8)];
    g_qrCells = 0;
    if (!qrcodegen_encodeText(url.c_str(), tmp, qr, qrcodegen_Ecc_LOW, 1, 8, qrcodegen_Mask_AUTO, true)) {
        g_infoCol = 3;
        return;
    }
    int n = qrcodegen_getSize(qr);
    const int quiet = 2;
    int cells = n + quiet * 2;
    // One console cell per module: cells are 16x16 px, so modules are square.
    for (int y = 0; y < cells; y++) {
        at(kTopRow + y, 3);
        for (int x = 0; x < cells; x++) {
            bool dark = qrcodegen_getModule(qr, x - quiet, y - quiet);  // false outside the code
            printf(dark ? "\x1b[48;2;0;0;0m " : "\x1b[48;2;231;230;226m ");
        }
        printf("%s", kReset);
    }
    g_qrCells = cells;
    g_infoCol = 3 + cells + 3;
}

// Address under the QR code (or, with no QR, on the right).
void drawAddress() {
    if (g_url.empty() || !g_err1.empty()) return;
    int row = g_qrCells ? kTopRow + g_qrCells + 1 : kTopRow;
    int col = 3;
    int w = g_qrCells ? g_qrCells + 2 : textWidth();
    at(row, col);
    printf("%sOpen on your phone or PC%s", kMuted, kReset);
    at(row + 2, col);
    printf("%s%s%s", kText, clip(g_url, w).c_str(), kReset);
    // Same-network reminder: a mint bar as wide as the QR code, text centred.
    // (An odd spare cell goes on the left.)
    {
        const std::string msg = "Must be on the same network";
        int bar = g_qrCells ? g_qrCells : (int)msg.size() + 2;
        std::string s = clip(msg, bar);
        int left = (bar - (int)s.size() + 1) / 2;
        int right = bar - (int)s.size() - left;
        at(row + 4, col);
        printf("%s%s%*s%s%*s%s", kChipBg, kChipFg, left, "", s.c_str(), right, "", kReset);
    }
    if (!g_note.empty()) {
        at(row + 6, col);
        printf("%s%s%s", kMuted, clip(g_note, w).c_str(), kReset);
    }
}

// Right column rows
int taglineRow() { return kTopRow; }
int networkRow() { return kTopRow + 3; }
int gamesRow() { return kTopRow + 5; }
int logTitleRow() { return kTopRow + 8; }
int logPathRow() { return logTitleRow() + 1; }
int logHeadRow() { return logTitleRow() + 3; }
int firstLogRow() { return logHeadRow() + 2; }

void drawInfo() {
    int w = textWidth();
    blank(taglineRow(), g_infoCol, w);
    blank(taglineRow() + 1, g_infoCol, w);
    if (!g_err1.empty()) {
        at(taglineRow(), g_infoCol);
        printf("%s%s%s", kErr, clip(g_err1, w).c_str(), kReset);
        if (!g_err2.empty()) {
            at(taglineRow() + 1, g_infoCol);
            printf("%s%s%s", kMuted, clip(g_err2, w).c_str(), kReset);
        }
        return;
    }
    at(taglineRow(), g_infoCol);
    printf("%s%s%s", kMuted, clip("A title editor powered by icon sysmodules", w).c_str(), kReset);
}

void drawNetwork() {
    blank(networkRow(), g_infoCol, textWidth());
    if (!g_err1.empty()) return;
    if (g_net < 0) return;
    at(networkRow(), g_infoCol);
    if (g_net) printf("%sNetwork connected%s", kOk, kReset);
    else printf("%sWaiting for network...%s", kAccent, kReset);
}

void drawGames() {
    blank(gamesRow(), g_infoCol, textWidth());
    if (g_progDone >= 0) {
        // Solid bar: orange for done, grey band for the rest, then "20 / 40"
        const int barWidth = 24;
        int filled = g_progTotal > 0 ? g_progDone * barWidth / g_progTotal : 0;
        fill(gamesRow(), g_infoCol, filled, kChipBg);
        fill(gamesRow(), g_infoCol + filled, barWidth - filled, kBandBg);
        at(gamesRow(), g_infoCol + barWidth + 2);
        printf("%s%d / %d%s", kText, g_progDone, g_progTotal, kReset);
        return;
    }
    at(gamesRow(), g_infoCol);
    printf("%s%s%s", kText, clip(g_games, textWidth()).c_str(), kReset);
}

// Table columns (offsets from g_infoCol): Time 1, Action 8, Title 18, and
// Item right-aligned to end at column 78, so Time and Item sit one cell in
// from the ends of the header band.
constexpr int kTimeOff = 1, kActionOff = 8, kTitleOff = 18, kItemWidth = 4;
int itemCol() { return kCols - 1 - kItemWidth; }
int titleWidth() { return itemCol() - (g_infoCol + kTitleOff) - 2; }

void drawLog() {
    int w = textWidth();
    at(logTitleRow(), g_infoCol);
    printf("%sActivity%s", kAccent, kReset);
    at(logPathRow(), g_infoCol);
    printf("%s%s%s", kMuted, clip("Saved at /config/nx-title-editor/log.txt", w).c_str(), kReset);

    fill(logHeadRow(), g_infoCol, w, kBandBg);
    at(logHeadRow(), g_infoCol + kTimeOff);
    printf("%s%sTime", kBandBg, kMuted);
    at(logHeadRow(), g_infoCol + kActionOff);
    printf("%s%sAction", kBandBg, kMuted);
    at(logHeadRow(), g_infoCol + kTitleOff);
    printf("%s%sTitle", kBandBg, kMuted);
    at(logHeadRow(), itemCol());
    printf("%s%sItem%s", kBandBg, kMuted, kReset);

    // One entry every other row, keeping a blank row above the footer bar.
    int rows = (kFooterTop - 2 - firstLogRow()) / 2 + 1;
    int maxLines = rows < kMaxLog ? rows : kMaxLog;
    int start = (int)g_log.size() > maxLines ? (int)g_log.size() - maxLines : 0;
    for (int i = 0; i < maxLines; i++) {
        int row = firstLogRow() + i * 2;
        blank(row, g_infoCol, w);
        int idx = start + i;
        if (idx >= (int)g_log.size()) continue;
        const LogLine& l = g_log[idx];
        const char* c = l.kind == LogKind::Ok ? kOk : l.kind == LogKind::Error ? kErr : kMuted;
        at(row, g_infoCol + kTimeOff);
        printf("%s%s", kMuted, l.time.c_str());
        at(row, g_infoCol + kActionOff);
        printf("%s%s", c, l.action.c_str());
        at(row, g_infoCol + kTitleOff);
        if (l.spans) {
            printf("%s%s%s", kText, clip(l.title, w - kTitleOff - 1).c_str(), kReset);
        } else {
            printf("%s%s", kText, clip(l.title, titleWidth()).c_str());
            at(row, itemCol());
            printf("%s%s%s", kText, l.item.c_str(), kReset);
        }
    }
}

void drawNormal() {
    printf("%s\x1b[2J", kReset);
    drawHeader();
    drawHeaderStatus();
    drawFooter();
    if (!g_url.empty() && g_err1.empty()) {
        drawQr(g_url);
    } else {
        g_qrCells = 0;
        g_infoCol = 3;
    }
    drawAddress();
    drawInfo();
    drawNetwork();
    drawGames();
    drawLog();
    consoleUpdate(NULL);
}

void drawConfirm() {
    printf("%s\x1b[2J", kReset);
    drawHeader();
    drawHeaderStatus();
    // Full-width red band, edge to edge
    const int bandTop = 19, bandRows = 5;
    for (int r = 0; r < bandRows; r++) fill(bandTop + r, 1, kCols, kRedBg);
    const char* word = "R E B O O T ?";
    at(bandTop + bandRows / 2, (kCols - 13) / 2 + 1);
    printf("%s%s%s%s", kRedBg, kRedFg, word, kReset);
    for (int r = kFooterTop; r <= kFooterTop + 2; r++) fill(r, 1, kCols, kBarBg);
    static const char* const keys[] = {"-", "B"};
    static const char* const labels[] = {"Reboot", "Cancel"};
    drawKeys(keys, labels, 2, 3);
    consoleUpdate(NULL);
}

}  // namespace

void init(const char* version) {
    g_version = version;
    startLogFile(g_version);
    g_lastSpinNs = nowNs();
    drawNormal();
}

void showAddress(const std::string& url, const std::string& note) {
    g_url = url;
    g_note = note;
    g_err1.clear();
    g_err2.clear();
    if (g_mode == Mode::Normal) drawNormal();
}

void showError(const std::string& line1, const std::string& line2) {
    fileLine("error", line1 + (line2.empty() ? "" : " " + line2));
    g_err1 = line1;
    g_err2 = line2;
    if (g_mode == Mode::Normal) drawNormal();
}

void setStatus(const std::string& status, bool busy) {
    if (status == g_status && busy == g_busy) return;
    g_status = status;
    g_busy = busy;
    if (g_mode == Mode::Rebooting) return;
    drawHeaderStatus();
    consoleUpdate(NULL);
}

void setIdle() { setStatus(g_mode == Mode::ConfirmReboot ? "Confirm reboot" : "Ready", false); }

void tick() {
    if (g_mode == Mode::Rebooting) return;
    u64 now = nowNs();
    if (now - g_lastSpinNs < 120000000ULL) return;  // ~8 frames per second
    g_lastSpinNs = now;
    g_spin = (g_spin + 1) % 4;
    drawSpinnerOnly();
}

void setGamesLine(const std::string& line) {
    g_games = line;
    g_progDone = -1;
    if (g_mode != Mode::Normal) return;
    drawGames();
    consoleUpdate(NULL);
}

void progress(int done, int total) {
    g_progDone = done;
    g_progTotal = total;
    if (g_mode == Mode::Normal) drawGames();
    tick();
    consoleUpdate(NULL);
}

static void push(LogLine line) {
    g_log.push_back(std::move(line));
    while ((int)g_log.size() > kMaxLog) g_log.pop_front();
    g_logCount++;
    if (g_mode != Mode::Normal) return;
    drawLog();
    consoleUpdate(NULL);
}

void logAction(LogKind kind, const std::string& action, const std::string& title, const std::string& item,
               const std::string& titleId) {
    fileItem(action, item, title, titleId);
    push({clockNow(), kind, action, ascii(title), item, false});
}

// Errors are often too long for the screen: it says "Check log" and the
// full message goes to log.txt.
void logMessage(LogKind kind, const std::string& action, const std::string& message) {
    fileLine(action, message);
    push({clockNow(), kind, action, kind == LogKind::Error ? "Check log" : ascii(message), "", true});
}

void logFileOnly(const std::string& action, const std::string& title, const std::string& item,
                 const std::string& titleId) {
    fileItem(action, item, title, titleId);
}

void setLastIcon(const std::string& item) {
    if (g_log.empty()) return;
    g_log.back().item = item;
    if (g_mode != Mode::Normal) return;
    drawLog();
    consoleUpdate(NULL);
}

unsigned logCount() { return g_logCount; }

void setNetwork(bool ok) {
    if ((int)ok == g_net) return;
    g_net = ok ? 1 : 0;
    if (g_mode != Mode::Normal) return;
    drawNetwork();
    consoleUpdate(NULL);
}

Mode mode() { return g_mode; }

void showRebootConfirm() {
    g_mode = Mode::ConfirmReboot;
    g_status = "Confirm reboot";
    g_busy = false;
    drawConfirm();
}

void showNormal() {
    g_mode = Mode::Normal;
    g_status = "Ready";
    g_busy = false;
    drawNormal();
}

void showRebooting() {
    g_mode = Mode::Rebooting;
    printf("%s\x1b[2J", kReset);
    const char* msg = "Rebooting...";
    at(23, (kCols - 12) / 2 + 1);
    printf("%s%s%s", kMuted, msg, kReset);
    consoleUpdate(NULL);
}

std::string ascii(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        if (c >= 0x20 && c < 0x7f) {
            if (c == ' ' && (out.empty() || out.back() == ' ')) continue;
            out += (char)c;
        }
    }
    while (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

}  // namespace screen
