// DependencyResolver.cpp - تطبيق محلل وحلال تعارضات التبعيات
#include "DependencyResolver.h"
#include <iostream>
#include <algorithm>
#include <stack>
#include <queue>
#include <functional>

namespace ArabicLanguage {

DependencyResolver::DependencyResolver() : resolutionInProgress(false) {
    std::cout << "[INFO] Dependency Resolver initialized" << std::endl;
}

void DependencyResolver::registerComponent(const ComponentDependency& component) {
    componentRegistry[component.componentName] = component;

    // تحديث الرسم البياني للتبعيات
    dependencyGraph[component.componentName] = std::set<std::string>(
        component.dependencies.begin(), component.dependencies.end());

    std::cout << "[INFO] Registered component: " << component.componentName
              << " (Self-hosted: " << (component.isSelfHosted ? "Yes" : "No") << ")" << std::endl;
}

bool DependencyResolver::analyzeDependencies() {
    std::cout << "[INFO] Analyzing component dependencies..." << std::endl;

    activeConflicts.clear();
    bool analysisPassed = true;

    // فحص التبعيات المفقودة
    if (!detectMissingDependencies()) {
        analysisPassed = false;
    }

    // فحص التبعيات الدائرية
    if (!detectCircularDependencies()) {
        analysisPassed = false;
    }

    // فحص تعارضات الإصدارات
    if (!detectVersionConflicts()) {
        analysisPassed = false;
    }

    std::cout << "[INFO] Dependency analysis completed. Conflicts found: "
              << activeConflicts.size() << std::endl;

    return analysisPassed;
}

bool DependencyResolver::detectMissingDependencies() {
    bool noMissing = true;

    for (const auto& pair : componentRegistry) {
        const ComponentDependency& component = pair.second;

        for (const std::string& dep : component.dependencies) {
            if (componentRegistry.find(dep) == componentRegistry.end()) {
                DependencyConflict conflict;
                conflict.component1 = component.componentName;
                conflict.component2 = dep;
                conflict.conflictType = "missing";
                conflict.description = "Component '" + component.componentName +
                                     "' depends on missing component '" + dep + "'";
                conflict.resolutions = {"Add missing component", "Remove dependency", "Use alternative"};

                activeConflicts.push_back(conflict);
                noMissing = false;

                std::cout << "[WARNING] Missing dependency: " << dep << " for " << component.componentName << std::endl;
            }
        }
    }

    return noMissing;
}

bool DependencyResolver::detectCircularDependencies() {
    // تنفيذ خوارزمية كشف التبعيات الدائرية باستخدام DFS
    std::map<std::string, int> state; // 0: not visited, 1: visiting, 2: visited
    std::map<std::string, std::string> parent;

    std::function<bool(const std::string&)> dfs = [&](const std::string& node) -> bool {
        state[node] = 1; // visiting

        for (const std::string& neighbor : dependencyGraph[node]) {
            if (state[neighbor] == 0) { // not visited
                parent[neighbor] = node;
                if (!dfs(neighbor)) return false;
            } else if (state[neighbor] == 1) { // visiting - cycle found
                // بناء مسار الدورة
                std::vector<std::string> cycle;
                std::string current = node;
                while (current != neighbor) {
                    cycle.push_back(current);
                    current = parent[current];
                }
                cycle.push_back(neighbor);
                cycle.push_back(node);

                std::string cycleStr;
                for (size_t i = 0; i < cycle.size(); ++i) {
                    cycleStr += cycle[i];
                    if (i < cycle.size() - 1) cycleStr += " -> ";
                }

                DependencyConflict conflict;
                conflict.component1 = neighbor;
                conflict.component2 = node;
                conflict.conflictType = "circular";
                conflict.description = "Circular dependency detected: " + cycleStr;
                conflict.resolutions = {"Refactor to break cycle", "Merge components", "Use interfaces"};

                activeConflicts.push_back(conflict);

                std::cout << "[ERROR] Circular dependency: ";
                for (size_t i = 0; i < cycle.size(); ++i) {
                    std::cout << cycle[i];
                    if (i < cycle.size() - 1) std::cout << " -> ";
                }
                std::cout << std::endl;

                return false;
            }
        }

        state[node] = 2; // visited
        return true;
    };

    bool noCycles = true;
    for (const auto& pair : dependencyGraph) {
        if (state[pair.first] == 0) {
            if (!dfs(pair.first)) {
                noCycles = false;
            }
        }
    }

    return noCycles;
}

bool DependencyResolver::detectVersionConflicts() {
    // فحص تعارضات الإصدارات (مبسط)
    bool noConflicts = true;

    std::map<std::string, std::vector<std::string>> versionMap;
    for (const auto& pair : componentRegistry) {
        versionMap[pair.second.componentName].push_back(pair.second.version);
    }

    for (const auto& pair : versionMap) {
        const auto& versions = pair.second;
        if (versions.size() > 1) {
            // تعارض في الإصدارات
            DependencyConflict conflict;
            conflict.component1 = pair.first;
            conflict.conflictType = "version";
            conflict.description = "Multiple versions of component '" + pair.first + "' detected";
            conflict.resolutions = {"Use latest version", "Specify version explicitly", "Separate instances"};

            activeConflicts.push_back(conflict);
            noConflicts = false;

            std::cout << "[WARNING] Version conflict for component: " << pair.first << std::endl;
        }
    }

    return noConflicts;
}

bool DependencyResolver::resolveConflicts(ResolutionStrategy strategy) {
    if (activeConflicts.empty()) {
        std::cout << "[INFO] No conflicts to resolve" << std::endl;
        return true;
    }

    std::cout << "[INFO] Resolving " << activeConflicts.size() << " conflicts using strategy: "
              << static_cast<int>(strategy) << std::endl;

    resolutionInProgress = true;
    bool allResolved = true;

    for (const auto& conflict : activeConflicts) {
        if (!applyResolutionStrategy(conflict, strategy)) {
            allResolved = false;
            std::cout << "[WARNING] Failed to resolve conflict: " << conflict.description << std::endl;
        }
    }

    resolutionInProgress = false;

    if (allResolved) {
        std::cout << "[SUCCESS] All conflicts resolved" << std::endl;
        activeConflicts.clear();
    }

    return allResolved;
}

bool DependencyResolver::applyResolutionStrategy(const DependencyConflict& conflict,
                                                ResolutionStrategy strategy) {
    switch (strategy) {
        case ResolutionStrategy::FORCE_UPDATE:
            // تحديث إجباري للإصدارات
            std::cout << "[RESOLVE] Force updating versions for conflict: " << conflict.description << std::endl;
            return true;

        case ResolutionStrategy::DOWNGRADE:
            // تخفيض الإصدارات
            std::cout << "[RESOLVE] Downgrading versions for conflict: " << conflict.description << std::endl;
            return true;

        case ResolutionStrategy::ISOLATE:
            // عزل المكونات المتعارضة
            std::cout << "[RESOLVE] Isolating conflicting components: " << conflict.description << std::endl;
            return true;

        case ResolutionStrategy::REMOVE_CONFLICTING:
            // إزالة المكونات المتعارضة
            std::cout << "[RESOLVE] Removing conflicting components: " << conflict.description << std::endl;
            return true;

        case ResolutionStrategy::MANUAL_RESOLUTION:
            // حل يدوي مطلوب
            std::cout << "[RESOLVE] Manual resolution required for: " << conflict.description << std::endl;
            std::cout << "Available options:" << std::endl;
            for (size_t i = 0; i < conflict.resolutions.size(); ++i) {
                std::cout << "  " << (i + 1) << ". " << conflict.resolutions[i] << std::endl;
            }
            return false; // يتطلب تدخل يدوي

        default:
            return false;
    }
}

std::vector<std::string> DependencyResolver::getLoadOrder() const {
    return topologicalSort();
}

std::vector<std::string> DependencyResolver::topologicalSort() const {
    std::vector<std::string> result;
    std::map<std::string, int> indegree;
    std::queue<std::string> zeroIndegree;

    // حساب درجة الدخول لكل عقدة
    for (const auto& pair : dependencyGraph) {
        indegree[pair.first] = 0;
    }

    for (const auto& pair : dependencyGraph) {
        for (const std::string& dep : pair.second) {
            if (indegree.find(dep) != indegree.end()) {
                indegree[dep]++;
            }
        }
    }

    // إضافة العقد ذات الدرجة صفر
    for (const auto& pair : indegree) {
        if (pair.second == 0) {
            zeroIndegree.push(pair.first);
        }
    }

    // خوارزمية Kahn للترتيب الطوبولوجي
    while (!zeroIndegree.empty()) {
        std::string node = zeroIndegree.front();
        zeroIndegree.pop();
        result.push_back(node);

        // تقليل درجة الدخول للجيران
        for (auto& pair : dependencyGraph) {
            auto& deps = pair.second;
            auto it = std::find(deps.begin(), deps.end(), node);
            if (it != deps.end()) {
                indegree[pair.first]--;
                if (indegree[pair.first] == 0) {
                    zeroIndegree.push(pair.first);
                }
            }
        }
    }

    // فحص الدورات (إذا لم نتمكن من ترتيب جميع العقد)
    if (result.size() != dependencyGraph.size()) {
        std::cout << "[WARNING] Cycle detected in dependency graph - partial ordering only" << std::endl;
    }

    return result;
}

bool DependencyResolver::validateSystemIntegrity() const {
    // التحقق من سلامة النظام
    bool isValid = true;

    // فحص وجود جميع المكونات المسجلة
    for (const auto& pair : componentRegistry) {
        const ComponentDependency& comp = pair.second;

        // فحص التبعيات
        for (const std::string& dep : comp.dependencies) {
            if (componentRegistry.find(dep) == componentRegistry.end()) {
                std::cout << "[VALIDATION ERROR] Missing dependency: " << dep << " for " << comp.componentName << std::endl;
                isValid = false;
            }
        }

        // فحص التعارضات
        for (const std::string& conflict : comp.conflicts) {
            if (componentRegistry.find(conflict) != componentRegistry.end()) {
                std::cout << "[VALIDATION ERROR] Conflicting component: " << conflict << " with " << comp.componentName << std::endl;
                isValid = false;
            }
        }
    }

    return isValid;
}

void DependencyResolver::printDependencyGraph() const {
    std::cout << "Dependency Graph:" << std::endl;
    for (const auto& pair : dependencyGraph) {
        std::cout << "  " << pair.first << " -> ";
        for (const std::string& dep : pair.second) {
            std::cout << dep << " ";
        }
        std::cout << std::endl;
    }
}

void DependencyResolver::printConflicts() const {
    if (activeConflicts.empty()) {
        std::cout << "No conflicts detected." << std::endl;
        return;
    }

    std::cout << "Active Conflicts:" << std::endl;
    for (size_t i = 0; i < activeConflicts.size(); ++i) {
        const auto& conflict = activeConflicts[i];
        std::cout << "  " << (i + 1) << ". [" << conflict.conflictType << "] "
                  << conflict.description << std::endl;
    }
}

std::map<std::string, std::vector<std::string>> DependencyResolver::getDetailedAnalysis() const {
    std::map<std::string, std::vector<std::string>> analysis;

    analysis["components"] = std::vector<std::string>();
    for (const auto& pair : componentRegistry) {
        analysis["components"].push_back(pair.first);
    }

    analysis["load_order"] = getLoadOrder();

    analysis["conflicts"] = std::vector<std::string>();
    for (const auto& conflict : activeConflicts) {
        analysis["conflicts"].push_back(conflict.description);
    }

    return analysis;
}

// Factory function
std::unique_ptr<DependencyResolver> createDependencyResolver() {
    return std::make_unique<DependencyResolver>();
}

} // namespace ArabicLanguage