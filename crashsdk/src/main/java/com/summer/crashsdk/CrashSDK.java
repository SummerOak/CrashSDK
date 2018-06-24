package com.summer.crashsdk;

import android.content.Context;
import android.os.Build;

import java.io.File;

public final class CrashSDK {

    private static final String TAG = "CrashSDK";

    public static final int init(Context context){
        System.loadLibrary("crashsdk");

        StringBuilder abi = new StringBuilder();

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            boolean first = true;
            for(String s:Build.SUPPORTED_ABIS){
                if(!first){
                    abi.append(" ");
                }
                abi.append(s);

                first = false;
            }
        }else{
            abi.append(Build.CPU_ABI);
        }

        setSystemInfo(Build.FINGERPRINT, Build.VERSION.SDK_INT, abi.toString());

        File files = context.getExternalFilesDir(null);
        File dir = new File(files.getAbsolutePath() + File.separator + "tombstones");
        if(!dir.exists()){
            dir.mkdirs();
        }

        setLogDir(dir.getAbsolutePath());

        return initNative();
    }

    public static void doCrash(int id){
        test(id);
    }

    private static native int initNative();
    private static native void setSystemInfo(String fingerprint, int version, String supportedABI);
    private static native void setLogDir(String dir);

    private static native int test(int id);

}
