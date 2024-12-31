enum EventType {
  add,
  remove,
}

class DeviceEvent {
  final int _deviceType;
  final EventType _eventType;

  DeviceEvent(this._deviceType, this._eventType);

  EventType get eventType => _eventType;
  int get deviceType => _deviceType;

  @override
  bool operator ==(Object other) =>
      other is DeviceEvent &&
      other.eventType == eventType &&
      other.deviceType == deviceType;

  @override
  int get hashCode {
    return Object.hash(eventType.hashCode, deviceType.hashCode);
  }
}
