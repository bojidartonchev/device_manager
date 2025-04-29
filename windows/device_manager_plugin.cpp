#include "include/device_manager/device_manager_plugin.h"

#include <windows.h>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <map>
#include <memory>
#include <sstream>
#include <dbt.h>

namespace {

class DeviceManagerPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  DeviceManagerPlugin(flutter::PluginRegistrarWindows *registrar);

  virtual ~DeviceManagerPlugin();

  std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> m_channel;

 private:
  // Called when a method is called on this plugin's channel from Dart.
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  // Called for top-level WindowProc delegation.
  std::optional<LRESULT> HandleWindowProc(HWND hwnd, UINT message,
                                            WPARAM wparam, LPARAM lparam);

  // The registrar for this plugin, for accessing the window.
  flutter::PluginRegistrarWindows *m_registrar;

  // The ID of the WindowProc delegate registration.
  int m_windowProcId = -1;
  bool m_registered = false;
};

// static
void DeviceManagerPlugin::RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar) {
  auto plugin = std::make_unique<DeviceManagerPlugin>(registrar);

  plugin.get()->m_channel = std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
    registrar->messenger(), "device_manager",
    &flutter::StandardMethodCodec::GetInstance());

  plugin.get()->m_channel->SetMethodCallHandler(
    [plugin_pointer = plugin.get()](const auto &call, auto result) {
    plugin_pointer->HandleMethodCall(call, std::move(result));
  });

  registrar->AddPlugin(std::move(plugin));
}

DeviceManagerPlugin::DeviceManagerPlugin(flutter::PluginRegistrarWindows *registrar)
: m_registrar(registrar) {
    m_windowProcId = m_registrar->RegisterTopLevelWindowProcDelegate(
      [this](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
        return HandleWindowProc(hwnd, message, wparam, lparam);
      });

    //Register for WM_DEVICECHANGE
    if(!m_registered) {
        HWND hwnd = ::GetActiveWindow();
        if (hwnd) {
            m_registered = true;

            DEV_BROADCAST_DEVICEINTERFACE notificationFilter;
            ZeroMemory(&notificationFilter, sizeof(notificationFilter));
            notificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
            notificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;

            RegisterDeviceNotification(
                    hwnd,
                    &notificationFilter,
                    DEVICE_NOTIFY_ALL_INTERFACE_CLASSES
            );
        }
    }
}

DeviceManagerPlugin::~DeviceManagerPlugin() {
    m_registrar->UnregisterTopLevelWindowProcDelegate(m_windowProcId);
}

void DeviceManagerPlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {

  if (method_call.method_name().compare("get_devices_count") == 0) {
    UINT32 nDevices = 0;
    GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST));

    result->Success(flutter::EncodableValue((int)nDevices));
  } else {
    result->NotImplemented();
  }
}

std::optional<LRESULT> DeviceManagerPlugin::HandleWindowProc(HWND hwnd,
                                                          UINT message,
                                                          WPARAM wparam,
                                                          LPARAM lparam) {
  std::optional<LRESULT> result;
  switch (message) {
    case WM_DEVICECHANGE:
      {
        if(wparam == DBT_DEVICEARRIVAL)
        {
          PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lparam;
          int deviceType = (int)(pHdr->dbch_devicetype);
          const flutter::EncodableValue argument = flutter::EncodableValue(deviceType);
          m_channel->InvokeMethod("device_added", std::make_unique<flutter::EncodableValue>(argument), nullptr);
        }
        else if(wparam == DBT_DEVICEREMOVECOMPLETE)
        {
          PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)lparam;
          int deviceType = (int)(pHdr->dbch_devicetype);
          const flutter::EncodableValue argument = flutter::EncodableValue(deviceType);
          m_channel->InvokeMethod("device_removed", std::make_unique<flutter::EncodableValue>(argument), nullptr);
        }
      }
      break;
  }
  return result;
}

}  // namespace

void DeviceManagerPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  DeviceManagerPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
