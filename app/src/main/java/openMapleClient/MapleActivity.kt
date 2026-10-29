package openMapleClient

import android.app.NativeActivity
import android.os.Bundle
import android.content.Intent
import android.content.res.Configuration
import android.os.Handler
import android.os.Looper
import android.view.inputmethod.InputMethodManager

class MapleActivity : NativeActivity() {
    private lateinit var monitor: KeyboardMonitor
    private val handler = Handler(Looper.getMainLooper())
    private val settingsPoll = object : Runnable {
        override fun run() {
            if (NativeInput.takeConnectionLost() && !isFinishing) {
                NativeInput.releaseKeys()
                android.app.AlertDialog.Builder(this@MapleActivity)
                    .setTitle("连接已断开")
                    .setMessage("与服务器的连接已断开，请关闭并重新打开客户端登录。可分享启动日志检查原因。")
                    .setCancelable(false)
                    .setPositiveButton("关闭游戏") { _, _ ->
                        finishAffinity()
                        android.os.Process.killProcess(android.os.Process.myPid())
                    }
                    .show()
                return
            }
            if (NativeInput.takeSettingsRequest()) {
                startActivity(Intent(this@MapleActivity, InputSettingsActivity::class.java))
            }
            handler.postDelayed(this, 200)
        }
    }
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        monitor = KeyboardMonitor(this) { available ->
            NativeInput.update(available, KeyboardMonitor.controlsVisible(this, available))
            if (available) {
                getSystemService(InputMethodManager::class.java).hideSoftInputFromWindow(window.decorView.windowToken, 0)
            }
        }
    }
    override fun onResume() {
        super.onResume()
        NativeInput.setControlsOpacity(getSharedPreferences("input", MODE_PRIVATE).getInt("opacity", 80))
        monitor.start()
        handler.post(settingsPoll)
    }
    override fun onPause() {
        handler.removeCallbacks(settingsPoll)
        monitor.stop()
        NativeInput.releaseKeys()
        super.onPause()
    }
    override fun onConfigurationChanged(configuration: Configuration) {
        super.onConfigurationChanged(configuration)
        monitor.refresh()
    }
}
