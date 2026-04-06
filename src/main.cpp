#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/FLAlertLayer.hpp>

using namespace geode::prelude;

// handles all timing + reminder logic
class WaterReminder {
public:
    static inline float timePassed = 0.0f;
    static inline int queuedReminders = 0;
    static inline float popupCooldown = 0.0f;

    static float getIntervalSeconds() {
        return Mod::get()->getSettingValue<int>("interval") * 60.0f;
    }

    static std::string getReminderText() {
        return Mod::get()->getSettingValue<std::string>("message");
    }

    static bool shouldPlaySound() {
        return Mod::get()->getSettingValue<bool>("sound");
    }

    static void loadState() {
        timePassed = Mod::get()->getSavedValue<float>("saved-time", 0.0f);
    }

    static void saveState() {
        Mod::get()->setSavedValue("saved-time", timePassed);
    }

    // update timer — slower in menu so it doesn’t feel annoying
    static void tick(float dt, bool inLevel) {
        if (inLevel)
            timePassed += dt;
        else
            timePassed += dt * 0.5f;

        if (popupCooldown > 0.0f)
            popupCooldown -= dt;

        if (timePassed >= getIntervalSeconds()) {
            queuedReminders++;
            timePassed = 0.0f;
            saveState();
        }
    }

    // only show when it's not disruptive (death, pause, etc)
    static void maybeShowWaterPopup() {
        if (queuedReminders > 0 && popupCooldown <= 0.0f) {

            std::string msg = getReminderText();

            // if user ignored multiple reminders, escalate tone
            if (queuedReminders > 3) {
                msg = "go drink water 💀";
            }

            FLAlertLayer::create(
                "Water Reminder 💧",
                msg.c_str(),
                "OK"
            )->show();

            if (shouldPlaySound()) {
                FMODAudioEngine::sharedEngine()->playEffect("quitSound_01.ogg");
            }

            queuedReminders--;

            // random cooldown
            popupCooldown = 8.0f + (rand() % 5);
        }
    }
};

// Menu layer

class $modify(MyMenuLayer, MenuLayer) {

    bool init() {
        if (!MenuLayer::init()) return false;

        WaterReminder::loadState();

        // run every second
        this->schedule(schedule_selector(MyMenuLayer::updateTimer), 1.0f);

        return true;
    }

    void updateTimer(float dt) {
        WaterReminder::tick(dt, false);
        WaterReminder::maybeShowWaterPopup();
    }
};

// Play Layer

class $modify(MyPlayLayer, PlayLayer) {

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) 
            return false;

        this->schedule(schedule_selector(MyPlayLayer::updateTimer), 1.0f);

        return true;
    }

    void updateTimer(float dt) {
        WaterReminder::tick(dt, true);
    }

    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        PlayLayer::destroyPlayer(player, obj);
        WaterReminder::maybeShowWaterPopup();
    }

    void levelComplete() {
        PlayLayer::levelComplete();
        WaterReminder::maybeShowWaterPopup();
    }

    void pauseGame(bool p0) {
        PlayLayer::pauseGame(p0);
        WaterReminder::maybeShowWaterPopup();
    }
};