// RESTRouter.cpp - تطبيق Router لـ REST APIs
// الأسبوع الأول من الشهر الخامس: خادم ويب بسيط
#include "RESTRouter.h"
#include <sstream>
#include <iostream>

namespace ArabicLanguage {

// ==================== RESTResource Implementation ====================

RESTResource::RESTResource(HTTPServer* srv, const std::string& path) 
    : server(srv), basePath(path) {
}

void RESTResource::setGetHandler(RequestHandler handler) {
    server->get(basePath, handler);
}

void RESTResource::setPostHandler(RequestHandler handler) {
    server->post(basePath, handler);
}

void RESTResource::setPutHandler(RequestHandler handler) {
    server->put(basePath, handler);
}

void RESTResource::setDeleteHandler(RequestHandler handler) {
    server->del(basePath, handler);
}

void RESTResource::setPatchHandler(RequestHandler handler) {
    server->patch(basePath, handler);
}

void RESTResource::addCustomRoute(HTTPMethod method, const std::string& subPath, RequestHandler handler) {
    std::string fullPath = basePath + subPath;
    server->addRoute(method, fullPath, handler);
}

// ==================== RESTRouter Implementation ====================

RESTRouter::RESTRouter(HTTPServer* srv, const std::string& prefix) 
    : server(srv), apiPrefix(prefix) {
}

RESTResource* RESTRouter::addResource(const std::string& name, const std::string& path) {
    std::string resourcePath = apiPrefix + (path.empty() ? "/" + name : path);
    auto resource = std::make_unique<RESTResource>(server, resourcePath);
    RESTResource* ptr = resource.get();
    resources[name] = std::move(resource);
    return ptr;
}

RESTResource* RESTRouter::getResource(const std::string& name) {
    auto it = resources.find(name);
    return it != resources.end() ? it->second.get() : nullptr;
}

void RESTRouter::get(const std::string& path, RequestHandler handler) {
    server->get(apiPrefix + path, handler);
}

void RESTRouter::post(const std::string& path, RequestHandler handler) {
    server->post(apiPrefix + path, handler);
}

void RESTRouter::put(const std::string& path, RequestHandler handler) {
    server->put(apiPrefix + path, handler);
}

void RESTRouter::del(const std::string& path, RequestHandler handler) {
    server->del(apiPrefix + path, handler);
}

void RESTRouter::patch(const std::string& path, RequestHandler handler) {
    server->patch(apiPrefix + path, handler);
}

void RESTRouter::getJSON(const std::string& path, std::function<std::string(const HTTPRequest&)> handler) {
    get(path, [handler](const HTTPRequest& req) -> HTTPResponse {
        HTTPResponse res;
        try {
            std::string json = handler(req);
            res.setJSON(json);
            res.setStatus(HTTPStatus::OK);
        } catch (const std::exception& e) {
            res.setJSON(jsonError(e.what()));
            res.setStatus(HTTPStatus::INTERNAL_SERVER_ERROR);
        }
        return res;
    });
}

void RESTRouter::postJSON(const std::string& path, std::function<std::string(const HTTPRequest&)> handler) {
    post(path, [handler](const HTTPRequest& req) -> HTTPResponse {
        HTTPResponse res;
        try {
            std::string json = handler(req);
            res.setJSON(json);
            res.setStatus(HTTPStatus::CREATED);
        } catch (const std::exception& e) {
            res.setJSON(jsonError(e.what()));
            res.setStatus(HTTPStatus::BAD_REQUEST);
        }
        return res;
    });
}

// ==================== Helper Functions ====================

std::string jsonResponse(const std::map<std::string, std::string>& data) {
    std::stringstream ss;
    ss << "{";
    bool first = true;
    for (const auto& pair : data) {
        if (!first) ss << ",";
        ss << "\"" << pair.first << "\":\"" << pair.second << "\"";
        first = false;
    }
    ss << "}";
    return ss.str();
}

std::string jsonError(const std::string& message, int code) {
    std::stringstream ss;
    ss << "{\"error\":true,\"code\":" << code << ",\"message\":\"" << message << "\"}";
    return ss.str();
}

std::string jsonSuccess(const std::string& message, const std::map<std::string, std::string>& data) {
    std::stringstream ss;
    ss << "{\"success\":true,\"message\":\"" << message << "\"";
    if (!data.empty()) {
        ss << ",\"data\":{";
        bool first = true;
        for (const auto& pair : data) {
            if (!first) ss << ",";
            ss << "\"" << pair.first << "\":\"" << pair.second << "\"";
            first = false;
        }
        ss << "}";
    }
    ss << "}";
    return ss.str();
}

} // namespace ArabicLanguage

