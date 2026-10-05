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

#include <app/clusters/network-commissioning/network-commissioning.h>
#include <app/server/Server.h>
#include <lib/core/CHIPCore.h>
#include <lib/core/DataModelTypes.h>
#include <lib/support/Span.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/DeviceInfoProvider.h>
#include <platform/NetworkCommissioning.h>

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

    // -----------------------------------------------------------------------
    // Fixed labels — name the two valve endpoints so a controller can tell them apart (both are the
    // same Water Valve device type). Spec convention: label "name". Read by the Delta phone/SOM to show
    // "Steam" / "Shower"; other ecosystems may ignore it. Heap-allocated per call because the Fixed
    // Label server Release()s the iterator when it's done.
    // -----------------------------------------------------------------------
    class EndpointNameIterator : public Iterator<LabelEntry>
    {
    public:
        explicit EndpointNameIterator(const char * name) : mName(name) {}
        size_t Count() override { return mName == nullptr ? 0 : 1; }
        bool Next(LabelEntry & item) override
        {
            if (mDone || mName == nullptr) return false;
            mDone       = true;
            item.label = CharSpan::fromCharString("name");
            item.value = CharSpan::fromCharString(mName);
            return true;
        }
        void Release() override { delete this; }
    private:
        const char * mName;
        bool mDone = false;
    };

    static const char * EndpointName(EndpointId endpoint)
    {
        switch (endpoint)
        {
        case 2: return "Steam";  // keep in sync with MatterAccessoryService.STEAM_VALVE_ENDPOINT
        case 3: return "Shower"; // keep in sync with MatterAccessoryService.SHOWER_VALVE_ENDPOINT
        default: return nullptr;
        }
    }

    SupportedLocalesIterator *       IterateSupportedLocales() override       { return &mLocaleIterator; }
    SupportedCalendarTypesIterator * IterateSupportedCalendarTypes() override { return &mCalendarIterator; }

    FixedLabelIterator * IterateFixedLabel(EndpointId endpoint) override
    {
        return new EndpointNameIterator(EndpointName(endpoint));
    }
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

// ---------------------------------------------------------------------------
// NetworkCommissioning on the root endpoint.
//
// The ZAP enables this cluster, but all of its attributes are EXTERNAL_STORAGE:
// they are only answered if a driver instance is registered. With no instance,
// every read fails with an IM error — including FeatureMap (0xFFFC), which is
// the FIRST thing a third-party commissioner reads after PASE. Google Home
// read it, got a status error, and silently abandoned commissioning until the
// fail-safe expired ("something went wrong"), which is how this was found.
//
// Normally this instance is created in examples/platform/linux/AppMain.cpp,
// which the Android target never links — the same gap that left
// DeviceInfoProvider unregistered above.
//
// EthernetDriver is the right flavour here even though the panel is on Wi-Fi:
// it reports "already on an IP network, nothing to provision", which is true,
// because the panel's Wi-Fi is owned by Android and never configured over
// Matter. A WiFiDriver would advertise scan/connect capability we do not
// implement; commissioners skip network provisioning for an already-networked
// device anyway.
// ---------------------------------------------------------------------------
// EthernetDriver is abstract (GetMaxNetworks/GetNetworks), so provide the minimal concrete driver.
// It reports exactly one, always-connected network: the interface the OS already brought up. There is
// nothing to scan, add or remove — which is the whole point of using the ethernet flavour here.
class SteamistEthernetDriver final : public NetworkCommissioning::EthernetDriver
{
public:
    class NetworkIteratorImpl final : public NetworkCommissioning::NetworkIterator
    {
    public:
        size_t Count() override { return 1; }

        bool Next(NetworkCommissioning::Network & item) override
        {
            if (mExhausted)
            {
                return false;
            }
            mExhausted = true;

            // A non-empty NetworkID is required; the interface name is what other platforms report.
            static constexpr char kInterfaceName[] = "wlan0";
            static_assert(sizeof(kInterfaceName) - 1 <= sizeof(item.networkID), "network id too long");
            memcpy(item.networkID, kInterfaceName, sizeof(kInterfaceName) - 1);
            item.networkIDLen = static_cast<uint8_t>(sizeof(kInterfaceName) - 1);
            item.connected    = true;
            return true;
        }

        void Release() override { delete this; }

    private:
        bool mExhausted = false;
    };

    uint8_t GetMaxNetworks() override { return 1; }
    NetworkCommissioning::NetworkIterator * GetNetworks() override { return new NetworkIteratorImpl(); }
};

SteamistEthernetDriver gEthernetDriver;
app::Clusters::NetworkCommissioning::Instance gEthernetNetworkCommissioningInstance(kRootEndpointId, &gEthernetDriver);

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

CHIP_ERROR PostServerInit()
{
    // Must run AFTER Server::Init: Init() registers the cluster's attribute/command handlers with
    // the interaction model, which does not exist before the server is up.
    CHIP_ERROR err = gEthernetNetworkCommissioningInstance.Init();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "NetworkCommissioning init failed: %" CHIP_ERROR_FORMAT, err.Format());
        return err;
    }
    ChipLogProgress(DeviceLayer, "NetworkCommissioning (ethernet) registered on the root endpoint");
    return CHIP_NO_ERROR;
}
