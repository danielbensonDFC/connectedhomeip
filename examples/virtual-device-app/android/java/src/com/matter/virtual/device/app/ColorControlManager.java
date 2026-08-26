package com.matter.virtual.device.app;

public interface ColorControlManager {

  /** initialize CurrentHue / CurrentSaturation values by DeviceApp */
  void initAttributeValue();

  /**
   * Notify that the CurrentHue value was changed by matter and should be effected. Note, a set by
   * DeviceApp will also trigger this function, so must check if the value is the same.
   *
   * @param value the new CurrentHue (0..254)
   */
  void handleHueChanged(int value);

  /**
   * Notify that the CurrentSaturation value was changed by matter and should be effected. Note, a
   * set by DeviceApp will also trigger this function, so must check if the value is the same.
   *
   * @param value the new CurrentSaturation (0..254)
   */
  void handleSaturationChanged(int value);
}
