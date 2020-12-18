package com.kylin.playervideo

import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import kotlin.concurrent.thread

/**
 *@Description:
 *@Auther: wangqi
 * CreateTime: 2020/10/23.
 */
class VideoPlayer : SurfaceHolder.Callback {

    companion object {
        init {
            System.loadLibrary("native-lib")
        }
    }

    external fun native_test(): String
    external fun native_prepare(source: String)
    external fun native_start()
    external fun native_duration(): Int
    external fun native_seek(progress: Int)
    external fun native_stop()
    external fun native_release()
    external fun native_pause()
    external fun native_fastForward()
    external fun native_wind()
    external fun native_setSurface(surface: Surface)

    private var surfaceViewHold: SurfaceHolder? = null
    private var source: String = ""

    fun setSurfaceView(surfaceView: SurfaceView) {
        if (surfaceViewHold != null) {
            surfaceViewHold?.removeCallback(this)
        }
        surfaceViewHold = surfaceView.holder
        surfaceViewHold?.addCallback(this)
    }

    override fun surfaceChanged(
        surfaceHolder: SurfaceHolder,
        format: Int,
        width: Int,
        height: Int
    ) {
    }

    override fun surfaceDestroyed(surfaceHolder: SurfaceHolder) {
    }

    override fun surfaceCreated(surfaceHolder: SurfaceHolder) {

    }

    fun setDataSource(source: String) {
        this.source = source
    }

    //********************************java层调用native层start*************************************
    fun prepare() {
        native_setSurface(surfaceViewHold!!.surface)
        native_prepare(source)
    }

    fun start() {
        native_start()
    }

    fun stop() {
        native_stop()//停止所有native对象线程数据
    }

    fun release() {
        if (surfaceViewHold != null) {
            surfaceViewHold?.removeCallback(this)
        }
        native_release()//释放window
    }

    fun getDuration(): Int {
        return native_duration()
    }

    fun setSeek(progress: Int) {
        thread { native_seek(progress) }
    }
    //*****************************************end************************************************


    //********************************native层通知java层更新ui信息的反射方法start*************************************
    fun onPrepare() {
        prepareListener?.onPrepared()
    }

    fun onProgress(progress: Int) {
        progressListener?.onProgress(progress)
    }

    fun onError(error: Int) {
        errorListener?.onError(error)
    }

    fun onTest(testID: Int) {
    }

    fun adaptiveSize(den: Int, num: Int) {
        adaptiveSize?.updateSize(den, num)
    }


    private var prepareListener: OnPrepareListener? = null
    private var progressListener: OnProgressListener? = null
    private var errorListener: OnErrorListener? = null
    private var adaptiveSize: OnAdaptiveSize? = null

    fun setPrepareListener(prepareListener: OnPrepareListener) {
        this.prepareListener = prepareListener
    }

    fun setProgressListener(progressListener: OnProgressListener) {
        this.progressListener = progressListener
    }

    fun setErrorListener(errorListener: OnErrorListener) {
        this.errorListener = errorListener
    }

    fun setAdaptiveSizeListener(adaptiveSize: OnAdaptiveSize) {
        this.adaptiveSize = adaptiveSize
    }

    fun pause() {
        native_pause()
    }

    fun rewind() {
        native_wind()
    }

    fun fastForward() {
        native_fastForward()
    }

    interface OnPrepareListener {
        fun onPrepared()
    }

    interface OnProgressListener {
        fun onProgress(progress: Int)
    }

    interface OnErrorListener {
        fun onError(error: Int)
    }

    interface OnAdaptiveSize {
        fun updateSize(den: Int, num: Int)
    }
    //*****************************************************end****************************************************
}