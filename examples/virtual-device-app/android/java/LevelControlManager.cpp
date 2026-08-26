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
#include "LevelControlManager.h"
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

static constexpr size_t kLevelManagerTableSize = MATTER_DM_LEVEL_CONTROL_CLUSTER_SERVER_ENDPOINT_COUNT;

namespace {

LevelControlManager * gLevelManagerTable[kLevelManagerTableSize] = { nullptr };
static_assert(kLevelManagerTableSize <= kEmberInvalidEndpointIndex, "gLevelManagerTable table size error");

} // namespace

void LevelControlManager::NewManager(jint endpoint, jobject manager)
{
    ChipLogProgress(Zcl, "Device App: LevelControlManager::NewManager");
    uint16_t ep = emberAfGetClusterServerEndpointIndex(static_cast<chip::EndpointId>(endpoint), app::Clusters::LevelControl::Id,
                                                       MATTER_DM_LEVEL_CONTROL_CLUSTER_SERVER_ENDPOINT_COUNT);
    VerifyOrReturn(ep < kLevelManagerTableSize,
                   ChipLogError(Zcl, "Device App::LevelControl::NewManager: endpoint %d not found", endpoint));

    VerifyOrReturn(gLevelManagerTable[ep] == nullptr,
                   ChipLogError(Zcl, "Device App::LevelControl::NewManager: endpoint %d already has a manager", endpoint));
    LevelControlManager * mgr = new LevelControlManager();
    CHIP_ERROR err            = mgr->InitializeWithObjects(manager);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(Zcl, "Device App::LevelControl::NewManager: failed to initialize manager for endpoint %d", endpoint);
        delete mgr;
    }
    else
    {
        gLevelManagerTable[ep] = mgr;
    }
}

LevelControlManager * GetLevelControlManager(EndpointId endpoint)
{
    uint16_t ep = emberAfGetClusterServerEndpointIndex(endpoint, app::Clusters::LevelControl::Id,
                                                       MATTER_DM_LEVEL_CONTROL_CLUSTER_SERVER_ENDPOINT_COUNT);
    return (ep >= kLevelManagerTableSize ? nullptr : gLevelManagerTable[ep]);
}

void LevelControlManager::PostLevelChanged(chip::EndpointId endpoint, uint8_t value)
{
    ChipLogProgress(Zcl, "Device App: LevelControlManager::PostLevelChanged");
    LevelControlManager * mgr = GetLevelControlManager(endpoint);
    VerifyOrReturn(mgr != nullptr, ChipLogError(Zcl, "LevelControlManager null"));

    mgr->HandleLevelChanged(value);
}

jboolean LevelControlManager::SetCurrentLevel(jint endpoint, uint8_t value)
{
    Protocols::InteractionModel::Status status =
        app::Clusters::LevelControl::Attributes::CurrentLevel::Set(static_cast<chip::EndpointId>(endpoint), value);
    return status == Protocols::InteractionModel::Status::Success;
}

CHIP_ERROR LevelControlManager::InitializeWithObjects(jobject managerObject)
{
    JNIEnv * env = JniReferences::GetInstance().GetEnvForCurrentThread();
    VerifyOrReturnLogError(env != nullptr, CHIP_ERROR_INCORRECT_STATE);
    ReturnLogErrorOnFailure(mLevelManagerObject.Init(managerObject));

    jclass LevelManagerClass = env->GetObjectClass(managerObject);
    VerifyOrReturnLogError(LevelManagerClass != nullptr, CHIP_ERROR_INVALID_ARGUMENT);

    mHandleLevelChangedMethod = env->GetMethodID(LevelManagerClass, "handleLevelChanged", "(I)V");
    if (mHandleLevelChangedMethod == nullptr)
    {
        ChipLogError(Zcl, "Failed to access LevelControlManager 'handleLevelChanged' method");
        env->ExceptionClear();
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    return CHIP_NO_ERROR;
}

void LevelControlManager::HandleLevelChanged(uint8_t value)
{
    ChipLogProgress(Zcl, "LevelControlManager::HandleLevelChanged");

    JNIEnv * env = JniReferences::GetInstance().GetEnvForCurrentThread();
    VerifyOrReturn(env != NULL, ChipLogProgress(Zcl, "env null"));
    VerifyOrReturn(mLevelManagerObject.HasValidObjectRef(), ChipLogProgress(Zcl, "mLevelManagerObject null"));
    VerifyOrReturn(mHandleLevelChangedMethod != nullptr, ChipLogProgress(Zcl, "mHandleLevelChangedMethod null"));

    env->ExceptionClear();
    env->CallVoidMethod(mLevelManagerObject.ObjectRef(), mHandleLevelChangedMethod, static_cast<jint>(value));
    if (env->ExceptionCheck())
    {
        ChipLogError(AppServer, "Java exception in LevelControlManager::HandleLevelChanged");
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}
