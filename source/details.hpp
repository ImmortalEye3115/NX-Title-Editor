// NX Title Editor
// Copyright (C) 2026 ImmortalEye
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
// Custom game details for sys-ticon: the [override_nacp] section of
// sdmc:/atmosphere/contents/<TitleID>/config.ini
//
//   [override_nacp]
//   name=Some Title
//   author=Some Publisher
//   display_version=21.37
//
// sys-ticon rules: UTF-8 without BOM; name <= 512 bytes, author <= 256 bytes,
// display_version <= 16 bytes; name and author only count when both are set.
// The file may hold other sections, so we only ever touch ours.

#include <switch.h>

#include <string>

struct TitleDetails {
    std::string name;       // empty = not overridden
    std::string publisher;  // written as "author"
    std::string version;    // written as "display_version"
    bool any() const { return !name.empty() || !publisher.empty() || !version.empty(); }
};

constexpr size_t kMaxNameBytes = 512;
constexpr size_t kMaxPublisherBytes = 256;
constexpr size_t kMaxVersionBytes = 16;

std::string detailsPath(u64 id);

// Reads our section. Returns an empty TitleDetails if there's no file or section.
TitleDetails readDetails(u64 id);

// Replaces our section (or adds it at the top), keeping the rest of the file.
bool writeDetails(u64 id, const TitleDetails& d);

// Removes our section; deletes the file if nothing else is left in it.
// `had` is set when there was a section to remove.
bool removeDetails(u64 id, bool& had);
