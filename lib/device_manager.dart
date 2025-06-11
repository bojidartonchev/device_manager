import 'dart:async';

import 'package:device_manager/device_event.dart';
import 'package:flutter/cupertino.dart';
import 'package:flutter/services.dart';

class DeviceManager with ChangeNotifier {
  static const int deviceInterface = 0x00000005;
  static const int handle = 0x00000006;
  static const int oem = 0x00000000;
  static const int port = 0x00000003;
  static const int volume = 0x00000002;

  static final DeviceManager _singleton = DeviceManager._internal();

  final MethodChannel _channel = const MethodChannel('device_manager');
  final EventChannel _eventChannel = const EventChannel('device_manager/events');

  factory DeviceManager() {
    return _singleton;
  }

  DeviceEvent? _lastEvent;

  DeviceManager._internal() {
    _eventChannel.receiveBroadcastStream().listen((msg) {
      if (msg['event'] == "device_added") {
        var ev = DeviceEvent(msg['productId'], EventType.add);
        if (_lastEvent == null || _lastEvent != ev) {
          _lastEvent = ev;
          notifyListeners();
        }
      } else if (msg['event'] == "device_removed") {
        var ev = DeviceEvent(msg['productId'], EventType.remove);
        if (_lastEvent == null || _lastEvent != ev) {
          _lastEvent = ev;
          notifyListeners();
        }
      }
    });

    _channel.setMethodCallHandler((call) {
      switch (call.method) {
        case "device_added":
          {
            int deviceType = call.arguments;
            var ev = DeviceEvent(deviceType, EventType.add);
            if (_lastEvent == null || _lastEvent != ev) {
              _lastEvent = ev;
              notifyListeners();
            }

            break;
          }
        case "device_removed":
          {
            int deviceType = call.arguments;
            var ev = DeviceEvent(deviceType, EventType.remove);
            if (_lastEvent == null || _lastEvent != ev) {
              _lastEvent = ev;
              notifyListeners();
            }
            break;
          }
      }

      return Future.value(0);
    });
  }

  Future<int?> get devicesCount async {
    final int? count = await _channel.invokeMethod('get_devices_count');
    return count;
  }

  DeviceEvent? get lastEvent => _lastEvent;
}
