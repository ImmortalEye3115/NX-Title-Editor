// NX Title Editor
// Copyright (C) 2026 ImmortalEye
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
// Minimal single-threaded HTTP/1.1 server: one request per connection,
// polled from the main loop so the console UI and controller stay responsive.

#include <functional>
#include <string>
#include <utility>
#include <vector>

struct HttpRequest {
    std::string method;
    std::string path;   // without the query string
    std::string query;  // raw query string, without '?'
    std::string body;
    std::string header(const std::string& name) const;
    // Decoded value of a query-string parameter, or "" if absent.
    std::string param(const std::string& name) const;
    std::vector<std::pair<std::string, std::string>> headers;  // names lower-cased
};

struct HttpResponse {
    int status = 200;
    std::string contentType = "text/plain; charset=utf-8";
    std::string body;
    std::vector<std::pair<std::string, std::string>> headers;

    static HttpResponse text(int status, const std::string& msg);
    static HttpResponse json(const std::string& body, int status = 200);
    static HttpResponse jsonError(int status, const std::string& msg);
    static HttpResponse jsonOk();
};

using HttpHandler = std::function<HttpResponse(const HttpRequest&)>;

class HttpServer {
public:
    explicit HttpServer(HttpHandler handler) : handler_(std::move(handler)) {}
    ~HttpServer();

    // Bind and listen on 0.0.0.0:port. Returns false and fills err on failure.
    bool start(int port, std::string& err);
    int port() const { return port_; }
    // Wait up to timeoutMs for a connection; handle at most a few per call.
    void poll(int timeoutMs);
    void stop();
    bool running() const { return listenFd_ >= 0; }
    // True once the listening socket has failed (e.g. the network dropped).
    bool broken() const { return broken_; }

    // Largest request body accepted (uploads come later; icons are small JPEGs).
    static constexpr size_t kMaxBody = 8 * 1024 * 1024;

private:
    void handleClient(int fd);
    HttpHandler handler_;
    int listenFd_ = -1;
    int port_ = 0;
    bool broken_ = false;
};

// Helpers shared by route handlers
std::string jsonEscape(const std::string& s);
std::string mimeForPath(const std::string& path);
std::string urlDecode(const std::string& s);
// Looks up `name` in an application/x-www-form-urlencoded string.
bool formValue(const std::string& data, const std::string& name, std::string& out);
