package com.openmaple.nxinstaller

import android.app.Activity
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.os.Environment
import android.provider.Settings
import android.view.Gravity
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ProgressBar
import android.widget.TextView
import java.io.BufferedInputStream
import java.io.BufferedOutputStream
import java.io.File
import java.io.FileOutputStream
import java.util.zip.ZipInputStream

class InstallerActivity : Activity() {
    private val required = setOf(
        "Base.nx", "Character.nx", "Effect.nx", "Etc.nx", "Item.nx", "Map.nx",
        "MapPretty.nx", "MapLatest.nx", "Map001.nx", "Mob.nx", "Morph.nx", "Npc.nx",
        "Quest.nx", "Reactor.nx", "Skill.nx", "Sound.nx", "String.nx", "TamingMob.nx", "UI.nx"
    )
    private lateinit var status: TextView
    private lateinit var progress: ProgressBar
    private lateinit var action: Button

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val layout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(40, 32, 40, 32)
        }
        status = TextView(this).apply { textSize = 18f }
        progress = ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal).apply {
            isIndeterminate = false
            max = 100
            progress = 0
        }
        action = Button(this)
        layout.addView(TextView(this).apply {
            text = "BeiDou v83 NX Resource Installer"
            textSize = 22f
        })
        layout.addView(status)
        layout.addView(progress)
        layout.addView(action)
        setContentView(layout)
        refresh()
    }

    override fun onResume() {
        super.onResume()
        refresh()
    }

    private fun refresh() {
        if (!::status.isInitialized) return
        val archive = File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS), ARCHIVE_NAME)
        if (!Environment.isExternalStorageManager()) {
            status.text = "First, allow All files access for this installer."
            action.text = "Open storage access settings"
            action.isEnabled = true
            action.setOnClickListener {
                startActivity(Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION).apply {
                    data = Uri.parse("package:$packageName")
                })
            }
        } else if (!archive.isFile) {
            status.text = "Place $ARCHIVE_NAME in Downloads, then return here."
            action.text = "Check again"
            action.isEnabled = true
            action.setOnClickListener { refresh() }
        } else {
            status.text = "Ready to install from Downloads/$ARCHIVE_NAME"
            action.text = "Install NX resources"
            action.isEnabled = true
            action.setOnClickListener { install(archive) }
        }
    }

    private fun install(archive: File) {
        action.isEnabled = false
        progress.progress = 0
        status.text = "Extracting NX resources. Keep this app open."
        Thread {
            var currentEntry = "archive"
            try {
                val storage = Environment.getExternalStorageDirectory()
                val destination = File(storage, "Android/media/com.OpenMapleClient/NX")
                check(destination.mkdirs() || destination.isDirectory) { "Cannot create shared NX directory." }
                val marker = File(destination, ".nx-installing")
                check(marker.createNewFile() || marker.isFile) { "Cannot create installation marker." }
                val found = mutableSetOf<String>()
                ZipInputStream(BufferedInputStream(archive.inputStream(), BUFFER_SIZE)).use { zip ->
                    var entry = zip.nextEntry
                    while (entry != null) {
                        val name = if (entry.isDirectory) "" else entry.name.substringAfterLast('/')
                        if (name in required) {
                            check(found.add(name)) { "Duplicate file in archive: $name" }
                            currentEntry = name
                            runOnUiThread { status.text = "Installing $name. Keep this app open." }
                            val target = File(destination, name)
                            val replacement = File(destination, "$name.installing")
                            val backup = File(destination, "$name.previous")
                            if (!target.exists() && backup.exists()) backup.renameTo(target)
                            if (target.exists()) backup.delete()
                            replacement.delete()
                            BufferedOutputStream(FileOutputStream(replacement), BUFFER_SIZE).use { out ->
                                val buffer = ByteArray(BUFFER_SIZE)
                                while (true) {
                                    val count = zip.read(buffer)
                                    if (count < 0) break
                                    out.write(buffer, 0, count)
                                }
                            }
                            zip.closeEntry()
                            if (target.exists()) check(target.renameTo(backup)) { "Cannot preserve existing $name" }
                            if (!replacement.renameTo(target)) {
                                if (backup.exists()) backup.renameTo(target)
                                error("Cannot install $name")
                            }
                            backup.delete()
                            runOnUiThread { progress.progress = found.size * 100 / required.size }
                        } else {
                            zip.closeEntry()
                        }
                        entry = zip.nextEntry
                    }
                }
                val missing = required - found
                check(missing.isEmpty()) { "Archive is missing: ${missing.joinToString()}" }
                marker.delete()
                runOnUiThread {
                    progress.progress = 100
                    status.text = "Installed 19 NX files to Android/media/com.OpenMapleClient/NX. You can now open the game."
                    action.text = "Done"
                    action.isEnabled = true
                    action.setOnClickListener { finish() }
                }
            } catch (error: Exception) {
                runOnUiThread {
                    status.text = "Installation failed at $currentEntry: ${error.message ?: error.javaClass.simpleName}"
                    action.text = "Try again"
                    action.isEnabled = true
                    action.setOnClickListener { refresh() }
                }
            }
        }.start()
    }

    companion object {
        private const val ARCHIVE_NAME = "OpenMapleClient-NX.zip"
        private const val BUFFER_SIZE = 1024 * 1024
    }
}
