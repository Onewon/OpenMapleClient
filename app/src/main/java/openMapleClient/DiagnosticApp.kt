package openMapleClient

import android.app.Application
import android.app.ActivityManager
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.Process
import android.util.Log
import androidx.core.content.FileProvider
import java.io.File
import java.util.zip.ZipEntry
import java.util.zip.ZipOutputStream

class DiagnosticApp : Application() {
    override fun onCreate() {
        super.onCreate()
        val directory = File(filesDir, "diagnostics").apply { mkdirs() }
        directory.listFiles()?.sortedByDescending { it.lastModified() }?.drop(4)?.forEach { it.delete() }
        val file = File(directory, "startup-${System.currentTimeMillis()}.txt")
        file.appendText("Device: ${Build.MANUFACTURER} ${Build.MODEL}\nAndroid: ${Build.VERSION.RELEASE} SDK ${Build.VERSION.SDK_INT}\nABIs: ${Build.SUPPORTED_ABIS.joinToString()}\n")
        val manager = getSystemService(Context.ACTIVITY_SERVICE) as ActivityManager
        manager.getHistoricalProcessExitReasons(packageName, 0, 3).forEach {
            file.appendText("Previous exit: reason=${it.reason} status=${it.status} time=${it.timestamp} description=${it.description}\n")
        }
        val previous = Thread.getDefaultUncaughtExceptionHandler()
        Thread.setDefaultUncaughtExceptionHandler { thread, error ->
            runCatching { file.appendText("\nUncaught on ${thread.name}\n${Log.getStackTraceString(error)}\n") }
            previous?.uncaughtException(thread, error)
        }
        Thread {
            try {
                val process = ProcessBuilder("logcat", "--pid=${Process.myPid()}", "-v", "threadtime", "-T", "1")
                    .redirectErrorStream(true).start()
                process.inputStream.bufferedReader().use { reader ->
                    file.appendText("Log capture started\n")
                    reader.forEachLine { line ->
                        if (file.length() < 8L * 1024 * 1024) file.appendText(line + "\n")
                    }
                }
            } catch (error: Exception) {
                file.appendText("Log capture failed: $error\n")
            }
        }.apply { isDaemon = true; name = "DiagnosticLog" }.start()
        Log.i("OpenMapleClient", "Diagnostic capture enabled")
    }

    companion object {
        fun share(context: Context) {
            val folder = File(context.cacheDir, "shared-logs").apply { mkdirs() }
            val archive = File(folder, "BeiDou083-diagnostics.zip")
            ZipOutputStream(archive.outputStream()).use { zip ->
                File(context.filesDir, "diagnostics").listFiles()?.forEach { file ->
                    zip.putNextEntry(ZipEntry(file.name))
                    file.inputStream().use { it.copyTo(zip) }
                    zip.closeEntry()
                }
            }
            val uri = FileProvider.getUriForFile(context, "${context.packageName}.diagnostics", archive)
            context.startActivity(Intent.createChooser(Intent(Intent.ACTION_SEND).apply {
                type = "application/zip"
                putExtra(Intent.EXTRA_STREAM, uri)
                clipData = android.content.ClipData.newRawUri("Diagnostic logs", uri)
                addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            }, "分享启动日志"))
        }
    }
}
