# Steamist SOM Matter Fork — Context

This is a fork (`danielbensonDFC/connectedhomeip`, remote `fork`) of the upstream Project CHIP
/ Matter SDK (`project-chip/connectedhomeip`, remote `origin`). It exists **only** to build a
custom Matter Android device for the Steamist SOM controller app — it is not a general
contribution branch, and nothing here is intended to be upstreamed as-is.

**The companion app repo** (`com.deltafaucet.steamist.som`) has its own `CLAUDE.md` with the
full picture: why on-network Wi-Fi commissioning was chosen, the app-side architecture, the
build→sign→install workflow, and a numbered list of non-obvious bugs found while integrating
this SDK. Read that first — this file only covers what's specific to working in *this* repo.

## Branches

- `steamistMods` — the base, on-network (Wi-Fi/mDNS) commissioning work. Mirrors the SOM
  repo's `matter-switch` branch. **Has uncommitted changes as of this writing** — see below.
- `matter-ble-generic-switch` — further-along, less-verified work adding a BLE peripheral
  and a second Generic Switch endpoint. Mirrors the SOM repo's branch of the same name.
  Push target for this is also `fork`, same caveats as the SOM-side doc: this hasn't been
  walked through end-to-end with a real ecosystem commissioner yet.

## What's modified from upstream, and why

All changes are scoped to `examples/virtual-device-app/` — we build Google's reference
"Matter Android device" example app and link it into the Steamist Android app as a native
library + jar, rather than using any prebuilt AAR (none exists for the Android device role).

1. **`AppImpl.cpp`** — added a `SteamistDeviceInfoProvider` (implements every pure-virtual
   method of `DeviceInfoProvider`, all stubbed to "none/empty" since we don't need locale or
   label data) and calls `DeviceLayer::SetDeviceInfoProvider(&gDeviceInfoProvider)` in
   `PreServerInit()`. Without this, `Server::Init` aborts (`VerifyOrDie` on a null provider)
   the first time the LocalizationConfiguration cluster initializes.

2. **`DeviceApp-JNI.cpp` / `DeviceApp.java`** — added
   `openBasicCommissioningWindow(int timeoutSeconds)` and `closeCommissioningWindow()`,
   wrapping `chip::Server::GetInstance().GetCommissioningWindowManager()`. The commissioning
   window auto-opened at `Server::Init` has its own short-lived timeout independent of
   whether the server process itself is running — the app needs to explicitly reopen a
   fresh window every time the user revisits the pairing screen, or the device silently
   stops advertising as commissionable (visible in `dns-sd -B` as Add/Rmv flapping).

3. **ZAP config trim (`virtual-device-common/virtual-device-app.zap` /
   `.matter`)** — endpoint 1 in the stock example has `OnOff`, `DoorLock`, and
   `ColorControl` all enabled simultaneously (it's a generic test template, not a real
   product). This made Google Home render the device as a Lock/Light instead of the
   intended Plug, because ecosystem icon selection looks at which clusters are present, not
   just the declared device type ID. Trimmed to `Identify, Groups, OnOff, Descriptor,
   PowerSource, ScenesManagement`, and the static `deviceTypeRef`/`deviceTypes`/
   `deviceIdentifiers` fields updated to `MA-onoffpluginunit` (code 266 / `0x010A`) to match
   what the app sets at runtime via `postServerInit()`.

   This required also **deleting** the now-dead `ColorControlManager.{cpp,h,java}` and
   `DoorLockManager.{cpp,h,java}` (and their `BUILD.gn` entries, JNI method bodies, and
   `#include`s/dispatch cases in `ClusterChangeAttribute.cpp`) — leaving them in place after
   removing the last endpoint that used those clusters causes a compile error
   (`MATTER_DM_*_CLUSTER_SERVER_ENDPOINT_COUNT` collapses to 0, so their
   `gXxxManagerTable[0] = { nullptr }` initializers become "excess elements in array
   initializer").

   **Status**: builds clean, not yet re-verified against a fresh Google Home commissioning
   (old pairings cache the previous cluster/endpoint structure — remove and re-pair to test).

4. **Android platform layer — mDNS.** Not a code change in this repo, but a **build config
   change**: `out/android-arm64-virtual-device-app/args.gn` (which is not tracked by git —
   it's a generated build-output directory, not source) must include:
   ```
   chip_mdns = "minimal"
   ```
   Android's `NsdManager` cannot publish the comma-separated Matter subtypes
   (`_matterc._udp,_V65521,_L3840,...`) that `NsdManagerServiceResolver.java` tries to
   register — modern Android's `NsdService` rejects any service type containing a comma.
   Setting `chip_mdns = "minimal"` switches to the SDK's own spec-compliant mDNS responder
   over raw multicast sockets, bypassing `NsdManager` entirely. **If you ever do a truly
   fresh `gn gen` for this out-directory, you must re-add this line** — it's easy to lose
   since it lives outside version control.

## Rebuild instructions

See the SOM repo's `CLAUDE.md` for the full loop (env vars, `zap generate.py`, `ninja`
targets, the flaky `onboarding_payload.classlist` retry, the stale-object-cache workaround).
Short version:
```bash
export ANDROID_HOME=~/Library/Android/sdk
export ANDROID_NDK_HOME=~/Library/Android/sdk/ndk/30.0.14904198
source scripts/activate.sh

# after editing the .zap file:
./scripts/tools/zap/generate.py examples/virtual-device-app/virtual-device-common/virtual-device-app.zap

ninja -C out/android-arm64-virtual-device-app jni lib/DeviceApp.jar
```
Output artifacts (`.so`, `DeviceApp.jar`, and several other jars — see the SOM repo's
`CLAUDE.md` for the full list and exact source paths) then get copied into the SOM repo's
`common/libs/matter/` and `common/src/main/jniLibs/arm64-v8a/`.

## Pushing

```bash
git push fork <branch-name>
```
Never push to `origin` (`project-chip/connectedhomeip`) — that's the real upstream project.
If any of this work ever becomes worth contributing back (e.g. the `DeviceInfoProvider` gap
or the `NsdManager` subtype-comma rejection are arguably real upstream bugs), open a PR from
`fork` into `origin` deliberately, not by accident.
