// NX Title Editor
// Copyright (C) 2026 ImmortalEye
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fetch.hpp"

#include <curl/curl.h>

#include <cctype>
#include <cstdio>

namespace {

struct Sink {
    std::string* body;
    size_t max;
    bool overflow;
};

size_t writeCb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    Sink* sink = (Sink*)userdata;
    size_t n = size * nmemb;
    if (sink->body->size() + n > sink->max) {
        sink->overflow = true;
        return 0;  // abort the transfer
    }
    sink->body->append(ptr, n);
    return n;
}

}  // namespace

void fetchGlobalInit() { curl_global_init(CURL_GLOBAL_DEFAULT); }
void fetchGlobalExit() { curl_global_cleanup(); }

FetchResult httpGet(const std::string& url, const std::vector<std::string>& headers,
                    size_t maxBytes) {
    FetchResult res;
    CURL* curl = curl_easy_init();
    if (!curl) {
        res.error = "curl init failed";
        return res;
    }

    Sink sink{&res.body, maxBytes, false};
    curl_slist* hdrs = nullptr;
    for (const auto& h : headers) hdrs = curl_slist_append(hdrs, h.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "NX-Title-Editor/0.6");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    // Only allow http(s), including after redirects. The _STR options arrived
    // in curl 7.85; devkitPro's curl may be older.
#if LIBCURL_VERSION_NUM >= 0x075500
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https");
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "http,https");
#else
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS, (long)(CURLPROTO_HTTP | CURLPROTO_HTTPS));
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, (long)(CURLPROTO_HTTP | CURLPROTO_HTTPS));
#endif
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 45L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sink);

    CURLcode rc = curl_easy_perform(curl);
    if (rc == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res.status);
        char* ct = nullptr;
        curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);
        if (ct) res.contentType = ct;
    } else if (sink.overflow) {
        res.error = "file is too large";
    } else {
        res.error = curl_easy_strerror(rc);
    }

    curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);
    return res;
}

std::string urlEncode(const std::string& s) {
    std::string out;
    char buf[4];
    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out += (char)c;
        } else {
            snprintf(buf, sizeof(buf), "%%%02X", c);
            out += buf;
        }
    }
    return out;
}
