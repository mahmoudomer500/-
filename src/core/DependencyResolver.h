// DependencyResolver.h - محلل وحلال تعارضات التبعيات
#ifndef DEPENDENCY_RESOLVER_H
#define DEPENDENCY_RESOLVER_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

namespace ArabicLanguage {

struct ComponentDependency {
    std::string componentName;
    std::string version;
    std::vector<std::string> dependencies;
    std::vector<std::string> conflicts;
    bool isSelfHosted;
    int priority; // أولوية التحميل (0-100)
};

struct DependencyConflict {
    std::string component1;
    std::string component2;
    std::string conflictType; // "version", "circular", "missing"
    std::string description;
    std::vector<std::string> resolutions;
};

enum class ResolutionStrategy {
    FORCE_UPDATE,
    DOWNGRADE,
    ISOLATE,
    REMOVE_CONFLICTING,
    MANUAL_RESOLUTION
};

// محلل وحلال تعارضات التبعيات
class DependencyResolver {
private:
    std::map<std::string, ComponentDependency> componentRegistry;
    std::vector<DependencyConflict> activeConflicts;
    std::map<std::string, std::set<std::string>> dependencyGraph;
    bool resolutionInProgress;

public:
    DependencyResolver();
    ~DependencyResolver() = default;

    // الواجهة العامة
    void registerComponent(const ComponentDependency& component);
    bool analyzeDependencies();
    std::vector<DependencyConflict> getConflicts() const;
    bool resolveConflicts(ResolutionStrategy strategy = ResolutionStrategy::FORCE_UPDATE);
    std::vector<std::string> getLoadOrder() const;
    bool validateSystemIntegrity() const;

    // دوال التشخيص
    void printDependencyGraph() const;
    void printConflicts() const;
    std::map<std::string, std::vector<std::string>> getDetailedAnalysis() const;

private:
    bool detectCircularDependencies();
    bool detectVersionConflicts();
    bool detectMissingDependencies();
    std::vector<std::string> topologicalSort() const;
    bool applyResolutionStrategy(const DependencyConflict& conflict, ResolutionStrategy strategy);
    void updateDependencyGraph();
    bool validateComponentCompatibility(const ComponentDependency& comp1,
                                      const ComponentDependency& comp2) const;
};

// Factory function لإنشاء محلل التبعيات
std::unique_ptr<DependencyResolver> createDependencyResolver();

} // namespace ArabicLanguage

#endif // DEPENDENCY_RESOLVER_H