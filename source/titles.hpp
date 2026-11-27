// NX Title Editor
// Copyright (C) 2026 ImmortalEye
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
// Installed-title listing via the ns service, with an SD card cache.
// Works for everything the console has a record for: retail games, eShop
// titles and custom NSPs.
//
// Listing Title IDs is near instant; reading each title's name/version
// (its control data) is the slow part. So on start we load the cache,
// compare Title IDs, and only read titles that are new.

#include <switch.h>

#include <functional>
#include <string>
#include <vector>

struct TitleInfo {
    u64 id = 0;
    bool readable = true;  // false: control data couldn't be read (archived, card removed...)
    std::string name;
    std::string publisher;
    std::string version;
};

struct ScanStats {
    int total = 0;      // readable titles in the final list
    int skipped = 0;    // records whose details couldn't be read
    int newTitles = 0;  // titles read this time (quick check) or all (full scan)
    int removed = 0;    // titles dropped since the cache was written
    bool fromCache = false;
};

using ScanProgress = std::function<void(int done, int total)>;

// full=false: quick check against the cache (reads only new titles).
// full=true:  re-read every title. Both save the cache afterwards.
// `out` only contains readable titles, sorted by name.
Result scanTitles(std::vector<TitleInfo>& out, bool full, ScanStats& stats, const ScanProgress& progress);

// Reads the title's built-in JPEG icon from its control data.
bool readTitleIcon(u64 id, std::string& jpeg);

// Paths used by sys-icon / Atmosphère content overrides.
std::string titleIdString(u64 id);  // 16 upper-case hex chars
std::string customIconPath(u64 id, bool small = false);
bool hasCustomIcon(u64 id, bool small = false);
bool parseTitleId(const std::string& s, u64& out);
