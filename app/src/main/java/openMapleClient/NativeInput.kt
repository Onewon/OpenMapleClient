package openMapleClient

object NativeInput {
    init { System.loadLibrary("OpenMapleClient") }
    external fun update(keyboard: Boolean, visible: Boolean)
    external fun setControlsOpacity(opacity: Int)
    external fun releaseKeys()
    external fun takeSettingsRequest(): Boolean
    external fun takeConnectionLost(): Boolean
}
