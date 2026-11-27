// NX Title Editor
// Copyright (C) 2026 ImmortalEye
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
// SD card helpers: settings, title cache and custom icon files.
// Everything lives under sdmc:/config/nx-title-editor/.

#include <string>

bool readFile(const std::string& path, std::string& out);
bool writeFile(const std::string& path, const std::string& data);
bool mkdirs(const std::string& dir);  // like mkdir -p

// SteamGridDB API key (sgdb_key.txt)
std::string loadSgdbKey();
bool saveSgdbKey(const std::string& key);  // empty key deletes the file

// Everything else the web page lets you set (settings.txt, key=value lines)
struct Settings {
    std::string homeShape = "1:1";    // Home screen (icon.jpg) display shape, "W:H"
    std::string allShape = "1:1";     // All Software (icon174.jpg) display shape, "W:H"
    std::string allDefault = "skip";  // default for the All Software slot: "skip" | "same"
    int port = 0;                     // 0 = automatic (8420, falling back to 8421-8429)
};

Settings loadSettings();
bool saveSettings(const Settings& s);

// "W:H" with both sides whole numbers from 1 to 100000
bool validShape(const std::string& s);

// Title cache (titles_cache.txt), read/written by titles.cpp
std::string cachePath();
