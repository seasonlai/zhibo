package com.season.example.learnffmpeg;

import android.annotation.TargetApi;
import android.hardware.Camera;
import android.os.Build;
import android.os.Handler;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import com.season.example.learnffmpeg.pusher.LivePusher;
import com.season.example.learnffmpeg.pusher.LiveStateChangeListener;

public class Main3Activity extends AppCompatActivity implements LiveStateChangeListener,
        View.OnClickListener, SurfaceHolder.Callback {
    private Button button01;
    private SurfaceView mSurfaceView;
    private SurfaceHolder mSurfaceHolder;
    private boolean isStart;
    private LivePusher livePusher;
    private Handler mHandler = new Handler() {
        public void handleMessage(android.os.Message msg) {
            switch (msg.what) {

                case -100:
                    Toast.makeText(Main3Activity.this, "视频预览开始失败", 0).show();
                    livePusher.stopPush();
                    break;
                case -101:
                    Toast.makeText(Main3Activity.this, "音频录制失败", Toast.LENGTH_SHORT).show();
                    livePusher.stopPush();
                    break;
                case -102:
                    Toast.makeText(Main3Activity.this, "音频编码器配置失败", Toast.LENGTH_SHORT).show();
                    livePusher.stopPush();
                    break;
                case -103:
                    Toast.makeText(Main3Activity.this, "视频频编码器配置失败", Toast.LENGTH_SHORT).show();
                    livePusher.stopPush();
                    break;
                case -104:
                    Toast.makeText(Main3Activity.this, "流媒体服务器/网络等问题", Toast.LENGTH_SHORT).show();
                    livePusher.stopPush();
                    break;
            }
            button01.setText("推流");
            isStart = false;
        }
    };

//    @SuppressWarnings("deprecation")
//    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main3);
        button01 = (Button) findViewById(R.id.button_first);
        button01.setOnClickListener(this);
        findViewById(R.id.button_take).setOnClickListener(
                new View.OnClickListener() {

                    @Override
                    public void onClick(View v) {
                        livePusher.switchCamera();
                    }
                });
        mSurfaceView = (SurfaceView) this.findViewById(R.id.surface);
        mSurfaceHolder = mSurfaceView.getHolder();
        mSurfaceHolder.addCallback(this);
        livePusher = new LivePusher(this, 800, 480, 1200_000, 10,
                Camera.CameraInfo.CAMERA_FACING_BACK);
        livePusher.setLiveStateChangeListener(this);
        livePusher.prepare(mSurfaceHolder);
    }

    // @Override
    // public void onRequestPermissionsResult(int requestCode,
    // String[] permissions, int[] grantResults) {
    // super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    // }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        livePusher.release();
    }

    @Override
    public void onClick(View v) {
        if (isStart) {
            button01.setText("推流");
            isStart = false;
            livePusher.stopPush();
        } else {
            button01.setText("停止");
            isStart = true;
            livePusher.startPush("rtmp://120.79.12.83/myapp/mystream");
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        System.out.println("MAIN: CREATE");
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width,
                               int height) {
        System.out.println("MAIN: CHANGE");
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        System.out.println("MAIN: DESTORY");
    }

    /**
     * 可能运行在子线程
     */
    @Override
    public void onErrorPusher(int code) {
        System.out.println("code:" + code);
        mHandler.sendEmptyMessage(code);
    }


    /**
     * 可能运行在子线程
     */
    @Override
    public void onStartPusher() {
        Log.d("Main3Activity", "开始推流");
    }

    /**
     * 可能运行在子线程
     */
    @Override
    public void onStopPusher() {
        Log.d("Main3Activity", "结束推流");
    }

    @Override
    public void onPointerCaptureChanged(boolean hasCapture) {

    }
}
