package com.summer.crashsdk;

public final class CrashSDK {

    private static final String TAG = "CrashSDK";

    static {
        System.loadLibrary("crashsdk");
    }

    public static final int init(){
        return initNative();
    }

    private static native int initNative();

    public static native int test();

}
