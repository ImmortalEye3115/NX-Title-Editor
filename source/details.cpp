// NX Title Editor
// Copyright (C) 2026 ImmortalEye
// SPDX-License-Identifier: GPL-3.0-or-later

#include "details.hpp"

#include <unistd.h>

#include <vector>

#include "storage.hpp"
#include "titles.hpp"

namespace {

const char* kSection = "[override_nacp]";

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

bool isHeader(const std::string& line) {
    std::string t = trim(line);
    return !t.empty() && t[0] == '[';
}

// Splits the file into lines (handles \n and \r\n) and drops a UTF-8 BOM.
std::vector<std::string> readLines(u64 id) {
    std::string data;
    std::vector<std::string> lines;
    if (!readFile(detailsPath(id), data)) return lines;
    if (data.compare(0, 3, "\xEF\xBB\xBF") == 0) data.erase(0, 3);
    size_t start = 0;
    while (start < data.size()) {
        size_t nl = data.find('\n', start);
        std::string line = data.substr(start, nl == std::string::npos ? std::string::npos : nl - start);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
        if (nl == std::string::npos) break;
        start = nl + 1;
    }
    return lines;
}

// Everything except our section, with leading/trailing blank lines dropped.
std::vector<std::string> otherLines(const std::vector<std::string>& lines, bool& had) {
    std::vector<std::string> rest;
    bool inOurs = false;
    had = false;
    for (const auto& line : lines) {
        if (isHeader(line)) inOurs = trim(line) == kSection;
        if (inOurs) {
            had = true;
            continue;
        }
        rest.push_back(line);
    }
    while (!rest.empty() && trim(rest.front()).empty()) rest.erase(rest.begin());
    while (!rest.empty() && trim(rest.back()).empty()) rest.pop_back();
    return rest;
}

std::string join(const std::vector<std::string>& lines) {
    std::string out;
    for (const auto& l : lines) out += l + "\n";
    return out;
}

}  // namespace

std::string detailsPath(u64 id) { return "sdmc:/atmosphere/contents/" + titleIdString(id) + "/config.ini"; }

TitleDetails readDetails(u64 id) {
    TitleDetails d;
    bool inOurs = false;
    for (const auto& line : readLines(id)) {
        if (isHeader(line)) {
            inOurs = trim(line) == kSection;
            continue;
        }
        if (!inOurs) continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(line.substr(0, eq)), value = trim(line.substr(eq + 1));
        if (key == "name") d.name = value;
        else if (key == "author") d.publisher = value;
        else if (key == "display_version") d.version = value;
    }
    return d;
}

bool writeDetails(u64 id, const TitleDetails& d) {
    bool had;
    std::vector<std::string> rest = otherLines(readLines(id), had);
    // sys-ticon wants its section at the top of the file.
    std::string out = std::string(kSection) + "\n";
    if (!d.name.empty() && !d.publisher.empty()) {
        out += "name=" + d.name + "\n";
        out += "author=" + d.publisher + "\n";
    }
    if (!d.version.empty()) out += "display_version=" + d.version + "\n";
    if (!rest.empty()) out += "\n" + join(rest);
    std::string dir = "sdmc:/atmosphere/contents/" + titleIdString(id);
    return mkdirs(dir) && writeFile(detailsPath(id), out);
}

bool removeDetails(u64 id, bool& had) {
    std::vector<std::string> rest = otherLines(readLines(id), had);
    if (!had) return true;
    if (rest.empty()) return unlink(detailsPath(id).c_str()) == 0;
    return writeFile(detailsPath(id), join(rest));
}
