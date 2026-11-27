// NX Title Editor
// Copyright (C) 2026 ImmortalEye
// SPDX-License-Identifier: GPL-3.0-or-later

#include "storage.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>

namespace {
const char* kConfigDir = "sdmc:/config/nx-title-editor";
const char* kKeyPath = "sdmc:/config/nx-title-editor/sgdb_key.txt";
const char* kSettingsPath = "sdmc:/config/nx-title-editor/settings.txt";

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}
}  // namespace

bool readFile(const std::string& path, std::string& out) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    out.clear();
    char buf[16 * 1024];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) out.append(buf, n);
    fclose(f);
    return true;
}

bool writeFile(const std::string& path, const std::string& data) {
    // Write to a temp file first so a failed write never leaves a
    // half-written icon behind.
    std::string tmp = path + ".tmp";
    FILE* f = fopen(tmp.c_str(), "wb");
    if (!f) return false;
    bool ok = fwrite(data.data(), 1, data.size(), f) == data.size();
    ok = (fclose(f) == 0) && ok;
    if (!ok) {
        unlink(tmp.c_str());
        return false;
    }
    unlink(path.c_str());  // rename() won't replace an existing file on FAT
    return rename(tmp.c_str(), path.c_str()) == 0;
}

bool mkdirs(const std::string& dir) {
    // Skip the device prefix ("sdmc:/") when walking components.
    size_t start = dir.find(":/");
    start = (start == std::string::npos) ? 1 : start + 2;
    for (size_t i = start; i <= dir.size(); i++) {
        if (i == dir.size() || dir[i] == '/') {
            std::string part = dir.substr(0, i);
            if (mkdir(part.c_str(), 0777) != 0 && errno != EEXIST) return false;
        }
    }
    return true;
}

std::string loadSgdbKey() {
    std::string key;
    if (!readFile(kKeyPath, key)) return "";
    return trim(key);
}

bool saveSgdbKey(const std::string& key) {
    std::string k = trim(key);
    if (k.empty()) {
        unlink(kKeyPath);
        return true;
    }
    return mkdirs(kConfigDir) && writeFile(kKeyPath, k);
}

bool validShape(const std::string& s) {
    size_t colon = s.find(':');
    if (colon == std::string::npos || colon == 0 || colon == s.size() - 1) return false;
    std::string a = s.substr(0, colon), b = s.substr(colon + 1);
    for (const std::string* part : {&a, &b}) {
        if (part->size() > 6) return false;
        for (char c : *part)
            if (c < '0' || c > '9') return false;
        long v = strtol(part->c_str(), nullptr, 10);
        if (v < 1 || v > 100000) return false;
    }
    return true;
}

Settings loadSettings() {
    Settings s;
    std::string text;
    if (!readFile(kSettingsPath, text)) return s;
    size_t pos = 0;
    while (pos < text.size()) {
        size_t nl = text.find('\n', pos);
        if (nl == std::string::npos) nl = text.size();
        std::string line = trim(text.substr(pos, nl - pos));
        pos = nl + 1;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(line.substr(0, eq)), val = trim(line.substr(eq + 1));
        if (key == "home_shape" && validShape(val)) s.homeShape = val;
        else if (key == "all_shape" && validShape(val)) s.allShape = val;
        else if (key == "all_default" && (val == "skip" || val == "same")) s.allDefault = val;
        else if (key == "port") {
            long p = strtol(val.c_str(), nullptr, 10);
            if (p >= 0 && p <= 65535) s.port = (int)p;
        }
    }
    return s;
}

bool saveSettings(const Settings& s) {
    std::string text = "home_shape=" + s.homeShape + "\n" +
                       "all_shape=" + s.allShape + "\n" +
                       "all_default=" + s.allDefault + "\n" +
                       "port=" + std::to_string(s.port) + "\n";
    return mkdirs(kConfigDir) && writeFile(kSettingsPath, text);
}

std::string cachePath() { return std::string(kConfigDir) + "/titles_cache.txt"; }
