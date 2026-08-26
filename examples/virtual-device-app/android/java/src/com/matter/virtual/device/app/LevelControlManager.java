package com.matter.virtual.device.app;

public interface LevelControlManager {

  /** initialize CurrentLevel value by DeviceApp */
  void initAttributeValue();

  /**
   * Notify that the CurrentLevel value was changed by matter and should be effected. Note, a set by
   * DeviceApp will also trigger this function, so must check if the value is the same.
   *
   * @param value the new CurrentLevel (0..254)
   */
  void handleLevelChanged(int value);
}
