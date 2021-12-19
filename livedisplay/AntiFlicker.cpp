/*
 * Copyright (C) 2021 The LineageOS Project
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

#define LOG_TAG "AntiFlickerService"

#include "AntiFlicker.h"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/strings.h>
#include <utils/Errors.h>
#include <fstream>

namespace vendor {
namespace lineage {
namespace livedisplay {
namespace V2_1 {
namespace implementation {

static constexpr const char* kDcDimmingPath =
        "/sys/devices/platform/soc/soc:qcom,dsi-display-primary/dc_dimming";

bool AntiFlicker::isSupported() {
    std::ofstream dc_dimming_file(kDcDimmingPath);
    if (!dc_dimming_file.is_open()) {
        LOG(ERROR) << "Failed to open " << kDcDimmingPath << ", error="
                   << errno << " (" << strerror(errno) << ")";
    }

    return !dc_dimming_file.fail();
}

Return<bool> AntiFlicker::isEnabled() {
    std::ifstream dc_dimming_file(kDcDimmingPath);
    int result = -1;
    dc_dimming_file >> result;
    return !dc_dimming_file.fail() && result > 0;
}

Return<bool> AntiFlicker::setEnabled(bool enabled) {
    std::ofstream dc_dimming_file(kDcDimmingPath);
    dc_dimming_file << (enabled ? '1' : '0');
    LOG(DEBUG) << "setEnabled fail " << dc_dimming_file.fail();
    return !dc_dimming_file.fail();
}

}  // namespace implementation
}  // namespace V2_1
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor
