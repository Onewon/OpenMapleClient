package openMapleClient

import android.content.Context
import android.content.res.Configuration
import android.hardware.input.InputManager
import android.os.Handler
import android.os.Looper
import android.view.InputDevice

class KeyboardMonitor(private val context: Context, private val changed: (Boolean) -> Unit) : InputManager.InputDeviceListener {
    private val manager = context.getSystemService(InputManager::class.java)
    fun start() {
        manager.registerInputDeviceListener(this, Handler(Looper.getMainLooper()))
        refresh()
    }
    fun stop() = manager.unregisterInputDeviceListener(this)
    fun refresh() = changed(isAvailable(context))
    override fun onInputDeviceAdded(id: Int) = refresh()
    override fun onInputDeviceRemoved(id: Int) = refresh()
    override fun onInputDeviceChanged(id: Int) = refresh()

    companion object {
        fun isAvailable(context: Context): Boolean {
            val configuration = context.resources.configuration
            if (configuration.hardKeyboardHidden == Configuration.HARDKEYBOARDHIDDEN_YES) return false
            val devicePresent = InputDevice.getDeviceIds().any { id ->
                InputDevice.getDevice(id)?.let {
                    !it.isVirtual && it.keyboardType == InputDevice.KEYBOARD_TYPE_ALPHABETIC &&
                        it.supportsSource(InputDevice.SOURCE_KEYBOARD)
                } ?: false
            }
            return devicePresent || (configuration.keyboard == Configuration.KEYBOARD_QWERTY &&
                configuration.hardKeyboardHidden == Configuration.HARDKEYBOARDHIDDEN_NO)
        }
        fun controlsVisible(context: Context, keyboard: Boolean): Boolean =
            when (context.getSharedPreferences("input", Context.MODE_PRIVATE).getString("visibility", "auto")) {
                "show" -> true
                "hide" -> false
                else -> !keyboard
            }
    }
}
