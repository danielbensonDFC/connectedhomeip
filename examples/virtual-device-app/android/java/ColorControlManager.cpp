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
#include "ColorControlManager.h"
#include "DeviceApp-JNI.h"
#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/util/attribute-storage.h>
#include <app/util/config.h>
#include <jni.h>
#include <lib/support/CHIPJNIError.h>
#include <lib/support/JniReferences.h>
#include <lib/support/JniTypeWrappers.h>

using namespace chip;

static constexpr size_t kColorManagerTableSize = MATTER_DM_COLOR_CONTROL_CLUSTER_SERVER_ENDPOINT_COUNT;

namespace {

ColorControlManager * gColorManagerTable[kColorManagerTableSize] = { nullptr };
static_assert(kColorManagerTableSize <= kEmberInvalidEndpointIndex, "gColorManagerTable table size error");

} // namespace

void ColorControlManager::NewManager(jint endpoint, jobject manager)
{
    ChipLogProgress(Zcl, "Device App: ColorControlManager::NewManager");
    uint16_t ep = emberAfGetClusterServerEndpointIndex(static_cast<chip::EndpointId>(endpoint), app::Clusters::ColorControl::Id,
                                                       MATTER_DM_COLOR_CONTROL_CLUSTER_SERVER_ENDPOINT_COUNT);
    VerifyOrReturn(ep < kColorManagerTableSize,
                   ChipLogError(Zcl, "Device App::ColorControl::NewManager: endpoint %d not found", endpoint));

    VerifyOrReturn(gColorManagerTable[ep] == nullptr,
                   ChipLogError(Zcl, "Device App::ColorControl::NewManager: endpoint %d already has a manager", endpoint));
    ColorControlManager * mgr = new ColorControlManager();
    CHIP_ERROR err            = mgr->InitializeWithObjects(manager);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(Zcl, "Device App::ColorControl::NewManager: failed to initialize manager for endpoint %d", endpoint);
        delete mgr;
    }
    else
    {
        gColorManagerTable[ep] = mgr;
    }
}

ColorControlManager * GetColorControlManager(EndpointId endpoint)
{
    uint16_t ep = emberAfGetClusterServerEndpointIndex(endpoint, app::Clusters::ColorControl::Id,
                                                       MATTER_DM_COLOR_CONTROL_CLUSTER_SERVER_ENDPOINT_COUNT);
    return (ep >= kColorManagerTableSize ? nullptr : gColorManagerTable[ep]);
}

void ColorControlManager::PostHueChanged(chip::EndpointId endpoint, uint8_t value)
{
    ChipLogProgress(Zcl, "Device App: ColorControlManager::PostHueChanged");
    ColorControlManager * mgr = GetColorControlManager(endpoint);
    VerifyOrReturn(mgr != nullptr, ChipLogError(Zcl, "ColorControlManager null"));

    mgr->HandleHueChanged(value);
}

void ColorControlManager::PostSaturationChanged(chip::EndpointId endpoint, uint8_t value)
{
    ChipLogProgress(Zcl, "Device App: ColorControlManager::PostSaturationChanged");
    ColorControlManager * mgr = GetColorControlManager(endpoint);
    VerifyOrReturn(mgr != nullptr, ChipLogError(Zcl, "ColorControlManager null"));

    mgr->HandleSaturationChanged(value);
}

jboolean ColorControlManager::SetCurrentHue(jint endpoint, uint8_t value)
{
    Protocols::InteractionModel::Status status =
        app::Clusters::ColorControl::Attributes::CurrentHue::Set(static_cast<chip::EndpointId>(endpoint), value);
    return status == Protocols::InteractionModel::Status::Success;
}

jboolean ColorControlManager::SetCurrentSaturation(jint endpoint, uint8_t value)
{
    Protocols::InteractionModel::Status status =
        app::Clusters::ColorControl::Attributes::CurrentSaturation::Set(static_cast<chip::EndpointId>(endpoint), value);
    return status == Protocols::InteractionModel::Status::Success;
}

CHIP_ERROR ColorControlManager::InitializeWithObjects(jobject managerObject)
{
    JNIEnv * env = JniReferences::GetInstance().GetEnvForCurrentThread();
    VerifyOrReturnLogError(env != nullptr, CHIP_ERROR_INCORRECT_STATE);
    ReturnLogErrorOnFailure(mColorManagerObject.Init(managerObject));

    jclass ColorManagerClass = env->GetObjectClass(managerObject);
    VerifyOrReturnLogError(ColorManagerClass != nullptr, CHIP_ERROR_INVALID_ARGUMENT);

    mHandleHueChangedMethod = env->GetMethodID(ColorManagerClass, "handleHueChanged", "(I)V");
    if (mHandleHueChangedMethod == nullptr)
    {
        ChipLogError(Zcl, "Failed to access ColorControlManager 'handleHueChanged' method");
        env->ExceptionClear();
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    mHandleSaturationChangedMethod = env->GetMethodID(ColorManagerClass, "handleSaturationChanged", "(I)V");
    if (mHandleSaturationChangedMethod == nullptr)
    {
        ChipLogError(Zcl, "Failed to access ColorControlManager 'handleSaturationChanged' method");
        env->ExceptionClear();
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    return CHIP_NO_ERROR;
}

void ColorControlManager::HandleHueChanged(uint8_t value)
{
    ChipLogProgress(Zcl, "ColorControlManager::HandleHueChanged");

    JNIEnv * env = JniReferences::GetInstance().GetEnvForCurrentThread();
    VerifyOrReturn(env != NULL, ChipLogProgress(Zcl, "env null"));
    VerifyOrReturn(mColorManagerObject.HasValidObjectRef(), ChipLogProgress(Zcl, "mColorManagerObject null"));
    VerifyOrReturn(mHandleHueChangedMethod != nullptr, ChipLogProgress(Zcl, "mHandleHueChangedMethod null"));

    env->ExceptionClear();
    env->CallVoidMethod(mColorManagerObject.ObjectRef(), mHandleHueChangedMethod, static_cast<jint>(value));
    if (env->ExceptionCheck())
    {
        ChipLogError(AppServer, "Java exception in ColorControlManager::HandleHueChanged");
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}

void ColorControlManager::HandleSaturationChanged(uint8_t value)
{
    ChipLogProgress(Zcl, "ColorControlManager::HandleSaturationChanged");

    JNIEnv * env = JniReferences::GetInstance().GetEnvForCurrentThread();
    VerifyOrReturn(env != NULL, ChipLogProgress(Zcl, "env null"));
    VerifyOrReturn(mColorManagerObject.HasValidObjectRef(), ChipLogProgress(Zcl, "mColorManagerObject null"));
    VerifyOrReturn(mHandleSaturationChangedMethod != nullptr, ChipLogProgress(Zcl, "mHandleSaturationChangedMethod null"));

    env->ExceptionClear();
    env->CallVoidMethod(mColorManagerObject.ObjectRef(), mHandleSaturationChangedMethod, static_cast<jint>(value));
    if (env->ExceptionCheck())
    {
        ChipLogError(AppServer, "Java exception in ColorControlManager::HandleSaturationChanged");
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}
