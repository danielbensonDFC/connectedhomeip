/**
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
#include "ValveManager.h"
#include "DeviceApp-JNI.h"
#include <app/clusters/valve-configuration-and-control-server/valve-configuration-and-control-server.h>
#include <jni.h>
#include <lib/support/CHIPJNIError.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/JniReferences.h>
#include <lib/support/JniTypeWrappers.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;

namespace {

// One valve endpoint on this device (the panel's shower valve). Kept so a second install can't leak a
// delegate and so SetValveState can be a no-op if the manager isn't up yet.
ValveManager * gValveManager = nullptr;

} // namespace

void ValveManager::NewManager(jint endpoint, jobject manager)
{
    ChipLogProgress(Zcl, "Device App: ValveManager::NewManager");
    VerifyOrReturn(gValveManager == nullptr,
                   ChipLogError(Zcl, "Device App::Valve::NewManager: a valve manager already exists"));

    ValveManager * mgr = new ValveManager();
    CHIP_ERROR err     = mgr->InitializeWithObjects(manager);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(Zcl, "Device App::Valve::NewManager: failed to initialize manager for endpoint %d", endpoint);
        delete mgr;
        return;
    }
    mgr->mEndpoint = static_cast<EndpointId>(endpoint);
    // Register as the cluster's application delegate — this is how Open/Close reach us.
    ValveConfigurationAndControl::SetDefaultDelegate(static_cast<EndpointId>(endpoint), mgr);
    gValveManager = mgr;
    ChipLogProgress(Zcl, "Device App: Valve delegate installed on endpoint %d", endpoint);
}

jboolean ValveManager::SetValveState(jint endpoint, bool open)
{
    CHIP_ERROR err = ValveConfigurationAndControl::UpdateCurrentState(
        static_cast<EndpointId>(endpoint),
        open ? ValveConfigurationAndControl::ValveStateEnum::kOpen : ValveConfigurationAndControl::ValveStateEnum::kClosed);
    return err == CHIP_NO_ERROR;
}

DataModel::Nullable<chip::Percent> ValveManager::HandleOpenValve(DataModel::Nullable<chip::Percent> level)
{
    ChipLogProgress(Zcl, "Device App: ValveManager::HandleOpenValve");
    HandleValveOpen();
    // Treat the transition as instant: the server set CurrentState=Transitioning; drive it to Open now so
    // a base (no-duration) valve settles deterministically and controllers see Open immediately. Calling
    // Update* from within the delegate is the supported pattern (see all-clusters ValveControlDelegate).
    LogErrorOnFailure(
        ValveConfigurationAndControl::UpdateCurrentState(mEndpoint, ValveConfigurationAndControl::ValveStateEnum::kOpen));
    // Echo the requested level back (null when the Level feature is off, which is our case).
    return level;
}

CHIP_ERROR ValveManager::HandleCloseValve()
{
    ChipLogProgress(Zcl, "Device App: ValveManager::HandleCloseValve");
    HandleValveClose();
    LogErrorOnFailure(
        ValveConfigurationAndControl::UpdateCurrentState(mEndpoint, ValveConfigurationAndControl::ValveStateEnum::kClosed));
    return CHIP_NO_ERROR;
}

void ValveManager::HandleRemainingDurationTick(uint32_t duration)
{
    // No-op: base feature, no timed open duration in this POC.
    (void) duration;
}

CHIP_ERROR ValveManager::InitializeWithObjects(jobject managerObject)
{
    JNIEnv * env = JniReferences::GetInstance().GetEnvForCurrentThread();
    VerifyOrReturnLogError(env != nullptr, CHIP_ERROR_INCORRECT_STATE);
    ReturnLogErrorOnFailure(mValveManagerObject.Init(managerObject));

    jclass ValveManagerClass = env->GetObjectClass(managerObject);
    VerifyOrReturnLogError(ValveManagerClass != nullptr, CHIP_ERROR_INVALID_ARGUMENT);

    mHandleValveOpenMethod = env->GetMethodID(ValveManagerClass, "handleValveOpen", "()V");
    if (mHandleValveOpenMethod == nullptr)
    {
        ChipLogError(Zcl, "Failed to access ValveManager 'handleValveOpen' method");
        env->ExceptionClear();
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    mHandleValveCloseMethod = env->GetMethodID(ValveManagerClass, "handleValveClose", "()V");
    if (mHandleValveCloseMethod == nullptr)
    {
        ChipLogError(Zcl, "Failed to access ValveManager 'handleValveClose' method");
        env->ExceptionClear();
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    return CHIP_NO_ERROR;
}

void ValveManager::HandleValveOpen()
{
    JNIEnv * env = JniReferences::GetInstance().GetEnvForCurrentThread();
    VerifyOrReturn(env != nullptr, ChipLogProgress(Zcl, "env null"));
    VerifyOrReturn(mValveManagerObject.HasValidObjectRef(), ChipLogProgress(Zcl, "mValveManagerObject null"));
    VerifyOrReturn(mHandleValveOpenMethod != nullptr, ChipLogProgress(Zcl, "mHandleValveOpenMethod null"));

    env->ExceptionClear();
    env->CallVoidMethod(mValveManagerObject.ObjectRef(), mHandleValveOpenMethod);
    if (env->ExceptionCheck())
    {
        ChipLogError(AppServer, "Java exception in ValveManager::HandleValveOpen");
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}

void ValveManager::HandleValveClose()
{
    JNIEnv * env = JniReferences::GetInstance().GetEnvForCurrentThread();
    VerifyOrReturn(env != nullptr, ChipLogProgress(Zcl, "env null"));
    VerifyOrReturn(mValveManagerObject.HasValidObjectRef(), ChipLogProgress(Zcl, "mValveManagerObject null"));
    VerifyOrReturn(mHandleValveCloseMethod != nullptr, ChipLogProgress(Zcl, "mHandleValveCloseMethod null"));

    env->ExceptionClear();
    env->CallVoidMethod(mValveManagerObject.ObjectRef(), mHandleValveCloseMethod);
    if (env->ExceptionCheck())
    {
        ChipLogError(AppServer, "Java exception in ValveManager::HandleValveClose");
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}
