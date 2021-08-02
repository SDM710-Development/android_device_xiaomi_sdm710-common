/*
 * Copyright (C) 2018-2020 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "LightsService"

#include "Light.h"

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <fstream>

namespace android {
namespace hardware {
namespace light {
namespace V2_0 {
namespace implementation {

static constexpr int kDefaultMaxBrightness = 255;
static constexpr int kRampSteps = 8;
static constexpr int kRampMaxStepDurationMs = 50;

// Loop: off ... just 1 cycle
//        on ... repeat
static constexpr int fLutLoop = 1;

// Ramp up: off ... LED is initially ON (ramp down follows)
//           on ... LED is initially OFF (ramp up follows)
static constexpr int fLutRampUp = 2;

// Reverse: off ... only single direction
//                  (e.g. off -> ramp-up -> on -> off -> ramp-up -> ...)
//           on ... full cycle
//                  (e.g. off -> ramp-up -> on -> ramp-down -> off -> ...)
static constexpr int fLutReverse = 4;

// Pause Hi: off ... no pause after ramp-up
//            on ... pause after ramp-up (configured pause_hi value)
static constexpr int fLutPauseHi = 8;

// Pause Lo: off ... no pause after ramp-down
//            on ... pause after ramp-down (configured pause_lo value)
static constexpr int fLutPauseLo = 16;

// Default LUT flags: start with off, full cycle and repeat
static constexpr int fLutDefault = fLutLoop | fLutRampUp | fLutReverse;

static bool isLedExist(const std::string& led)
{
    std::string path = "/sys/class/leds/";
    std::ofstream file;

    path += led + "/brightness";

    // try to open for write
    file.open(path);

    return !file.fail();
}

/*
 * Returns brightness for specified component where for
 * component == 0 ... blue modulated by alpha,
 * component == 1 ... green modulated by alpha,
 * component == 2 ... red modulated by alpha,
 * otherwise ... RGB mix modulated by alpha
 */
static uint32_t getBrightness(int component, const LightState& state) {
    uint32_t alpha, red, green, blue;

    // Extract brightness from AARRGGBB
    alpha = (state.color >> 24) & 0xff;

    // Retrieve each of the RGB colors
    red = (state.color >> 16) & 0xff;
    green = (state.color >> 8) & 0xff;
    blue = state.color & 0xff;

    // Scale RGB colors if a brightness has been applied by the user
    if (alpha != 0xff) {
        red = red * alpha / 0xff;
        green = green * alpha / 0xff;
        blue = blue * alpha / 0xff;
    }

    switch (component) {
    case 0: return blue;
    case 1: return green;
    case 2: return red;
    }

    return (77 * red + 150 * green + 29 * blue) >> 8;
}

Light::Light() {
    mLights.emplace(Type::ATTENTION, std::bind(&Light::handleLed, this,
                                               std::placeholders::_1,
                                               std::placeholders::_2, 0));
    mLights.emplace(Type::BATTERY, std::bind(&Light::handleLed, this,
                                             std::placeholders::_1,
                                             std::placeholders::_2, 2));
    mLights.emplace(Type::NOTIFICATIONS, std::bind(&Light::handleLed, this,
                                                   std::placeholders::_1,
                                                   std::placeholders::_2, 1));

    if (isLedExist("blue") && isLedExist("green") && isLedExist("red")) {
        mLeds.push_back("blue");
        mLeds.push_back("green");
        mLeds.push_back("red");
    } else if (isLedExist("white")) {
        mLeds.push_back("white");
    }
}

void Light::handleLed(int led, const LightState& state, size_t index) {
    // Update light state with specified index
    mLightStates.at(index) = state;

    // Select light state (lower index -> higher priority)
    LightState stateToUse = mLightStates.front();
    for (const auto& lightState : mLightStates) {
        if (lightState.color & 0xffffff) {
            stateToUse = lightState;
            break;
        }
    }

    // If number of leds is 1 then request brightness for RGB mix
    // otherwise get brightness for color component
    uint32_t brightness = getBrightness(mLeds.size() > 1 ? led : -1,
                                        stateToUse);

    auto getScaledDutyPercent = [](int value) -> std::string {
        std::string output;
        for (int i = 0; i <= kRampSteps; i++) {
            if (i != 0) {
                output += ",";
            }
            output += std::to_string(i * 100 * value /
                                     (kDefaultMaxBrightness * kRampSteps));
        }
        return output;
    };

    // Disable blinking to start
    setLedParam(led, "blink", 0);

    if (stateToUse.flashMode == Flash::TIMED) {
        // Use default flags for LUT
        int lutFlags = fLutDefault;

        // Use default ramp step duration
        int stepDuration = kRampMaxStepDurationMs;

        // Check if default ramp step duration is too long for either
        // flashOnMs or flashOffMs and modify it if so
        int len = std::min(stateToUse.flashOnMs, stateToUse.flashOffMs);
        if (len < stepDuration * kRampSteps)
                stepDuration = len / kRampSteps;

        // Compute pauses
        int pauseHi = stateToUse.flashOnMs - stepDuration * kRampSteps;
        int pauseLo = stateToUse.flashOffMs - stepDuration * kRampSteps;

        // Enable LUT pause flags if they should be used
        lutFlags |= pauseHi ? fLutPauseHi : 0;
        lutFlags |= pauseLo ? fLutPauseLo : 0;

        setLedParam(led, "start_idx", 0);
        setLedParam(led, "duty_pcts", getScaledDutyPercent(brightness));
        setLedParam(led, "pause_lo", pauseLo);
        setLedParam(led, "pause_hi", pauseHi);
        setLedParam(led, "ramp_step_ms", stepDuration);
        setLedParam(led, "lut_flags", lutFlags);

        // Start blinking
        setLedParam(led, "blink", 1);
    } else {
        setLedParam(led, "brightness", brightness);
    }
}

template <typename T>
void Light::setLedParam(int led, const std::string& param, const T& value)
{
    std::string path = "/sys/class/leds/";

    if (led < mLeds.size()) {
        std::ofstream file(path + mLeds[led] + "/" + param);

        file << value;
    }
}

Return<Status> Light::setLight(Type type, const LightState& state) {
    auto it = mLights.find(type);

    if (it == mLights.end()) {
        return Status::LIGHT_NOT_SUPPORTED;
    }

    // Lock global mutex until light state is updated.
    std::lock_guard<std::mutex> lock(mLock);

    for (int led = 0; led < mLeds.size(); led++)
        it->second(led, state);

    return Status::SUCCESS;
}

Return<void> Light::getSupportedTypes(getSupportedTypes_cb _hidl_cb) {
    std::vector<Type> types;

    for (auto const& light : mLights) {
        types.push_back(light.first);
    }

    _hidl_cb(types);

    return Void();
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace light
}  // namespace hardware
}  // namespace android
