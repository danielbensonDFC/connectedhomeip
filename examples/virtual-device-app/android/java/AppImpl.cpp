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

#include "AppImpl.h"
#include "DeviceApp-JNI.h"

#include <app/server/Server.h>
#include <lib/core/CHIPCore.h>
#include <lib/core/DataModelTypes.h>
#include <lib/support/Span.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/DeviceInfoProvider.h>

using namespace chip;
using namespace chip::DeviceLayer;

// ---------------------------------------------------------------------------
// Minimal DeviceInfoProvider implementation required before Server::Init.
// The LocalizationConfiguration cluster aborts if no DeviceInfoProvider is
// registered when it initialises (see ClusterIntegration.cpp).
// This stub reports "en-US" as the single supported locale, which is
// sufficient for the Matter commissioning demo.
// ---------------------------------------------------------------------------
namespace {

class SteamistDeviceInfoProvider : public DeviceInfoProvider
{
public:
    // -----------------------------------------------------------------------
    // Locale support — report "en-US" as the single supported locale.
    // -----------------------------------------------------------------------
    class LocaleIterator : public Iterator<CharSpan>
    {
    public:
        size_t Count() override { return 1; }
        bool Next(CharSpan & item) override
        {
            if (mDone) return false;
            mDone = true;
            item  = CharSpan::fromCharString("en-US");
            return true;
        }
        void Release() override {}
    private:
        bool mDone = false;
    };

    // -----------------------------------------------------------------------
    // Calendar type support — report nothing (optional cluster feature).
    // -----------------------------------------------------------------------
    class CalendarIterator : public Iterator<CalendarType>
    {
    public:
        size_t Count() override { return 0; }
        bool Next(CalendarType &) override { return false; }
        void Release() override {}
    };

    // -----------------------------------------------------------------------
    // Fixed/User label support — report nothing (optional cluster feature).
    // -----------------------------------------------------------------------
    using LabelEntry = chip::app::Clusters::detail::Structs::LabelStruct::Type;
    class EmptyLabelIterator : public Iterator<LabelEntry>
    {
    public:
        size_t Count() override { return 0; }
        bool Next(LabelEntry &) override { return false; }
        void Release() override {}
    };

    SupportedLocalesIterator *       IterateSupportedLocales() override       { return &mLocaleIterator; }
    SupportedCalendarTypesIterator * IterateSupportedCalendarTypes() override { return &mCalendarIterator; }

    FixedLabelIterator * IterateFixedLabel(EndpointId) override { return &mLabelIterator; }
    UserLabelIterator *  IterateUserLabel(EndpointId) override  { return &mLabelIterator; }

    CHIP_ERROR SetUserLabelAt(EndpointId, size_t, const LabelEntry &) override { return CHIP_NO_ERROR; }
    CHIP_ERROR DeleteUserLabelAt(EndpointId, size_t) override                  { return CHIP_NO_ERROR; }
    CHIP_ERROR SetUserLabelLength(EndpointId, size_t) override                 { return CHIP_NO_ERROR; }
    CHIP_ERROR GetUserLabelLength(EndpointId, size_t & val) override           { val = 0; return CHIP_NO_ERROR; }

private:
    LocaleIterator     mLocaleIterator;
    CalendarIterator   mCalendarIterator;
    EmptyLabelIterator mLabelIterator;
};

SteamistDeviceInfoProvider gDeviceInfoProvider;

} // namespace

void DeviceEventCallback(const ChipDeviceEvent * event, intptr_t arg)
{
    ChipLogProgress(DeviceLayer, "DeviceEventCallback : %d", event->Type);

    switch (event->Type)
    {
    case DeviceEventType::kWiFiConnectivityChange:
        ChipLogProgress(DeviceLayer, "kWiFiConnectivityChange");
        break;
    case DeviceEventType::kInternetConnectivityChange:
        ChipLogProgress(DeviceLayer, "InternetConnectivityChange");
        break;
    case DeviceEventType::kServiceConnectivityChange:
        ChipLogProgress(DeviceLayer, "ServiceConnectivityChange");
        break;
    case DeviceEventType::kServiceProvisioningChange:
        ChipLogProgress(DeviceLayer, "ServiceProvisioningChange");
        break;
    case DeviceEventType::kCHIPoBLEConnectionEstablished:
        ChipLogProgress(DeviceLayer, "CHIPoBLE connection established");
        break;
    case DeviceEventType::kCHIPoBLEConnectionClosed:
        ChipLogProgress(DeviceLayer, "CHIPoBLE disconnected");
        break;
    case DeviceEventType::kCHIPoBLEAdvertisingChange:
        ChipLogProgress(DeviceLayer, "CHIPoBLEAdvertisingChange");
        break;
    case DeviceEventType::kInterfaceIpAddressChanged:
        ChipLogProgress(DeviceLayer, "InterfaceIpAddressChanged");
        break;
    case DeviceEventType::kCommissioningComplete:
        ChipLogProgress(DeviceLayer, "Commissioning complete");
        break;
    case DeviceEventType::kOperationalNetworkEnabled:
        ChipLogProgress(DeviceLayer, "OperationalNetworkEnabled");
        break;
    case DeviceEventType::kDnssdInitialized:
        ChipLogProgress(DeviceLayer, "DnssdPlatformInitialized");
        break;
    }

    DeviceAppJNIMgr().PostEvent(event->Type);
}

static int kFabricRemoved = 0x9FFF; // out of public event range (0x8000)

class FabricDelegate : public FabricTable::Delegate
{
    void OnFabricRemoved(const FabricTable & fabricTable, FabricIndex fabricIndex) override
    {
        ChipLogProgress(DeviceLayer, "OnFabricRemoved():FabricCount[%d],FabricIndex[%d]", fabricTable.FabricCount(), fabricIndex);
        if (fabricTable.FabricCount() == 0)
        {
            DeviceAppJNIMgr().PostEvent(kFabricRemoved);
        }
    }
};

static FabricDelegate gFabricDelegate;

CHIP_ERROR PreServerInit()
{
    /**
     * Apply any user-defined configurations prior to initializing Server.
     *
     * Ex.
     *   DnssdServer::Instance().SetExtendedDiscoveryTimeoutSecs(userTimeoutSecs);
     *
     */

    // Must be set before Server::Init so the LocalizationConfiguration cluster
    // can retrieve the device's supported locales without aborting.
    DeviceLayer::SetDeviceInfoProvider(&gDeviceInfoProvider);

    (void) chip::DeviceLayer::PlatformMgr().AddEventHandler(DeviceEventCallback, reinterpret_cast<intptr_t>(nullptr));
    (void) Server::GetInstance().GetFabricTable().AddFabricDelegate(&gFabricDelegate);

    return CHIP_NO_ERROR;
}
