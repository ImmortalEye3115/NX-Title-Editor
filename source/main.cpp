// NX Title Editor
// Copyright (C) 2026 ImmortalEye
// SPDX-License-Identifier: GPL-3.0-or-later

// NX Title Editor: on-console web server.
// Open the app from the Homebrew Menu, then scan the QR code or browse to
// the address it shows.
//
// The browser does all image work (picking, stretch to 256x256 / 174x174,
// JPEG encoding). The Switch lists titles, fetches remote images/APIs on the
// browser's behalf, writes the finished JPEGs to the SD card and can reboot.

#include <arpa/inet.h>
#include <netinet/in.h>
#include <switch.h>
#include <unistd.h>

#include <cstdio>
#include <set>
#include <string>
#include <vector>

#include "details.hpp"
#include "fetch.hpp"
#include "http.hpp"
#include "screen.hpp"
#include "storage.hpp"
#include "titles.hpp"

namespace {

// Set once, in the Makefile's APP_VERSION, which Sphaira and the Homebrew
// Menu also show; the screen and the About popup read the same value.
#ifndef APP_VERSION
#define APP_VERSION "dev"
#endif
const char* kVersion = "v" APP_VERSION;
constexpr int kDefaultPort = 8420;
constexpr int kFallbackPorts = 10;                    // 8420-8429
constexpr size_t kMaxIconBytes = 2 * 1024 * 1024;    // finished JPEGs are ~10-60 KB
constexpr size_t kMaxFetchBytes = 20 * 1024 * 1024;  // source art from SGDB / URLs
#ifndef SGDB_BASE  // overridable for off-console testing
#define SGDB_BASE "https://www.steamgriddb.com/api/v2"
#endif
const char* kSgdbBase = SGDB_BASE;

std::vector<TitleInfo> g_titles;
ScanStats g_stats;
bool g_rebootRequested = false;
int g_activePort = 0;
// Titles whose icons changed since the app started (each needs a reboot to show).
std::set<u64> g_changed;
// The last icon write, so a Home + Apps save of the same game logs as one "Both" row.
struct { u64 id; bool small; unsigned logSeq; u64 ns; } g_lastWrite = {0, false, 0, 0};

// ---------------------------------------------------------------------------
// Title scanning
// ---------------------------------------------------------------------------
std::string gamesSummary(const ScanStats& s, bool full) {
    std::string line = std::to_string(s.total) + (s.total == 1 ? " game" : " games");
    std::vector<std::string> notes;
    if (!full && s.fromCache) {
        notes.push_back("from cache");
        if (s.newTitles) notes.push_back(std::to_string(s.newTitles) + " new");
        if (s.removed) notes.push_back(std::to_string(s.removed) + " removed");
    }
    if (s.skipped) notes.push_back(std::to_string(s.skipped) + " skipped");
    if (!notes.empty()) {
        line += " (";
        for (size_t i = 0; i < notes.size(); i++) line += (i ? ", " : "") + notes[i];
        line += ")";
    }
    return line;
}

void scan(bool full) {
    screen::setStatus("Scanning", true);
    Result rc = scanTitles(g_titles, full, g_stats, [](int done, int total) { screen::progress(done, total); });
    if (R_FAILED(rc)) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Couldn't list games (0x%x)", rc);
        screen::setGamesLine(buf);
        screen::logMessage(screen::LogKind::Error, "error", buf);
    } else {
        screen::setGamesLine(gamesSummary(g_stats, full));
        if (full) screen::logMessage(screen::LogKind::Info, "rescan", std::to_string(g_stats.total) + " games scanned");
    }
    screen::setIdle();
}

const TitleInfo* findTitle(u64 id) {
    for (const auto& t : g_titles)
        if (t.id == id) return &t;
    return nullptr;
}

std::string titleLabel(u64 id) {
    const TitleInfo* t = findTitle(id);
    return t ? t->name : titleIdString(id);
}

// ---------------------------------------------------------------------------
// JSON
// ---------------------------------------------------------------------------
std::string titleJson(const TitleInfo& t) {
    std::string j = "{\"id\":\"" + titleIdString(t.id) + "\"";
    j += ",\"name\":\"" + jsonEscape(t.name) + "\"";
    j += ",\"publisher\":\"" + jsonEscape(t.publisher) + "\"";
    j += ",\"version\":\"" + jsonEscape(t.version) + "\"";
    j += std::string(",\"custom_icon\":") + (hasCustomIcon(t.id) ? "true" : "false");
    j += std::string(",\"custom_icon174\":") + (hasCustomIcon(t.id, true) ? "true" : "false");
    TitleDetails d = readDetails(t.id);
    j += ",\"custom_name\":\"" + jsonEscape(d.name) + "\"";
    j += ",\"custom_publisher\":\"" + jsonEscape(d.publisher) + "\"";
    j += ",\"custom_version\":\"" + jsonEscape(d.version) + "\"";
    return j + "}";
}

std::string titlesJson() {
    std::string j = "{\"titles\":[";
    for (size_t i = 0; i < g_titles.size(); i++) {
        if (i) j += ",";
        j += titleJson(g_titles[i]);
    }
    return j + "],\"skipped\":" + std::to_string(g_stats.skipped) + ",\"changes\":" + std::to_string(g_changed.size()) + "}";
}

std::string settingsJson() {
    Settings s = loadSettings();
    std::string j = "{";
    j += std::string("\"has_sgdb_key\":") + (loadSgdbKey().empty() ? "false" : "true");
    j += ",\"home_shape\":\"" + s.homeShape + "\"";
    j += ",\"all_shape\":\"" + s.allShape + "\"";
    j += ",\"all_default\":\"" + s.allDefault + "\"";
    j += ",\"port\":" + std::to_string(s.port);
    j += ",\"active_port\":" + std::to_string(g_activePort);
    j += std::string(",\"version\":\"") + kVersion + "\"";
    return j + "}";
}

// ---------------------------------------------------------------------------
// Routes
// ---------------------------------------------------------------------------
bool startsWith(const std::string& s, const char* prefix) { return s.rfind(prefix, 0) == 0; }

HttpResponse serveStatic(const std::string& urlPath) {
    std::string rel = (urlPath == "/") ? "/index.html" : urlPath;
    if (rel.find("..") != std::string::npos) return HttpResponse::text(400, "Bad path");
    HttpResponse r;
    if (!readFile("romfs:/web" + rel, r.body)) return HttpResponse::text(404, "Not found");
    r.contentType = mimeForPath(rel);
    return r;
}

HttpResponse jpegResponse(std::string data, bool cacheable) {
    HttpResponse r;
    r.contentType = "image/jpeg";
    r.body = std::move(data);
    r.headers.push_back({"Cache-Control", cacheable ? "max-age=3600" : "no-store"});
    return r;
}

HttpResponse okWithChanges() {
    return HttpResponse::json("{\"ok\":true,\"changes\":" + std::to_string(g_changed.size()) + "}");
}

// /api/custom-icon/<TID> : GET (read), POST (write), DELETE (remove)
HttpResponse customIcon(const HttpRequest& req, u64 id) {
    std::string size = req.param("size");

    if (req.method == "GET") {
        std::string jpeg;
        if (!readFile(customIconPath(id, size == "174"), jpeg))
            return HttpResponse::text(404, "No custom icon");
        return jpegResponse(std::move(jpeg), false);
    }

    if (req.method == "POST") {
        if (size != "256" && size != "174") return HttpResponse::jsonError(400, "size must be 256 or 174");
        if (req.body.size() < 4 || (unsigned char)req.body[0] != 0xFF || (unsigned char)req.body[1] != 0xD8)
            return HttpResponse::jsonError(415, "Expected a JPEG image");
        if (req.body.size() > kMaxIconBytes) return HttpResponse::jsonError(413, "Icon file is too large");

        screen::setStatus("Writing icon to SD card...", true);
        std::string dir = "sdmc:/atmosphere/contents/" + titleIdString(id);
        if (!mkdirs(dir)) return HttpResponse::jsonError(500, "Couldn't create " + dir);
        std::string path = customIconPath(id, size == "174");
        if (!writeFile(path, req.body)) return HttpResponse::jsonError(500, "Couldn't write " + path);
        bool small = size == "174";
        u64 now = armTicksToNs(armGetSystemTick());
        bool merge = g_lastWrite.id == id && g_lastWrite.small != small && g_lastWrite.logSeq == screen::logCount() &&
                     now - g_lastWrite.ns < 10000000000ULL;  // same save: within 10 s, nothing logged since
        if (merge) {
            screen::logFileOnly("wrote", titleLabel(id), small ? "Apps" : "Home", titleIdString(id));
            screen::setLastIcon("Both");
        }
        else
            screen::logAction(screen::LogKind::Ok, "wrote", titleLabel(id), small ? "Apps" : "Home", titleIdString(id));
        g_lastWrite = {id, small, screen::logCount(), now};
        g_changed.insert(id);
        return okWithChanges();
    }

    if (req.method == "DELETE") {
        // size=256, 174, or all (default)
        bool home = size != "174" && unlink(customIconPath(id, false).c_str()) == 0;
        bool all = size != "256" && unlink(customIconPath(id, true).c_str()) == 0;
        if (home || all) {
            screen::logAction(screen::LogKind::Ok, "removed", titleLabel(id), home && all ? "Both" : home ? "Home" : "Apps",
                              titleIdString(id));
            g_changed.insert(id);
        }
        return okWithChanges();
    }

    return HttpResponse::jsonError(405, "Method not allowed");
}

// SteamGridDB passthrough (the API key lives on the SD card)
HttpResponse sgdbGet(const std::string& url, const char* job) {
    std::string key = loadSgdbKey();
    if (key.empty()) return HttpResponse::jsonError(400, "Add your SteamGridDB API key in Settings first.");
    screen::setStatus(job, true);
    FetchResult r = httpGet(url, {"Authorization: Bearer " + key}, kMaxFetchBytes);
    if (r.status == 0) return HttpResponse::jsonError(502, "Couldn't reach SteamGridDB: " + r.error);
    if (r.status == 401 || r.status == 403)
        return HttpResponse::jsonError(502, "SteamGridDB rejected the API key. Check it in Settings.");
    if (r.status == 404) return HttpResponse::json("{\"success\":true,\"data\":[]}");
    if (r.status < 200 || r.status >= 300)
        return HttpResponse::jsonError(502, "SteamGridDB returned HTTP " + std::to_string(r.status));
    return HttpResponse::json(r.body);
}

// Generic image fetch, so the browser can draw remote art to a canvas
HttpResponse fetchImage(const std::string& url) {
    if (!startsWith(url, "http://") && !startsWith(url, "https://"))
        return HttpResponse::jsonError(400, "Only http:// and https:// links are supported");
    bool sgdb = url.find("steamgriddb.com") != std::string::npos;
    screen::setStatus(sgdb ? "Downloading image from SteamGridDB..." : "Downloading image...", true);
    FetchResult r = httpGet(url, {}, kMaxFetchBytes);
    if (r.status == 0) return HttpResponse::jsonError(502, "Download failed: " + r.error);
    if (r.status < 200 || r.status >= 300)
        return HttpResponse::jsonError(502, "That link returned HTTP " + std::to_string(r.status));
    HttpResponse res;
    res.contentType = r.contentType.empty() ? "application/octet-stream" : r.contentType;
    res.body = std::move(r.body);
    res.headers.push_back({"Cache-Control", "no-store"});
    return res;
}

HttpResponse saveSettingsRoute(const HttpRequest& req) {
    Settings s = loadSettings();
    std::string v;
    if (formValue(req.body, "home_shape", v)) {
        if (!validShape(v)) return HttpResponse::jsonError(400, "Home screen shape must look like 2:3");
        s.homeShape = v;
    }
    if (formValue(req.body, "all_shape", v)) {
        if (!validShape(v)) return HttpResponse::jsonError(400, "All Software shape must look like 2:3");
        s.allShape = v;
    }
    if (formValue(req.body, "all_default", v)) {
        if (v != "skip" && v != "same") return HttpResponse::jsonError(400, "all_default must be skip or same");
        s.allDefault = v;
    }
    if (formValue(req.body, "port", v)) {
        if (v.empty()) {
            s.port = 0;
        } else {
            if (v.size() > 5 || v.find_first_not_of("0123456789") != std::string::npos)
                return HttpResponse::jsonError(400, "Port must be a number from 1 to 65535");
            int p = atoi(v.c_str());
            if (p < 1 || p > 65535) return HttpResponse::jsonError(400, "Port must be a number from 1 to 65535");
            s.port = p;
        }
    }
    // An empty sgdb_key removes the saved key (the web page sends it when the box is cleared).
    if (formValue(req.body, "sgdb_key", v) && !saveSgdbKey(v))
        return HttpResponse::jsonError(500, "Couldn't save the API key to the SD card");
    if (!saveSettings(s)) return HttpResponse::jsonError(500, "Couldn't save settings to the SD card");
    return HttpResponse::json(settingsJson());
}
// /api/details/<TID> : POST (save name/publisher/version), DELETE (remove)
// Writes sys-ticon's [override_nacp] section in the title's config.ini.
HttpResponse detailsRoute(const HttpRequest& req, u64 id) {
    if (req.method == "POST") {
        TitleDetails d;
        formValue(req.body, "name", d.name);
        formValue(req.body, "publisher", d.publisher);
        formValue(req.body, "version", d.version);
        for (const std::string* v : {&d.name, &d.publisher, &d.version})
            if (v->find_first_of("\r\n") != std::string::npos)
                return HttpResponse::jsonError(400, "Details can't contain line breaks");
        if (d.name.empty() != d.publisher.empty())
            return HttpResponse::jsonError(400, "Name and publisher must be set together");
        if (d.name.size() > kMaxNameBytes || d.publisher.size() > kMaxPublisherBytes || d.version.size() > kMaxVersionBytes)
            return HttpResponse::jsonError(400, "Too long for sys-ticon");
        if (!d.any()) return HttpResponse::jsonError(400, "Nothing to save");
        screen::setStatus("Writing details to SD card...", true);
        if (!writeDetails(id, d)) return HttpResponse::jsonError(500, "Couldn't write " + detailsPath(id));
        screen::logAction(screen::LogKind::Ok, "wrote", titleLabel(id), "Info", titleIdString(id));
        g_changed.insert(id);
        return okWithChanges();
    }

    if (req.method == "DELETE") {
        bool had = false;
        if (!removeDetails(id, had)) return HttpResponse::jsonError(500, "Couldn't update " + detailsPath(id));
        if (had) {
            screen::logAction(screen::LogKind::Ok, "removed", titleLabel(id), "Info", titleIdString(id));
            g_changed.insert(id);
        }
        return okWithChanges();
    }

    return HttpResponse::jsonError(405, "Method not allowed");
}

bool validGameId(const std::string& s) {
    return !s.empty() && s.size() < 12 && s.find_first_not_of("0123456789") == std::string::npos;
}

HttpResponse routeApi(const HttpRequest& req) {
    const std::string& p = req.path;
    const std::string& m = req.method;

    if (p == "/api/titles" && m == "GET") {
        if (req.param("refresh") == "1") scan(true);
        return HttpResponse::json(titlesJson());
    }

    if (startsWith(p, "/api/icon/") && m == "GET") {
        u64 id;
        std::string jpeg;
        if (!parseTitleId(p.substr(10), id)) return HttpResponse::text(400, "Bad title ID");
        if (!readTitleIcon(id, jpeg)) return HttpResponse::text(404, "No icon");
        return jpegResponse(std::move(jpeg), true);
    }

    if (startsWith(p, "/api/custom-icon/")) {
        u64 id;
        if (!parseTitleId(p.substr(17), id)) return HttpResponse::jsonError(400, "Bad title ID");
        return customIcon(req, id);
    }

    if (startsWith(p, "/api/details/")) {
        u64 id;
        if (!parseTitleId(p.substr(13), id)) return HttpResponse::jsonError(400, "Bad title ID");
        return detailsRoute(req, id);
    }

    if (p == "/api/settings") {
        if (m == "GET") return HttpResponse::json(settingsJson());
        if (m == "POST") return saveSettingsRoute(req);
    }

    if (p == "/api/sgdb/search" && m == "GET") {
        std::string q = req.param("q");
        if (q.empty()) return HttpResponse::json("{\"success\":true,\"data\":[]}");
        return sgdbGet(std::string(kSgdbBase) + "/search/autocomplete/" + urlEncode(q), "Searching SteamGridDB...");
    }

    if (startsWith(p, "/api/sgdb/grids/") && m == "GET") {
        std::string gameId = p.substr(16);
        if (!validGameId(gameId)) return HttpResponse::jsonError(400, "Bad game ID");
        // SteamGridDB's grid shapes and the sizes it files under each
        std::string style = req.param("style");
        const char* dims = style == "vertical" || style == "boxart" ? "600x900"
                           : style == "galaxy"                     ? "342x482,660x930"
                           : style == "horizontal"                 ? "460x215,920x430"
                                                                   : "512x512,1024x1024";
        std::string url = std::string(kSgdbBase) + "/grids/game/" + gameId + "?dimensions=" + dims;
        return sgdbGet(url, "Loading art from SteamGridDB...");
    }

    if (p == "/api/fetch" && m == "GET") return fetchImage(req.param("url"));

    if (p == "/api/reboot" && m == "POST") {
        // Reply first; the main loop reboots once this response is sent.
        g_rebootRequested = true;
        return HttpResponse::jsonOk();
    }

    return HttpResponse::jsonError(404, "Unknown API route");
}

// Pulls the message out of {"error":"..."} for the activity log.
std::string errorMessage(const std::string& body) {
    const std::string key = "{\"error\":\"";
    if (!startsWith(body, key.c_str())) return "";
    std::string msg = body.substr(key.size());
    if (msg.size() >= 2) msg.resize(msg.size() - 2);
    std::string out;
    for (size_t i = 0; i < msg.size(); i++) {
        if (msg[i] == '\\' && i + 1 < msg.size()) i++;
        out += msg[i];
    }
    return out;
}

HttpResponse route(const HttpRequest& req) {
    HttpResponse res;
    bool api = startsWith(req.path, "/api/");
    if (api)
        res = routeApi(req);
    else if (req.method == "GET")
        res = serveStatic(req.path);
    else
        res = HttpResponse::text(405, "Method not allowed");

    // Log API failures, except "this game has no icon" lookups, which are normal.
    bool iconLookup = req.method == "GET" &&
                      (startsWith(req.path, "/api/icon/") || startsWith(req.path, "/api/custom-icon/"));
    if (api && res.status >= 400 && !iconLookup) {
        std::string msg = errorMessage(res.body);
        // The screen shows "Check log"; the file gets the message and the request.
        std::string where = req.method + " " + req.path + " " + std::to_string(res.status);
        screen::logMessage(screen::LogKind::Error, "error", msg.empty() ? where : msg + " (" + where + ")");
    }
    screen::setIdle();
    return res;
}

// ---------------------------------------------------------------------------
// Startup helpers
// ---------------------------------------------------------------------------
// Tries the saved port (if any), then 8420-8429. Returns false if none work.
bool startServer(HttpServer& server, std::string& note, std::string& failure) {
    Settings s = loadSettings();
    std::vector<int> ports;
    if (s.port > 0) ports.push_back(s.port);
    for (int i = 0; i < kFallbackPorts; i++)
        if (kDefaultPort + i != s.port) ports.push_back(kDefaultPort + i);

    std::string err;
    for (int p : ports) {
        if (server.start(p, err)) {
            int wanted = s.port > 0 ? s.port : kDefaultPort;
            if (p != wanted) note = "(port " + std::to_string(wanted) + " was busy)";
            g_activePort = p;
            return true;
        }
    }
    failure = s.port > 0 ? "Port " + std::to_string(s.port) + " and ports 8420-8429 are all in use."
                         : "Ports 8420-8429 are all in use.";
    return false;
}

// The Switch's current IP (network byte order), or 0 when there's no network.
u32 currentIp() {
    u32 ip = 0;
    if (R_FAILED(nifmGetCurrentIpAddress(&ip))) return 0;
    if (ip == htonl(INADDR_LOOPBACK) || ip == INADDR_LOOPBACK) return 0;
    return ip;
}

std::string urlFor(u32 ip, int port) {
    in_addr a{};
    a.s_addr = (in_addr_t)ip;
    char url[64];
    snprintf(url, sizeof(url), "http://%s:%d", inet_ntoa(a), port);
    return url;
}

// Watches the network about once a second. When it drops, the screen says
// "Waiting for network..."; when it comes back (or the IP changes, or the
// listener dies), the web server is reopened on the same port and the QR
// code and address are redrawn.
struct NetWatch {
    u32 ip = 0;           // address the page was last shown on
    bool up = false;
    bool everUp = false;  // to say "Connected" vs "Reconnected"
    u64 lastCheckNs = 0;
};

void showNetDown(NetWatch& nw) {
    nw.up = false;
    screen::setNetwork(false);
    screen::showAddress("", "");
    screen::setStatus("Waiting for network", true);
}

void checkNetwork(NetWatch& nw, HttpServer& server, bool& serving, std::string& note) {
    u64 now = armTicksToNs(armGetSystemTick());
    if (now - nw.lastCheckNs < 1000000000ULL) return;
    nw.lastCheckNs = now;

    u32 ip = currentIp();
    if (ip == 0) {
        if (nw.up) {
            showNetDown(nw);
            screen::logMessage(screen::LogKind::Info, "network", "Connection lost, waiting...");
        }
        return;
    }
    bool wasDown = !nw.up, moved = ip != nw.ip, dead = !serving || server.broken();
    if (!wasDown && !moved && !dead) return;

    // Reopen the listener: after a drop the old socket is dead even if the IP is the same.
    server.stop();
    std::string failure;
    serving = server.start(g_activePort, failure) || startServer(server, note, failure);
    if (!serving) {
        screen::showError("Couldn't restart the web server.", failure);
        return;
    }
    bool first = !nw.everUp;
    nw.ip = ip;
    nw.up = nw.everUp = true;
    screen::setNetwork(true);
    screen::showAddress(urlFor(ip, g_activePort), note);
    screen::setIdle();
    if (wasDown) screen::logMessage(screen::LogKind::Ok, "network", first ? "Connected" : "Reconnected");
    else if (moved) screen::logMessage(screen::LogKind::Info, "network", "New address, QR code updated");
    else screen::logMessage(screen::LogKind::Info, "network", "Web server restarted");
}

void rebootConsole() {
    screen::showRebooting();
    svcSleepThread(500'000'000ULL);  // let the browser get its reply

    // spsm is what the HOME-menu power options use; bpc is the fallback.
    Result rc = spsmInitialize();
    if (R_SUCCEEDED(rc)) {
        rc = spsmShutdown(true);
        spsmExit();
    }
    if (R_FAILED(rc)) {
        Result rc2 = bpcInitialize();
        if (R_SUCCEEDED(rc2)) {
            rc2 = bpcRebootSystem();
            bpcExit();
        }
        char buf[80];
        snprintf(buf, sizeof(buf), "Reboot failed (0x%x / 0x%x), use HOME", rc, rc2);
        screen::showNormal();
        screen::logMessage(screen::LogKind::Error, "error", buf);
    }
}

}  // namespace

int main(int argc, char** argv) {
    consoleInit(NULL);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    screen::init(kVersion);

    romfsInit();
    socketInitializeDefault();
    fetchGlobalInit();
    nsInitialize();
    // Keep the console awake while the server is open.
    appletSetMediaPlaybackState(true);

    // Ask the Switch to keep the network up while the app is open, like a
    // game does. Best effort: the app still works if this is refused.
    NifmRequest netRequest{};
    bool netRequested = false;
    if (R_SUCCEEDED(nifmInitialize(NifmServiceType_User))) {
        if (R_SUCCEEDED(nifmCreateRequest(&netRequest, true))) {
            netRequested = R_SUCCEEDED(nifmRequestSubmit(&netRequest));
            if (!netRequested) nifmRequestClose(&netRequest);
        }
    }

    scan(false);

    HttpServer server(route);
    std::string note, failure;
    bool serving = startServer(server, note, failure);

    NetWatch nw;
    nw.ip = currentIp();
    if (!serving) {
        screen::showError("Couldn't start the web server.", failure);
    } else if (nw.ip == 0) {
        showNetDown(nw);  // picked up automatically once the network is back
    } else {
        nw.up = nw.everUp = true;
        screen::setNetwork(true);
        screen::showAddress(urlFor(nw.ip, g_activePort), note);
    }
    if (nw.up || !serving) screen::setIdle();

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 down = padGetButtonsDown(&pad);
        if (screen::mode() == screen::Mode::ConfirmReboot) {
            // Only - (reboot) and B (cancel) do anything on the confirmation screen.
            if (down & HidNpadButton_Minus) g_rebootRequested = true;
            else if (down & HidNpadButton_B) screen::showNormal();
        } else {
            if (down & HidNpadButton_Plus) break;
            if (down & HidNpadButton_A) scan(true);
            if (down & HidNpadButton_Minus) screen::showRebootConfirm();
        }

        if (serving) server.poll(16);
        checkNetwork(nw, server, serving, note);
        if (g_rebootRequested) {
            g_rebootRequested = false;
            server.stop();
            rebootConsole();
            serving = server.start(g_activePort, failure);  // only reached if the reboot failed
        }
        screen::tick();
        consoleUpdate(NULL);
    }

    server.stop();
    if (netRequested) nifmRequestClose(&netRequest);
    nifmExit();
    appletSetMediaPlaybackState(false);
    nsExit();
    fetchGlobalExit();
    socketExit();
    romfsExit();
    consoleExit(NULL);
    return 0;
}
