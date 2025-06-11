package com.example.device_manager

import android.content.*
import android.hardware.usb.UsbManager
import io.flutter.embedding.engine.plugins.FlutterPlugin
import io.flutter.plugin.common.MethodCall
import io.flutter.plugin.common.MethodChannel
import io.flutter.plugin.common.EventChannel
import android.util.Log
import android.os.Handler
import android.os.Looper
import android.hardware.usb.UsbDevice
/** DeviceManagerPlugin */
class DeviceManagerPlugin : FlutterPlugin, MethodChannel.MethodCallHandler, EventChannel.StreamHandler {

  private lateinit var context: Context
  private lateinit var methodChannel: MethodChannel
  private lateinit var eventChannel: EventChannel
  private var receiver: BroadcastReceiver? = null
  private var eventSink: EventChannel.EventSink? = null

  private val connectedDevices = mutableSetOf<String>()
  private val handler = Handler(Looper.getMainLooper())
  private var debounceRunnable: Runnable? = null

  private fun deviceId(device: UsbDevice): String {
    return "${device.vendorId}:${device.productId}"
  }

  override fun onAttachedToEngine(binding: FlutterPlugin.FlutterPluginBinding) {
    context = binding.applicationContext
    methodChannel = MethodChannel(binding.binaryMessenger, "device_manager")
    methodChannel.setMethodCallHandler(this)

    eventChannel = EventChannel(binding.binaryMessenger, "device_manager/events")
    eventChannel.setStreamHandler(this)
  }

  override fun onDetachedFromEngine(binding: FlutterPlugin.FlutterPluginBinding) {
    methodChannel.setMethodCallHandler(null)
    eventChannel.setStreamHandler(null)
    if (receiver != null) {
      context.unregisterReceiver(receiver)
    }
  }

  override fun onMethodCall(call: MethodCall, result: MethodChannel.Result) {
    when (call.method) {
      "get_devices_count" -> {
        Log.d("onMethodCall", "get_devices_count called")
        val usbManager = context.getSystemService(Context.USB_SERVICE) as UsbManager
        result.success(usbManager.deviceList.size)
      }
      else -> {
        result.notImplemented()
      }
    }
  }

  override fun onListen(arguments: Any?, events: EventChannel.EventSink?) {
    Log.d("DeviceManagerPlugin", "onListen called — registering receiver")

    eventSink = events

    receiver = object : BroadcastReceiver() {
      override fun onReceive(context: Context?, intent: Intent?) {
        val usbDevice = intent?.getParcelableExtra<UsbDevice>(UsbManager.EXTRA_DEVICE) ?: return
        val id = deviceId(usbDevice)

        when (intent.action) {
          UsbManager.ACTION_USB_DEVICE_ATTACHED -> {
            if (connectedDevices.contains(id)) {
              // Already connected, ignore duplicate
              return
            }
            connectedDevices.add(id)
            sendEventWithDebounce("device_added", usbDevice)
          }
          UsbManager.ACTION_USB_DEVICE_DETACHED -> {
            if (!connectedDevices.contains(id)) {
              // Device not known connected, ignore
              return
            }
            connectedDevices.remove(id)
            sendEventWithDebounce("device_removed", usbDevice)
          }
        }
      }
    }

    val usbManager = context.getSystemService(Context.USB_SERVICE) as UsbManager
    val currentDevices = usbManager.deviceList.values

    for (device in currentDevices) {
      val id = deviceId(device)
      connectedDevices.add(id)
    }

    val filter = IntentFilter().apply {
      addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED)
      addAction(UsbManager.ACTION_USB_DEVICE_DETACHED)
    }

    context.registerReceiver(receiver, filter)
  }

  private fun sendEventWithDebounce(eventType: String, device: UsbDevice) {
    debounceRunnable?.let { handler.removeCallbacks(it) }
    debounceRunnable = Runnable {
      val event = mapOf(
              "event" to eventType,
              "vendorId" to device.vendorId,
              "productId" to device.productId,
              "deviceClass" to device.deviceClass
              // add more fields if needed
      )
      eventSink?.success(event)
    }

    handler.postDelayed(debounceRunnable!!, 300) // 300ms debounce
  }

  override fun onCancel(arguments: Any?) {
    if (receiver != null) {
      context.unregisterReceiver(receiver)
      receiver = null
    }
    eventSink = null
  }
}