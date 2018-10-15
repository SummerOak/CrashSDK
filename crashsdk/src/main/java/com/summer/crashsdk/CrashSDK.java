package com.summer.crashsdk;

import android.content.Context;
import android.os.Build;
import android.os.Debug;
import android.util.Log;

import java.io.BufferedWriter;
import java.io.Closeable;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.text.SimpleDateFormat;
import java.util.Date;

public final class CrashSDK {

    private static final String TAG = "CrashSDK";

    private static String LOG_DIR;
    private static Date sDate;
    private static SimpleDateFormat sFormat;

    private static Thread.UncaughtExceptionHandler sOldUncaughtExceptionHandler = null;

    /**
     * init crash sdk,
     * @param context
     * @return 0 on success, otherwise failed.
     */
    public static final int init(Context context){

        if(!loadLib()){
            return 1;
        }

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

        sDate = new Date();
        sFormat = new SimpleDateFormat("yyyyMMdd_HH_mm_ss");

        initJavaCrashHandler();

        return initNative();
    }

    private static boolean loadLib(){
        try {
            System.loadLibrary("crashsdk21");
        }catch (Throwable t){
            t.printStackTrace();
            try {
                System.loadLibrary("crashsdk");
            }catch (Throwable t2){
                t2.printStackTrace();
                return false;
            }
        }

        return true;
    }

    public static boolean initJavaCrashHandler(){

        sOldUncaughtExceptionHandler = Thread.getDefaultUncaughtExceptionHandler();

        Thread.setDefaultUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {

            @Override
            public void uncaughtException(Thread t, Throwable e) {

                StringBuilder sb = new StringBuilder();
                Debug.MemoryInfo mi = new Debug.MemoryInfo();
                Debug.getMemoryInfo(mi);
                sb.append("Pss: ").append(mi.getTotalPss()).append("\n\r");
                sb.append("Native heap size: ").append(Debug.getNativeHeapSize()).append("\n\r");
                sb.append("Native heap alloced: ").append(Debug.getNativeHeapAllocatedSize()).append("\n\r");
                sb.append("Native head free: ").append(Debug.getNativeHeapFreeSize()).append("\n\r");
                sb.append("FATAL died, thread: ").append((t==null?"":t.getName())).append("\n\r")
                        .append(" reason: ").append(e==null?"":e.getMessage()).append("\n\r")
                        .append(" stack: ").append(Log.getStackTraceString(e));

                Log.e(TAG, sb.toString());

                writeString2File(getLogFilePath(), sb.toString());

                if(sOldUncaughtExceptionHandler != null){
                    sOldUncaughtExceptionHandler.uncaughtException(t, e);
                }
            }
        });

        return true;
    }

    private static final String getLogFilePath() {
        sDate.setTime(System.currentTimeMillis());
        return LOG_DIR + (LOG_DIR.endsWith(File.separator)?"":File.separator) + sFormat.format(sDate) + ".crash";
    }

    private static final boolean writeString2File(String targetPath,String content){
        Log.d(TAG,"write string: " + targetPath + " " + content);
        PrintWriter out = null;
        BufferedWriter bw = null;
        FileWriter fw = null;
        try {
            fw = new FileWriter(targetPath, true);
            bw = new BufferedWriter(fw);
            out = new PrintWriter(bw);
            out.print(content);
            out.flush();
            return true;
        }catch (Exception e) {
            Log.d(TAG,"write content failed: ");
            e.printStackTrace();
        }finally {
            safeClose(out);
            safeClose(bw);
            safeClose(fw);
        }

        return false;
    }

    public static final void safeClose(Closeable i){
        if(i != null) {
            try {
                i.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    /**
     * call from native
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
