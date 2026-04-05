// ArabicInteractiveEdu.h - أدوات التعليم التفاعلي باللغة العربية
#ifndef ARABIC_INTERACTIVE_EDU_H
#define ARABIC_INTERACTIVE_EDU_H

#include <string>
#include <vector>
#include <functional>

namespace ArabicEdu {

    // أنواع الأنشطة التفاعلية
    enum class ActivityType {
        SIMULATOR,      // محاكي
        CODING_CHALLENGE, // تحدي برمجي
        EDU_GAME        // لعبة تعليمية
    };

    struct InteractiveActivity {
        int id;
        std::string title;
        ActivityType type;
        std::string description;
    };

    class InteractiveManager {
    public:
        // إدارة الأنشطة
        static int registerActivity(const std::string& title, ActivityType type, const std::string& desc);
        static void startSimulator(int activityId);
        
        // تحديات البرمجة التفاعلية
        static bool runCodingExercise(int activityId, const std::string& studentCode, const std::string& expectedOutput);
        
        // الألعاب التعليمية
        static void playEduGame(int activityId);
        static void onGameScoreUpdate(int activityId, int score);

        // واجهة المحاكاة (Callback للرسوميات أو المنطق)
        static void setSimulatorUpdateCallback(std::function<void(float)> callback);

    private:
        static std::vector<InteractiveActivity> activities;
        static std::function<void(float)> simCallback;
    };

} // namespace ArabicEdu

#endif // ARABIC_INTERACTIVE_EDU_H
