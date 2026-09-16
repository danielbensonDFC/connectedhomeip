package com.matter.virtual.device.app;

/**
 * Bridges the ValveConfigurationAndControl cluster to the app. A controller's Open/Close command on the
 * valve endpoint calls {@link #handleValveOpen()} / {@link #handleValveClose()} — the SOM maps these to
 * starting / stopping a shower. Unlike OnOff/LevelControl there is no attribute-set echo to guard
 * against: these fire only for real controller commands via the cluster delegate.
 */
public interface ValveManager {

  /** A controller opened the valve (Open command) → start the shower. */
  void handleValveOpen();

  /** A controller closed the valve (Close command) → stop the shower. */
  void handleValveClose();
}
