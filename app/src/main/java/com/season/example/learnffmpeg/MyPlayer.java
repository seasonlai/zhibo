package com.season.example.learnffmpeg;

import android.graphics.PixelFormat;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

import java.io.File;

/**
 * Created by season on 2018/4/8.
 */

public class MyPlayer implements SurfaceHolder.Callback {


    static {
        System.loadLibrary("play");
    }

    private static final String TAG = "MyPlayer";

    private SurfaceHolder mHolder;
    public MyPlayer(SurfaceHolder holder){
        this.mHolder=holder;
        mHolder.setFormat(PixelFormat.RGBA_8888);
        mHolder.addCallback(this);
    }

    public void play(final File file){
        if(file==null||!file.exists()){
            Log.e(TAG, "play: 文件为空或不存在");
            return;
        }
        play0(file.getAbsolutePath());
    }

    public void play(final String path) {
        play0(path);
    }

    public void stop(){
        stop0();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        initNativeWindow(holder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    private native void initNativeWindow(Surface surface);

    private native void play0(String path);

    private native void stop0();
}
