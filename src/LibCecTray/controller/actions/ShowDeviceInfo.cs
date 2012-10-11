﻿using CecSharp;
using LibCECTray.Properties;
using LibCECTray.ui;

namespace LibCECTray.controller.actions
{
  class ShowDeviceInfo : UpdateProcess
  {
    public ShowDeviceInfo(CECController controller, LibCecSharp lib, CecLogicalAddress address)
    {
      _controller = controller;
      _lib = lib;
      _address = address;
    }

    public virtual void ShowDialog(CecLogicalAddress address, ref LibCecSharp lib,
      bool devicePresent, CecVendorId vendor, bool isActiveSource, ushort physicalAddress,
      CecVersion version, CecPowerStatus power, string osdName, string menuLanguage)
    {
      DeviceInformation di = new DeviceInformation(_controller, _address, ref _lib, devicePresent, vendor, isActiveSource, physicalAddress, version, power, osdName, menuLanguage);
      _controller.DisplayDialog(di, false);
    }

    public override void Process()
    {
      CecVendorId vendor = CecVendorId.Unknown;
      bool isActiveSource = false;
      ushort physicalAddress = 0xFFFF;
      CecVersion version = CecVersion.Unknown;
      CecPowerStatus power = CecPowerStatus.Unknown;
      string osdName = Resources.unknown;
      string menuLanguage = Resources.unknown;

      SendEvent(UpdateEventType.StatusText, string.Format(Resources.action_check_device_present, _lib.ToString(_address)));
      SendEvent(UpdateEventType.ProgressBar, 10);

      bool devicePresent = _lib.IsActiveDevice(_address);
      if (devicePresent)
      {
        SendEvent(UpdateEventType.StatusText, Resources.action_requesting_vendor_id);
        SendEvent(UpdateEventType.ProgressBar, 20);
        vendor = _lib.GetDeviceVendorId(_address);

        SendEvent(UpdateEventType.StatusText, Resources.action_requesting_active_source_state);
        SendEvent(UpdateEventType.ProgressBar, 30);
        isActiveSource = _lib.IsActiveSource(_address);

        SendEvent(UpdateEventType.StatusText, Resources.action_requesting_physical_address);
        SendEvent(UpdateEventType.ProgressBar, 40);
        physicalAddress = _lib.GetDevicePhysicalAddress(_address);

        SendEvent(UpdateEventType.StatusText, Resources.action_requesting_cec_version);
        SendEvent(UpdateEventType.ProgressBar, 50);
        version = _lib.GetDeviceCecVersion(_address);

        SendEvent(UpdateEventType.StatusText, Resources.action_requesting_power_status);
        SendEvent(UpdateEventType.ProgressBar, 60);
        power = _lib.GetDevicePowerStatus(_address);

        SendEvent(UpdateEventType.StatusText, Resources.action_requesting_osd_name);
        SendEvent(UpdateEventType.ProgressBar, 70);
        osdName = _lib.GetDeviceOSDName(_address);

        SendEvent(UpdateEventType.StatusText, Resources.action_requesting_menu_language);
        SendEvent(UpdateEventType.ProgressBar, 80);
        menuLanguage = _lib.GetDeviceMenuLanguage(_address);
      }

      SendEvent(UpdateEventType.StatusText, Resources.action_showing_device_information);
      SendEvent(UpdateEventType.ProgressBar, 90);
      SendEvent(UpdateEventType.ProcessCompleted, true);

      ShowDialog(_address, ref _lib, devicePresent, vendor, isActiveSource, physicalAddress, version, power, osdName, menuLanguage);

      SendEvent(UpdateEventType.StatusText, Resources.ready);
      SendEvent(UpdateEventType.ProgressBar, 100);
    }

    private readonly CECController _controller;
    private LibCecSharp _lib;
    private readonly CecLogicalAddress _address;
  }
}
