// NX Title Editor
// Copyright (C) 2026 ImmortalEye
// SPDX-License-Identifier: GPL-3.0-or-later

#include "titles.hpp"

#include <sys/stat.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>

#include "storage.hpp"

namespace {

const char* kCacheHeader = "NXTE-CACHE 1";

// NsApplicationControlData is ~0x24000 bytes; keep it off the stack.
NsApplicationControlData* allocControlData() {
    return (NsApplicationControlData*)malloc(sizeof(NsApplicationControlData));
}

std::string fixedString(const char* s, size_t max) {
    // NACP strings aren't guaranteed to be NUL-terminated.
    return std::string(s, strnlen(s, max));
}

std::string lower(const std::string& s) {
    std::string out = s;
    for (auto& c : out) c = (char)std::tolower((unsigned char)c);
    return out;
}

// Tabs/newlines would break the cache's line format.
std::string cacheField(const std::string& s) {
    std::string out = s;
    for (auto& c : out)
        if (c == '\t' || c == '\n' || c == '\r') c = ' ';
    return out;
}

Result listApplicationIds(std::vector<u64>& ids) {
    ids.clear();
    const s32 kBatch = 64;
    NsApplicationRecord records[kBatch];
    std::set<u64> seen;
    s32 offset = 0;
    while (true) {
        s32 count = 0;
        Result rc = nsListApplicationRecord(records, kBatch, offset, &count);
        if (R_FAILED(rc)) return ids.empty() ? rc : 0;
        if (count <= 0) break;
        offset += count;
        for (s32 i = 0; i < count; i++)
            if (seen.insert(records[i].application_id).second) ids.push_back(records[i].application_id);
        if (count < kBatch) break;
    }
    return 0;
}

TitleInfo readTitleInfo(u64 id, NsApplicationControlData* ctrl) {
    TitleInfo t;
    t.id = id;
    u64 size = 0;
    memset(ctrl, 0, sizeof(*ctrl));
    Result r = nsGetApplicationControlData(NsApplicationControlSource_Storage, id, ctrl, sizeof(*ctrl), &size);
    if (R_FAILED(r) || size < sizeof(ctrl->nacp)) {
        t.readable = false;
        return t;
    }
    NacpLanguageEntry* lang = nullptr;
    if (R_SUCCEEDED(nacpGetLanguageEntry(&ctrl->nacp, &lang)) && lang) {
        t.name = fixedString(lang->name, sizeof(lang->name));
        t.publisher = fixedString(lang->author, sizeof(lang->author));
    }
    if (t.name.empty()) t.name = titleIdString(id);
    t.version = fixedString(ctrl->nacp.display_version, sizeof(ctrl->nacp.display_version));
    return t;
}

// Cache format: header line, then one line per title:
// TITLEID \t R|S \t name \t publisher \t version   (S = skipped/unreadable)
std::map<u64, TitleInfo> loadCache() {
    std::map<u64, TitleInfo> cache;
    std::string text;
    if (!readFile(cachePath(), text)) return cache;
    size_t pos = 0;
    bool first = true;
    while (pos < text.size()) {
        size_t nl = text.find('\n', pos);
        if (nl == std::string::npos) nl = text.size();
        std::string line = text.substr(pos, nl - pos);
        pos = nl + 1;
        if (first) {
            first = false;
            if (line != kCacheHeader) return {};  // unknown format: ignore it
            continue;
        }
        std::vector<std::string> f;
        size_t s = 0;
        while (true) {
            size_t tab = line.find('\t', s);
            f.push_back(line.substr(s, tab == std::string::npos ? std::string::npos : tab - s));
            if (tab == std::string::npos) break;
            s = tab + 1;
        }
        TitleInfo t;
        if (f.size() != 5 || !parseTitleId(f[0], t.id)) continue;
        t.readable = f[1] != "S";
        t.name = f[2];
        t.publisher = f[3];
        t.version = f[4];
        cache[t.id] = t;
    }
    return cache;
}

void saveCache(const std::map<u64, TitleInfo>& titles) {
    std::string text = std::string(kCacheHeader) + "\n";
    for (const auto& kv : titles) {
        const TitleInfo& t = kv.second;
        text += titleIdString(t.id) + "\t" + (t.readable ? "R" : "S") + "\t" + cacheField(t.name) + "\t" +
                cacheField(t.publisher) + "\t" + cacheField(t.version) + "\n";
    }
    std::string path = cachePath();
    mkdirs(path.substr(0, path.rfind('/')));
    writeFile(path, text);
}

}  // namespace

Result scanTitles(std::vector<TitleInfo>& out, bool full, ScanStats& stats, const ScanProgress& progress) {
    stats = ScanStats{};
    std::vector<u64> ids;
    Result rc = listApplicationIds(ids);
    if (R_FAILED(rc)) return rc;

    std::map<u64, TitleInfo> cache = full ? std::map<u64, TitleInfo>{} : loadCache();
    stats.fromCache = !full && !cache.empty();

    // Which installed titles need reading?
    std::vector<u64> toRead;
    for (u64 id : ids)
        if (!cache.count(id)) toRead.push_back(id);
    std::set<u64> installed(ids.begin(), ids.end());
    for (const auto& kv : cache)
        if (!installed.count(kv.first)) stats.removed++;

    std::map<u64, TitleInfo> result;
    for (u64 id : ids)
        if (cache.count(id)) result[id] = cache[id];

    if (!toRead.empty()) {
        NsApplicationControlData* ctrl = allocControlData();
        if (!ctrl) return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
        for (size_t i = 0; i < toRead.size(); i++) {
            if (progress) progress((int)i, (int)toRead.size());
            result[toRead[i]] = readTitleInfo(toRead[i], ctrl);
        }
        if (progress) progress((int)toRead.size(), (int)toRead.size());
        free(ctrl);
    }
    stats.newTitles = (int)toRead.size();

    if (full || !toRead.empty() || stats.removed > 0 || cache.empty()) saveCache(result);

    out.clear();
    for (const auto& kv : result) {
        if (kv.second.readable)
            out.push_back(kv.second);
        else
            stats.skipped++;
    }
    std::sort(out.begin(), out.end(), [](const TitleInfo& a, const TitleInfo& b) {
        return lower(a.name) < lower(b.name);
    });
    stats.total = (int)out.size();
    return 0;
}

bool readTitleIcon(u64 id, std::string& jpeg) {
    NsApplicationControlData* ctrl = allocControlData();
    if (!ctrl) return false;
    u64 size = 0;
    Result r = nsGetApplicationControlData(NsApplicationControlSource_Storage, id, ctrl, sizeof(*ctrl), &size);
    bool ok = R_SUCCEEDED(r) && size > sizeof(ctrl->nacp);
    if (ok) {
        size_t iconSize = std::min<size_t>(size - sizeof(ctrl->nacp), sizeof(ctrl->icon));
        jpeg.assign((const char*)ctrl->icon, iconSize);
    }
    free(ctrl);
    return ok;
}

std::string titleIdString(u64 id) {
    char buf[17];
    snprintf(buf, sizeof(buf), "%016lX", (unsigned long)id);
    return buf;
}

std::string customIconPath(u64 id, bool small) {
    return "sdmc:/atmosphere/contents/" + titleIdString(id) + (small ? "/icon174.jpg" : "/icon.jpg");
}

bool hasCustomIcon(u64 id, bool small) {
    struct stat st;
    return stat(customIconPath(id, small).c_str(), &st) == 0;
}

bool parseTitleId(const std::string& s, u64& out) {
    if (s.size() != 16) return false;
    for (char c : s)
        if (!std::isxdigit((unsigned char)c)) return false;
    out = strtoull(s.c_str(), nullptr, 16);
    return true;
}
