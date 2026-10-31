package openMapleClient

import android.content.Intent
import android.os.Bundle
import android.os.Build
import android.os.Environment
import android.provider.Settings
import android.net.Uri
import android.text.InputType
import android.widget.Button
import android.widget.CheckBox
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import OpenMapleClient.R
import java.io.File
import java.net.InetSocketAddress
import java.net.Socket

class MainActivity : AppCompatActivity() {
    private val requiredFiles = listOf(
        "Base", "Character", "Effect", "Etc", "Item", "Map", "MapPretty",
        "MapLatest", "Map001", "Mob", "Morph", "Npc", "Quest", "Reactor",
        "Skill", "Sound", "String", "TamingMob", "UI"
    )

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val preferences = getSharedPreferences("connection", MODE_PRIVATE)
        val emulator = Build.FINGERPRINT.contains("generic") || Build.MODEL.contains("sdk_gphone")
        val layout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(32, 20, 32, 20)
        }
        setContentView(ScrollView(this).apply { addView(layout) })
        layout.addView(TextView(this).apply {
            setText(R.string.connection_title)
            textSize = 22f
        })
        val host = EditText(this).apply {
            setHint(R.string.server_host)
            inputType = InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_VARIATION_URI
            setSingleLine()
            setText(preferences.getString("host", if (emulator) "10.0.2.2" else ""))
        }
        val port = EditText(this).apply {
            setHint(R.string.server_port)
            inputType = InputType.TYPE_CLASS_NUMBER
            setSingleLine()
            setText(preferences.getString("port", "18484"))
        }
        val channelHost = CheckBox(this).apply {
            setText(R.string.channel_same_host)
            isChecked = preferences.getBoolean("channel_same_host", true)
        }
        layout.addView(host)
        layout.addView(Button(this).apply {
            text = "分享启动日志"
            setOnClickListener {
                try { DiagnosticApp.share(this@MainActivity) }
                catch (error: Exception) {
                    android.widget.Toast.makeText(this@MainActivity, error.message, android.widget.Toast.LENGTH_LONG).show()
                }
            }
        })
        layout.addView(port)
        layout.addView(channelHost)
        layout.addView(Button(this).apply {
            setText(R.string.input_settings)
            setOnClickListener { startActivity(Intent(this@MainActivity, InputSettingsActivity::class.java)) }
        })
        val status = TextView(this).apply {
            setText(R.string.connection_help)
            setTextIsSelectable(true)
        }
        fun endpoint(): Pair<String, Int>? {
            val address = host.text.toString().trim()
            val number = port.text.toString().toIntOrNull()
            if (address.isEmpty() || number == null || number !in 1..65535) {
                status.setText(R.string.invalid_endpoint)
                return null
            }
            return address to number
        }
        layout.addView(Button(this).apply {
            setText(R.string.check_connection)
            setOnClickListener {
                val (address, number) = endpoint() ?: return@setOnClickListener
                isEnabled = false
                status.setText(R.string.checking_connection)
                Thread {
                    val result = try {
                        Socket().use { socket ->
                            socket.connect(InetSocketAddress(address, number), 5000)
                            socket.soTimeout = 5000
                            val input = socket.getInputStream()
                            val handshake = ByteArray(16)
                            var count = 0
                            while (count < handshake.size) {
                                val read = input.read(handshake, count, handshake.size - count)
                                check(read > 0) { getString(R.string.incomplete_handshake) }
                                count += read
                            }
                            val version = (handshake[2].toInt() and 255) or
                                ((handshake[3].toInt() and 255) shl 8)
                            check(handshake[0].toInt() == 14 && handshake[1].toInt() == 0 &&
                                version == 83 && handshake[15].toInt() == 8) {
                                getString(R.string.unsupported_handshake)
                            }
                            getString(R.string.connection_success, address, number, version)
                        }
                    } catch (error: Exception) {
                        getString(R.string.connection_failure, error.localizedMessage ?: error.javaClass.simpleName)
                    }
                    runOnUiThread {
                        if (!isFinishing && !isDestroyed) {
                            status.text = result
                            isEnabled = true
                        }
                    }
                }.start()
            }
        })
        layout.addView(Button(this).apply {
            setText(R.string.start_game)
            setOnClickListener {
                val (address, number) = endpoint() ?: return@setOnClickListener
                try {
                    val sharedDirectory = File(Environment.getExternalStorageDirectory(),
                        "Android/media/$packageName/NX")
                    val legacyDirectory = checkNotNull(getExternalFilesDir(null))
                    if (Environment.isExternalStorageManager() && File(sharedDirectory, ".nx-installing").exists()) {
                        status.setText(R.string.resource_install_incomplete)
                        return@setOnClickListener
                    }
                    val legacyHasResources = requiredFiles.all { File(legacyDirectory, "$it.nx").isFile }
                    if (!Environment.isExternalStorageManager() && !legacyHasResources) {
                        status.setText(R.string.shared_storage_permission)
                        val intent = Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION).apply {
                            data = Uri.parse("package:$packageName")
                        }
                        startActivity(intent)
                        return@setOnClickListener
                    }
                    val directory = if (Environment.isExternalStorageManager() &&
                        File(sharedDirectory, "Base.nx").isFile) sharedDirectory else legacyDirectory
                    val missing = requiredFiles.filter { !File(directory, "$it.nx").isFile }
                    if (missing.isNotEmpty()) {
                        status.text = getString(R.string.missing_resources, directory.absolutePath,
                            missing.joinToString { "$it.nx" })
                        return@setOnClickListener
                    }
                    File(legacyDirectory, "hostip.txt").writeText(address)
                    File(legacyDirectory, "serverport.txt").writeText(number.toString())
                    File(legacyDirectory, "channelhost.txt").writeText(if (channelHost.isChecked) address else "")
                    android.util.Log.i("OpenMapleClient", "Starting game: endpoint=$address:$number resources=${directory.absolutePath}")
                    preferences.edit().putString("host", address).putString("port", number.toString())
                        .putBoolean("channel_same_host", channelHost.isChecked).apply()
                    startActivity(Intent(this@MainActivity, MapleActivity::class.java))
                } catch (error: Exception) {
                    status.text = getString(R.string.connection_failure, error.localizedMessage ?: error.javaClass.simpleName)
                }
            }
        })
        layout.addView(status)
    }
}
