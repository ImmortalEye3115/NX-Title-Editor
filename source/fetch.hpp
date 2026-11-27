// NX Title Editor
// Copyright (C) 2026 ImmortalEye
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
// Outbound HTTP(S) via libcurl. The browser can't call SteamGridDB or most
// image hosts directly (CORS), so the Switch fetches on its behalf.

#include <string>
#include <vector>

struct FetchResult {
    long status = 0;          // HTTP status, 0 if the request never completed
    std::string body;
    std::string contentType;
    std::string error;        // set when status == 0 or the body was too large
};

void fetchGlobalInit();
void fetchGlobalExit();

FetchResult httpGet(const std::string& url, const std::vector<std::string>& headers,
                    size_t maxBytes);

// Percent-encodes one URL path segment / query value.
std::string urlEncode(const std::string& s);
