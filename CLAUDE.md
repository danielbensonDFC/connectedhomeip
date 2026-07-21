# Steamist SOM Matter Fork — Context (BLE / Generic Switch branch)

This is a fork (`danielbensonDFC/connectedhomeip`, remote `fork`) of the upstream Project CHIP
/ Matter SDK (`project-chip/connectedhomeip`, remote `origin`). It exists **only** to build a
custom Matter Android device for the Steamist SOM controller app — it is not a general
contribution branch, and nothing here is intended to be upstreamed as-is.

**The companion app repo** (`com.deltafaucet.steamist.som`) has its own `CLAUDE.md` with the
full picture: why on-network Wi-Fi commissioning was built first, the app-side architecture,
the build→sign→install workflow, and a numbered list of non-obvious bugs found while
integrating this SDK. Read that first — this file only covers what's specific to working in
*this* repo, and to this branch in particular.

## ⚠️ This branch is a sibling of `steamistMods`, not a descendant — it's missing later fixes

`matter-ble-generic-switch` branched from `steamistMods` at commit `1dcbdc83c8` ("Changes
needed to compile Steamist SOM"). Since then, `steamistMods` gained a commit
(`f8842cb96a`, "Trim endpoint 1 to a proper Plug device type, remove dead cluster code")
that this branch does **not** have:

- `ColorControlManager` / `DoorLockManager` (cpp/h/java) still exist here and are still
  wired up — this branch's endpoint 1 still has `DoorLock`/`ColorControl` enabled, which
  means it will likely have the same "Google Home shows a Lock/Light icon instead of a Plug"
  problem that motivated that fix.
- The ZAP device-type fields for endpoint 1 are still the stock `MA-onofflight` (0x100), not
  the corrected `MA-onoffpluginunit` (0x010A).

**Before merging this branch back into `steamistMods` (or vice versa), reconcile these.**
The cleanest path is probably: rebase this branch onto latest `steamistMods`, then re-apply
the Generic Switch endpoint addition on top of the already-trimmed endpoint 1, rather than
trying to merge two divergent ZAP-file edits.

## What this branch adds on top of the base

1. **A BLE peripheral** (`common/src/main/java/.../service/matter/MatterBlePeripheralManager.java`
   in the SOM repo — not in this repo) implementing `AndroidBleManager`'s advertising/GATT-server
   side: `BluetoothGattServer`, Matter service UUID advertising, C1/C2 characteristics, MTU and
   connection bookkeeping. The stock SDK's `AndroidBleManager.java` has these as literal `// TODO`
   stubs (device-role BLE was never implemented upstream for Android) — this is net-new work.

2. **A second Matter endpoint: Generic Switch (`0x000F`)**, added via ZAP config, with a
   `triggerSwitchPress()` JNI method (`DeviceApp-JNI.cpp` / `DeviceApp.java`) that emits an
   `InitialPress` + `ShortRelease` event pair via the SDK's `SwitchCluster` API — surfaced as a
   stateless "button" trigger for ecosystems that support it (notably Apple Home; Google Home
   does not support Generic Switch).

## Status: unverified

This compiles, but **has not been walked through end-to-end with a real ecosystem commissioner**
the way the on-network (`steamistMods`) path was — no confirmed BLE-first commissioning with
Google Home / Apple Home, no logcat trace of a successful BTP handshake. Treat it as a running
start, not a known-working baseline. Before relying on it:

- Test BLE commissioning from both Google Home and Apple Home (BLE-first discovery flow).
- Confirm the Generic Switch endpoint actually shows up as a button in Apple Home after pairing.
- Reconcile with the `steamistMods` fixes noted above.
- Everything in the base branch's gotcha list still applies here too (server can only start
  once per process, commissioning window needs explicit reopen, `DeviceInfoProvider` must be
  set before `Server::Init`, Android's `NsdManager` can't publish Matter subtypes so
  `chip_mdns = "minimal"` is required in the build's `args.gn`, etc.) — see the base branch's
  `CLAUDE.md` / the SOM repo's `CLAUDE.md` for the full writeup of each.

## Rebuild instructions

Same as the base branch — see the SOM repo's `CLAUDE.md` for the full loop. Short version:
```bash
export ANDROID_HOME=~/Library/Android/sdk
export ANDROID_NDK_HOME=~/Library/Android/sdk/ndk/30.0.14904198
source scripts/activate.sh

# after editing the .zap file:
./scripts/tools/zap/generate.py examples/virtual-device-app/virtual-device-common/virtual-device-app.zap

ninja -C out/android-arm64-virtual-device-app jni lib/DeviceApp.jar
```

## Pushing

```bash
git push fork matter-ble-generic-switch
```
Never push to `origin` (`project-chip/connectedhomeip`) — that's the real upstream project.
