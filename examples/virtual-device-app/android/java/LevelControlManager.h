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
 * @brief Handles interfacing between java code and C++ code for the purposes of the Level Control
 * cluster. Mirrors OnOffManager. Unlike OnOff, the manager is installed explicitly from Java after
 * server init (setLevelControlManager) rather than via a cluster init callback.
 */
class LevelControlManager
{
public:
    // install a bridge for a Level Control cluster endpoint and java object
    static void NewManager(jint endpoint, jobject manager);

    // helps java set attributes::CurrentLevel of the Level Control cluster
    static jboolean SetCurrentLevel(jint endpoint, uint8_t value);

    // posts a CurrentLevel-changed event to the suitable LevelControlManager
    static void PostLevelChanged(chip::EndpointId endpoint, uint8_t value);

    // handles `Changed` callbacks by calling the java `void handleLevelChanged(int)` method
    void HandleLevelChanged(uint8_t value);

private:
    // init with java objects
    CHIP_ERROR InitializeWithObjects(jobject managerObject);
    chip::JniGlobalReference mLevelManagerObject;
    jmethodID mHandleLevelChangedMethod = nullptr;
};
