package com.summer.crashsdk;

import android.content.Context;
import android.os.Build;
import android.util.Log;

import java.io.File;

public final class CrashSDK {

    private static final String TAG = "CrashSDK";

    private static String LOG_DIR;

    /**
     * init crash sdk,
     * @param context
     * @return 0 on success, otherwise failed.
     */
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
            try{
                if(!dir.mkdirs()){
                    Log.e(TAG,"init crash log directory failed");
                }
            }catch (Throwable t){
                t.printStackTrace();
                Log.e(TAG,"init crash log directory failed");
            }
        }

        LOG_DIR = dir.getAbsolutePath();
        setLogDir(dir.getAbsolutePath());

        return initNative();
    }

    /**
     * get the path where crash info saved
     * @return
     */
    public static String getTombstonesDirectory(){
        return LOG_DIR;
    }

    public static void doCrash(int id){
        test(id);
    }

    private static native int initNative();
    private static native void setSystemInfo(String fingerprint, int version, String supportedABI);
    private static native void setLogDir(String dir);

    private static native int test(int id);

}
