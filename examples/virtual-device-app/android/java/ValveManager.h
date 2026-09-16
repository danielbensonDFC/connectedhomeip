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

#include <app-common/zap-generated/cluster-enums.h>
#include <app/clusters/valve-configuration-and-control-server/valve-configuration-and-control-delegate.h>
#include <app/data-model/Nullable.h>
#include <jni.h>
#include <lib/support/JniReferences.h>

/**
 * @brief Bridges the ValveConfigurationAndControl cluster to Java for the Steamist SOM's shower-valve
 * endpoint. Unlike OnOff/LevelControl (which are attribute stores driven from ClusterChangeAttribute),
 * the valve cluster is delegate-driven: the SDK server calls HandleOpenValve/HandleCloseValve when a
 * controller sends Open/Close. This manager IS that delegate; it forwards each to Java
 * (handleValveOpen/handleValveClose), where the SOM starts/stops a shower. It also exposes SetValveState
 * for the reverse direction (a shower started locally on the panel → push CurrentState to the fabric).
 *
 * Installed explicitly from Java after server init (setValveManager), matching LevelControl/ColorControl.
 */
class ValveManager : public chip::app::Clusters::ValveConfigurationAndControl::Delegate
{
public:
    // Create the manager for a valve endpoint + java object and register it as the cluster's delegate.
    static void NewManager(jint endpoint, jobject manager);

    // Reverse direction: push CurrentState (Open/Closed) to the fabric for a locally-driven change.
    static jboolean SetValveState(jint endpoint, bool open);

    // ---- ValveConfigurationAndControl::Delegate ----
    // A controller opened the valve. We treat the transition as instant and forward to Java. Returns the
    // requested level unchanged (null when the Level feature is off, which is our case).
    chip::app::DataModel::Nullable<chip::Percent> HandleOpenValve(chip::app::DataModel::Nullable<chip::Percent> level) override;
    // A controller closed the valve. Forward to Java.
    CHIP_ERROR HandleCloseValve() override;
    // No timed auto-close in this POC (base feature, no OpenDuration) — nothing to tick.
    void HandleRemainingDurationTick(uint32_t duration) override;

private:
    CHIP_ERROR InitializeWithObjects(jobject managerObject);
    void HandleValveOpen();
    void HandleValveClose();

    chip::JniGlobalReference mValveManagerObject;
    jmethodID mHandleValveOpenMethod  = nullptr;
    jmethodID mHandleValveCloseMethod = nullptr;
    chip::EndpointId mEndpoint        = 0;
};
