package com.season.example.learnffmpeg;

import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.SurfaceView;
import android.view.View;

import java.io.File;

public class Main2Activity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);

        final SurfaceView surfaceView = findViewById(R.id.surfaceView);
        final MyPlayer myPlayer = new MyPlayer(surfaceView.getHolder());


        final File input = new File(Environment.getExternalStorageDirectory(), "input.mp4");
        findViewById(R.id.button).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
//                myPlayer.play(input);
                myPlayer.play("rtmp://120.79.12.83/myapp/mystream");
            }
        });
        findViewById(R.id.button2).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                myPlayer.stop();
            }
        });
    }

}
