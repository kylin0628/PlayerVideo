package com.kylin.playervideo

import android.os.Build
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.os.Environment
import android.view.Gravity
import android.view.View
import android.view.WindowManager
import android.widget.LinearLayout
import android.widget.SeekBar
import androidx.annotation.RequiresApi
import androidx.appcompat.widget.LinearLayoutCompat
import com.blankj.utilcode.constant.PermissionConstants
import com.blankj.utilcode.util.PermissionUtils
import com.blankj.utilcode.util.ScreenUtils
import com.blankj.utilcode.util.ToastUtils
import kotlinx.android.synthetic.main.activity_main.*
import java.io.File

class MainActivity : AppCompatActivity() {

    var isTouch = false
    var isSeek = false
    var seekProgress: Int = 0
    private val videoPlayer by lazy { VideoPlayer() }
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setFlags(
            WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
            WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON
        )
        setContentView(R.layout.activity_main)
        PermissionUtils.permission(PermissionConstants.STORAGE)
            .callback(object : PermissionUtils.SimpleCallback {
                override fun onGranted() {
                    ToastUtils.showLong("onGranted")
                }

                override fun onDenied() {
                    ToastUtils.showLong("onDenied")
                }
            }).request()


        seek_bar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(p0: SeekBar?, p1: Int, p2: Boolean) {

            }

            override fun onStartTrackingTouch(p0: SeekBar?) {
                isTouch = true
            }

            override fun onStopTrackingTouch(p0: SeekBar?) {
                isTouch = false
                isSeek = true
                seekProgress = videoPlayer.getDuration() * (p0?.progress ?: 1) / 100
                videoPlayer.setSeek(seekProgress)
            }
        })

        //设置surfaceview
        videoPlayer.setSurfaceView(surface_view)
        val file = File(Environment.getExternalStorageDirectory(), "video.mp4")
        videoPlayer.setDataSource(file.absolutePath)
        videoPlayer.setPrepareListener(object : VideoPlayer.OnPrepareListener {
            override fun onPrepared() {
                videoPlayer.start()
                runOnUiThread {
                    if (videoPlayer.getDuration() > 0) {
                        seek_bar.visibility = View.VISIBLE
                    } else {
                        seek_bar.visibility = View.GONE
                    }
                    tv_time.text =
                        "${videoPlayer.getDuration()}s"
                }
            }
        })
        videoPlayer.setProgressListener(object : VideoPlayer.OnProgressListener {
            @RequiresApi(Build.VERSION_CODES.N)
            override fun onProgress(progress: Int) {
                runOnUiThread {
                    val duration = videoPlayer.getDuration()
                    //如果是直播
                    if (duration > 0) {
                        if (isSeek) {
                            isSeek = false
                            return@runOnUiThread
                        }
                        if (!isTouch) {
                            val seekProgress = progress * 100 / duration
                            seek_bar.setProgress(seekProgress, true)
                        }
                    }
                }
            }
        })
        videoPlayer.setAdaptiveSizeListener(object : VideoPlayer.OnAdaptiveSize {
            override fun updateSize(den: Int, num: Int) {
//                runOnUiThread {
//                    val params = LinearLayoutCompat.LayoutParams(
//                        ScreenUtils.getScreenWidth(),
//                        (ScreenUtils.getScreenWidth() * den.toFloat() / num).toInt()
//                    )
//                    params.gravity = Gravity.CENTER
//                    surface_view.layoutParams = params
//                }
            }
        })
    }


    fun play(view: View) {
        videoPlayer.prepare()
    }

    fun stop(view: View) {
        videoPlayer.stop()
    }

    fun pause(view: View) {
        videoPlayer.pause()
    }

    fun rewind(view: View) {
        videoPlayer.rewind()
    }

    fun fastForward(view: View) {
        videoPlayer.fastForward()
    }
}