/*
 * Copyright (C) 2019 The LineageOS Project
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

#define LOG_TAG "SunlightEnhancementService"

#include <fstream>

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/strings.h>
#include <utils/Errors.h>

#include "SunlightEnhancement.h"

namespace vendor {
namespace lineage {
namespace livedisplay {
namespace V2_1 {
namespace implementation {

static constexpr const char* kHbmPath =
        "/sys/devices/platform/soc/soc:qcom,dsi-display-primary/hbm";

bool SunlightEnhancement::isSupported() {
    std::ofstream hbm_file(kHbmPath);
    if (!hbm_file.is_open()) {
        LOG(ERROR) << "Failed to open " << kHbmPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
    }

    return !hbm_file.fail();
}

Return<bool> SunlightEnhancement::isEnabled() {
    std::ifstream hbm_file(kHbmPath);
    int result = -1;
    hbm_file >> result;
    return !hbm_file.fail() && result > 0;
}

Return<bool> SunlightEnhancement::setEnabled(bool enabled) {
    std::ofstream hbm_file(kHbmPath);
    hbm_file << (enabled ? '1' : '0');
    LOG(DEBUG) << "setEnabled fail " << hbm_file.fail();
    return !hbm_file.fail();
}

}  // namespace implementation
}  // namespace V2_1
}  // namespace livedisplay
}  // namespace lineage
}  // namespace vendor
