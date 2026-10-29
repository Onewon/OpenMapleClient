package openMapleClient

import android.os.Bundle
import android.content.res.Configuration
import android.widget.LinearLayout
import android.widget.RadioButton
import android.widget.RadioGroup
import android.widget.TextView
import android.widget.Button
import android.widget.ScrollView
import android.widget.SeekBar
import androidx.appcompat.app.AppCompatActivity
import OpenMapleClient.R

class InputSettingsActivity : AppCompatActivity() {
    private lateinit var monitor: KeyboardMonitor
    private lateinit var status: TextView
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val preferences = getSharedPreferences("input", MODE_PRIVATE)
        val layout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            val padding = (20 * resources.displayMetrics.density).toInt()
            setPadding(padding, padding, padding, padding)
        }
        status = TextView(this)
        layout.addView(TextView(this).apply { setText(R.string.input_settings); textSize = 22f })
        layout.addView(status)
        layout.addView(TextView(this).apply { setText(R.string.controls_visibility) })
        val group = RadioGroup(this)
        val modes = listOf("auto", "show", "hide")
        val labels = listOf(R.string.controls_auto, R.string.controls_show, R.string.controls_hide)
        modes.forEachIndexed { index, mode ->
            group.addView(RadioButton(this).apply {
                id = index + 1
                setText(labels[index])
                isChecked = preferences.getString("visibility", "auto") == mode
            })
        }
        group.setOnCheckedChangeListener { _, id ->
            preferences.edit().putString("visibility", modes[id - 1]).apply()
        }
        layout.addView(group)
        val opacityLabel = TextView(this)
        val savedOpacity = preferences.getInt("opacity", 80).coerceIn(10, 100)
        opacityLabel.text = getString(R.string.controls_opacity, savedOpacity)
        layout.addView(opacityLabel)
        layout.addView(SeekBar(this).apply {
            max = 90
            progress = savedOpacity - 10
            setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
                override fun onProgressChanged(bar: SeekBar?, progress: Int, fromUser: Boolean) {
                    val value = progress + 10
                    opacityLabel.text = getString(R.string.controls_opacity, value)
                    if (fromUser) {
                        preferences.edit().putInt("opacity", value).apply()
                        NativeInput.setControlsOpacity(value)
                    }
                }
                override fun onStartTrackingTouch(bar: SeekBar?) {}
                override fun onStopTrackingTouch(bar: SeekBar?) {}
            })
        })
        layout.addView(TextView(this).apply { setText(R.string.controls_opacity_help) })
        layout.addView(TextView(this).apply { setText(R.string.keyboard_help) })
        layout.addView(Button(this).apply { setText(R.string.settings_done); setOnClickListener { finish() } })
        setContentView(ScrollView(this).apply { addView(layout) })
        monitor = KeyboardMonitor(this) { available ->
            status.setText(if (available) R.string.keyboard_connected else R.string.keyboard_disconnected)
        }
    }
    override fun onStart() { super.onStart(); monitor.start() }
    override fun onStop() { monitor.stop(); super.onStop() }
    override fun onConfigurationChanged(configuration: Configuration) {
        super.onConfigurationChanged(configuration)
        monitor.refresh()
    }
}
