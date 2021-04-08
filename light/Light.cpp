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
static constexpr int kRampSteps = 50;
static constexpr int kRampMaxStepDurationMs = 5;

static bool isLedExist(const std::string& led)
{
    std::string path = "/sys/class/leds/";
    std::ofstream file;

    path += led + "/brightness";

    // try to open for write
    file.open(path);

    return !file.fail();
}

static uint32_t getBrightness(const LightState& state) {
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

    return (77 * red + 150 * green + 29 * blue) >> 8;
}

Light::Light() {
    mLights.emplace(Type::ATTENTION, std::bind(&Light::handleNotification, this, std::placeholders::_1, 0));
    mLights.emplace(Type::BATTERY, std::bind(&Light::handleBattery, this, std::placeholders::_1));
    mLights.emplace(Type::NOTIFICATIONS, std::bind(&Light::handleNotification, this, std::placeholders::_1, 1));

    if (isLedExist("white")) {
        mLeds.push_back("white");
    }
}

void Light::handleBattery(const LightState& state) {
    uint32_t whiteBrightness = getBrightness(state);

    uint32_t onMs = state.flashMode == Flash::TIMED ? state.flashOnMs : 0;
    uint32_t offMs = state.flashMode == Flash::TIMED ? state.flashOffMs : 0;

    auto getScaledDutyPercent = [](int brightness) -> std::string {
        std::string output;
        for (int i = 0; i <= kRampSteps; i++) {
            if (i != 0) {
                output += ",";
            }
            if (i <= kRampSteps / 2) {
                output += "0";
            } else {
                output += std::to_string((i - kRampSteps / 2) * 100 * brightness /
                                         (kDefaultMaxBrightness * (kRampSteps/2)));
            }
        }
        return output;
    };

    // Disable blinking to start
    setLedParam(0, "blink", 0);

    if (onMs > 0 && offMs > 0) {
        uint32_t pauseLo, pauseHi, stepDuration;
        stepDuration = 10;
        if (stepDuration * kRampSteps > onMs) {
            pauseHi = 0;
        } else {
            pauseHi = onMs - kRampSteps * stepDuration;
            pauseLo = offMs - kRampSteps * stepDuration;
        }

        setLedParam(0, "start_idx", 0);
        setLedParam(0, "duty_pcts", getScaledDutyPercent(whiteBrightness));
        setLedParam(0, "pause_lo", pauseLo);
        setLedParam(0, "pause_hi", pauseHi);
        setLedParam(0, "ramp_step_ms", stepDuration);

        // Start blinking
        setLedParam(0, "blink", 1);
    } else {
        setLedParam(0, "brightness", whiteBrightness);
    }
}

void Light::handleNotification(const LightState& state, size_t index) {
    mLightStates.at(index) = state;

    LightState stateToUse = mLightStates.front();
    for (const auto& lightState : mLightStates) {
        if (lightState.color & 0xffffff) {
            stateToUse = lightState;
            break;
        }
    }

    uint32_t whiteBrightness = getBrightness(stateToUse);

    uint32_t onMs = stateToUse.flashMode == Flash::TIMED ? stateToUse.flashOnMs : 0;
    uint32_t offMs = stateToUse.flashMode == Flash::TIMED ? stateToUse.flashOffMs : 0;

    auto getScaledDutyPercent = [](int brightness) -> std::string {
        std::string output;
        for (int i = 0; i <= kRampSteps; i++) {
            if (i != 0) {
                output += ",";
            }
            output += std::to_string(i * 100 * brightness / (kDefaultMaxBrightness * kRampSteps));
        }
        return output;
    };

    // Disable blinking to start
    setLedParam(0, "blink", 0);

    if (onMs > 0 && offMs > 0) {
        uint32_t pauseLo, pauseHi, stepDuration;
        if (kRampMaxStepDurationMs * kRampSteps > onMs) {
            stepDuration = onMs / kRampSteps;
            pauseHi = 0;
        } else {
            stepDuration = kRampMaxStepDurationMs;
            pauseHi = onMs - kRampSteps * stepDuration;
            pauseLo = offMs - kRampSteps * stepDuration;
        }

        setLedParam(0, "start_idx", 0);
        setLedParam(0, "duty_pcts", getScaledDutyPercent(whiteBrightness));
        setLedParam(0, "pause_lo", pauseLo);
        setLedParam(0, "pause_hi", pauseHi);
        setLedParam(0, "ramp_step_ms", stepDuration);

        // Start blinking
        setLedParam(0, "blink", 1);
    } else {
        setLedParam(0, "brightness", whiteBrightness);
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

    it->second(state);

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
