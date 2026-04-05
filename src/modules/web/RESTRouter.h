// RESTRouter.h - Router لـ REST APIs
// الأسبوع الأول من الشهر الخامس: خادم ويب بسيط
#ifndef REST_ROUTER_H
#define REST_ROUTER_H

#include "HTTPServer.h"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>

namespace ArabicLanguage {

// مورد REST (Resource)
class RESTResource {
private:
    std::string basePath;
    HTTPServer* server;

public:
    RESTResource(HTTPServer* srv, const std::string& path);
    
    // معالجات CRUD
    void setGetHandler(RequestHandler handler);
    void setPostHandler(RequestHandler handler);
    void setPutHandler(RequestHandler handler);
    void setDeleteHandler(RequestHandler handler);
    void setPatchHandler(RequestHandler handler);
    
    // معالجات مخصصة
    void addCustomRoute(HTTPMethod method, const std::string& subPath, RequestHandler handler);
    
    std::string getBasePath() const { return basePath; }
};

// Router لـ REST APIs
class RESTRouter {
private:
    HTTPServer* server;
    std::map<std::string, std::unique_ptr<RESTResource>> resources;
    std::string apiPrefix;

public:
    RESTRouter(HTTPServer* srv, const std::string& prefix = "/api");
    
    // إدارة الموارد
    RESTResource* addResource(const std::string& name, const std::string& path = "");
    RESTResource* getResource(const std::string& name);
    
    // مسارات سريعة
    void get(const std::string& path, RequestHandler handler);
    void post(const std::string& path, RequestHandler handler);
    void put(const std::string& path, RequestHandler handler);
    void del(const std::string& path, RequestHandler handler);
    void patch(const std::string& path, RequestHandler handler);
    
    // معالجات JSON
    void getJSON(const std::string& path, std::function<std::string(const HTTPRequest&)> handler);
    void postJSON(const std::string& path, std::function<std::string(const HTTPRequest&)> handler);
    
    std::string getAPIPrefix() const { return apiPrefix; }
};

// Helper functions لـ JSON
std::string jsonResponse(const std::map<std::string, std::string>& data);
std::string jsonError(const std::string& message, int code = 400);
std::string jsonSuccess(const std::string& message, const std::map<std::string, std::string>& data = {});

} // namespace ArabicLanguage

#endif // REST_ROUTER_H

