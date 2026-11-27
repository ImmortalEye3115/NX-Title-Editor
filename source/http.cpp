// NX Title Editor
// Copyright (C) 2026 ImmortalEye
// SPDX-License-Identifier: GPL-3.0-or-later

#include "http.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <cerrno>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

const char* statusText(int status) {
    switch (status) {
        case 200: return "OK";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 415: return "Unsupported Media Type";
        case 500: return "Internal Server Error";
        case 502: return "Bad Gateway";
        default:  return "Unknown";
    }
}

std::string lower(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

bool sendAll(int fd, const char* data, size_t len) {
    while (len > 0) {
        ssize_t n = send(fd, data, len, 0);
        if (n <= 0) return false;
        data += n;
        len -= (size_t)n;
    }
    return true;
}

void sendResponse(int fd, const HttpResponse& res) {
    std::string head;
    char line[128];
    snprintf(line, sizeof(line), "HTTP/1.1 %d %s\r\n", res.status, statusText(res.status));
    head += line;
    head += "Content-Type: " + res.contentType + "\r\n";
    snprintf(line, sizeof(line), "Content-Length: %zu\r\n", res.body.size());
    head += line;
    head += "Connection: close\r\n";
    for (const auto& h : res.headers) head += h.first + ": " + h.second + "\r\n";
    head += "\r\n";
    if (sendAll(fd, head.data(), head.size()))
        sendAll(fd, res.body.data(), res.body.size());
}

}  // namespace

std::string HttpRequest::header(const std::string& name) const {
    std::string key = lower(name);
    for (const auto& h : headers)
        if (h.first == key) return h.second;
    return "";
}

bool formValue(const std::string& data, const std::string& name, std::string& out) {
    size_t pos = 0;
    while (pos <= data.size()) {
        size_t amp = data.find('&', pos);
        if (amp == std::string::npos) amp = data.size();
        std::string part = data.substr(pos, amp - pos);
        size_t eq = part.find('=');
        if (!part.empty() && urlDecode(part.substr(0, eq)) == name) {
            out = eq == std::string::npos ? "" : urlDecode(part.substr(eq + 1));
            return true;
        }
        pos = amp + 1;
    }
    return false;
}

std::string HttpRequest::param(const std::string& name) const {
    std::string v;
    return formValue(query, name, v) ? v : "";
}

HttpResponse HttpResponse::jsonError(int status, const std::string& msg) {
    return json("{\"error\":\"" + jsonEscape(msg) + "\"}", status);
}

HttpResponse HttpResponse::jsonOk() { return json("{\"ok\":true}"); }

HttpResponse HttpResponse::text(int status, const std::string& msg) {
    HttpResponse r;
    r.status = status;
    r.body = msg;
    return r;
}

HttpResponse HttpResponse::json(const std::string& body, int status) {
    HttpResponse r;
    r.status = status;
    r.contentType = "application/json; charset=utf-8";
    r.body = body;
    r.headers.push_back({"Cache-Control", "no-store"});
    return r;
}

HttpServer::~HttpServer() { stop(); }

bool HttpServer::start(int port, std::string& err) {
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        err = "socket() failed";
        return false;
    }
    int yes = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);
    if (bind(listenFd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        err = "port in use";
        stop();
        return false;
    }
    if (listen(listenFd_, 16) < 0) {
        err = "listen() failed";
        stop();
        return false;
    }
    fcntl(listenFd_, F_SETFL, fcntl(listenFd_, F_GETFL, 0) | O_NONBLOCK);
    port_ = port;
    broken_ = false;
    return true;
}

void HttpServer::stop() {
    if (listenFd_ >= 0) {
        close(listenFd_);
        listenFd_ = -1;
    }
}

void HttpServer::poll(int timeoutMs) {
    if (listenFd_ < 0) return;
    pollfd pfd{listenFd_, POLLIN, 0};
    int ready = ::poll(&pfd, 1, timeoutMs);
    if (ready < 0 && errno != EINTR) {
        broken_ = true;
        return;
    }
    if (ready <= 0) return;
    // When the network drops, the Switch invalidates every socket. The
    // listener then reports an error forever instead of accepting, so flag
    // it for the main loop to reopen.
    if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
        broken_ = true;
        return;
    }

    // Drain a handful of queued connections per frame; browsers open several
    // at once when loading a grid of icons.
    for (int i = 0; i < 8; i++) {
        int fd = accept(listenFd_, nullptr, nullptr);
        if (fd < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR && errno != ECONNABORTED) broken_ = true;
            break;
        }
        // The client socket must block (with a timeout) even though the
        // listener doesn't.
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NONBLOCK);
        timeval tv{5, 0};
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        handleClient(fd);
        close(fd);
    }
}

void HttpServer::handleClient(int fd) {
    std::string buf;
    char chunk[4096];
    size_t headerEnd = std::string::npos;

    // Read until end of headers
    while (headerEnd == std::string::npos) {
        ssize_t n = recv(fd, chunk, sizeof(chunk), 0);
        if (n <= 0) return;
        buf.append(chunk, (size_t)n);
        headerEnd = buf.find("\r\n\r\n");
        if (headerEnd == std::string::npos && buf.size() > 32 * 1024) {
            sendResponse(fd, HttpResponse::text(400, "Headers too large"));
            return;
        }
    }

    HttpRequest req;
    std::string head = buf.substr(0, headerEnd);
    size_t lineEnd = head.find("\r\n");
    std::string requestLine = head.substr(0, lineEnd);

    // "GET /path?query HTTP/1.1"
    size_t sp1 = requestLine.find(' ');
    size_t sp2 = requestLine.find(' ', sp1 + 1);
    if (sp1 == std::string::npos || sp2 == std::string::npos) {
        sendResponse(fd, HttpResponse::text(400, "Bad request line"));
        return;
    }
    req.method = requestLine.substr(0, sp1);
    std::string target = requestLine.substr(sp1 + 1, sp2 - sp1 - 1);
    size_t q = target.find('?');
    req.path = target.substr(0, q);
    if (q != std::string::npos) req.query = target.substr(q + 1);

    size_t pos = (lineEnd == std::string::npos) ? head.size() : lineEnd + 2;
    while (pos < head.size()) {
        size_t e = head.find("\r\n", pos);
        if (e == std::string::npos) e = head.size();
        std::string line = head.substr(pos, e - pos);
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string val = line.substr(colon + 1);
            size_t start = val.find_first_not_of(" \t");
            req.headers.push_back({lower(line.substr(0, colon)),
                                   start == std::string::npos ? "" : val.substr(start)});
        }
        pos = e + 2;
    }

    size_t contentLength = 0;
    std::string cl = req.header("content-length");
    if (!cl.empty()) contentLength = strtoul(cl.c_str(), nullptr, 10);
    if (contentLength > kMaxBody) {
        sendResponse(fd, HttpResponse::text(413, "Request body too large"));
        return;
    }

    req.body = buf.substr(headerEnd + 4);
    while (req.body.size() < contentLength) {
        ssize_t n = recv(fd, chunk, sizeof(chunk), 0);
        if (n <= 0) return;
        req.body.append(chunk, (size_t)n);
    }
    if (req.body.size() > contentLength) req.body.resize(contentLength);

    sendResponse(fd, handler_(req));
}

std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char tmp[8];
                    snprintf(tmp, sizeof(tmp), "\\u%04x", c);
                    out += tmp;
                } else {
                    out += (char)c;  // UTF-8 passes through unchanged
                }
        }
    }
    return out;
}

std::string mimeForPath(const std::string& path) {
    auto ends = [&](const char* ext) {
        size_t n = strlen(ext);
        return path.size() >= n && path.compare(path.size() - n, n, ext) == 0;
    };
    if (ends(".html")) return "text/html; charset=utf-8";
    if (ends(".js"))   return "application/javascript; charset=utf-8";
    if (ends(".css"))  return "text/css; charset=utf-8";
    if (ends(".png"))  return "image/png";
    if (ends(".jpg") || ends(".jpeg")) return "image/jpeg";
    if (ends(".svg"))  return "image/svg+xml";
    if (ends(".json")) return "application/json; charset=utf-8";
    return "application/octet-stream";
}

std::string urlDecode(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if (c == '+') {
            out += ' ';
        } else if (c == '%' && i + 2 < s.size() && std::isxdigit((unsigned char)s[i + 1]) &&
                   std::isxdigit((unsigned char)s[i + 2])) {
            out += (char)strtol(s.substr(i + 1, 2).c_str(), nullptr, 16);
            i += 2;
        } else {
            out += c;
        }
    }
    return out;
}
