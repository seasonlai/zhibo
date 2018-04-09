package com.season.example.learnffmpeg.pusher;

/**
 * Created by david on 2017/10/11.
 */

public abstract class Pusher {

    boolean isPushing = false;

    PusherNative mNative;
    LiveStateChangeListener mListener;

    protected Pusher(PusherNative pusherNative) {
        this.mNative = pusherNative;
    }


    public void setLiveStateChangeListener(LiveStateChangeListener listener) {
        mListener = listener;
    }

    public abstract void startPush();

    public abstract void stopPush();

    public abstract void release();


}
