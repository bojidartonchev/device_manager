import 'package:device_manager/device_manager.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  const MethodChannel channel = MethodChannel('device_manager');

  TestWidgetsFlutterBinding.ensureInitialized();

  final defaultBinaryMessenger =
      TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger;

  setUp(() {
    defaultBinaryMessenger.setMockMethodCallHandler(channel,
        (MethodCall methodCall) async {
      return 0;
    });
  });

  tearDown(() {
    defaultBinaryMessenger.setMockMethodCallHandler(channel, null);
  });

  test('getDevicesCount', () async {
    expect(await DeviceManager().devicesCount, 0);
  });
}
