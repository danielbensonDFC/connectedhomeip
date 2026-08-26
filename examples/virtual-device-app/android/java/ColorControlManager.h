/*
 *
 *    Copyright (c) 2023 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
#pragma once

#include <app-common/zap-generated/cluster-objects.h>
#include <jni.h>
#include <lib/support/JniReferences.h>

/**
 * @brief Handles interfacing between java code and C++ code for the purposes of the Color Control
 * cluster (HueSaturation feature). Mirrors LevelControlManager. Like LevelControl, the manager is
 * installed explicitly from Java after server init (setColorControlManager) rather than via a
 * cluster init callback.
 */
class ColorControlManager
{
public:
    // install a bridge for a Color Control cluster endpoint and java object
    static void NewManager(jint endpoint, jobject manager);

    // helps java set attributes::CurrentHue of the Color Control cluster
    static jboolean SetCurrentHue(jint endpoint, uint8_t value);

    // helps java set attributes::CurrentSaturation of the Color Control cluster
    static jboolean SetCurrentSaturation(jint endpoint, uint8_t value);

    // posts a CurrentHue-changed event to the suitable ColorControlManager
    static void PostHueChanged(chip::EndpointId endpoint, uint8_t value);

    // posts a CurrentSaturation-changed event to the suitable ColorControlManager
    static void PostSaturationChanged(chip::EndpointId endpoint, uint8_t value);

    // handles `Changed` callbacks by calling the java `void handleHueChanged(int)` method
    void HandleHueChanged(uint8_t value);

    // handles `Changed` callbacks by calling the java `void handleSaturationChanged(int)` method
    void HandleSaturationChanged(uint8_t value);

private:
    // init with java objects
    CHIP_ERROR InitializeWithObjects(jobject managerObject);
    chip::JniGlobalReference mColorManagerObject;
    jmethodID mHandleHueChangedMethod        = nullptr;
    jmethodID mHandleSaturationChangedMethod = nullptr;
};
