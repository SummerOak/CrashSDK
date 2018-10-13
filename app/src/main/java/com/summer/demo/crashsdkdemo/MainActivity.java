package com.summer.demo.crashsdkdemo;

import android.app.Activity;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Process;
import android.util.Log;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.summer.crashsdk.CrashSDK;


public class MainActivity extends Activity implements View.OnClickListener, CrashRecordWindow.ICallback{

    private static final String TAG = "MainActivity";



    private final static int ID_NPE = 1;
    private final static int ID_WPE = 2;
    private final static int ID_VPE = 3;
    private final static int ID_STACK_OVERFLOW = 4;
    private final static int ID_OOM = 5;
    private final static int ID_ABORT = 6;
    private final static int ID_ART_CRASH = 7;
    private final static int ID_LIBC_CRASH = 8;


    private final static int ID_JAVA_CRASH_NPE = 1008;

    private final static int ID_SHOW_CRASH_LOG = 99999;

    private CrashRecordWindow mRecordWindow;
    private CrashInfoWindow mCrashInfoWindow;


    private final byte[] bbb = new byte[1<<23];

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        CrashSDK.init(getApplicationContext());

        bbb[0] = 0;

        FrameLayout view = new FrameLayout(this);
        setContentView(view);

        LinearLayout main = new LinearLayout(this);
        main.setOrientation(LinearLayout.VERTICAL);
        view.addView(main);

        mRecordWindow = new CrashRecordWindow(this, this);
        view.addView(mRecordWindow);
        mRecordWindow.setBackgroundColor(Color.WHITE);
        mRecordWindow.setVisibility(View.GONE);
        mCrashInfoWindow = new CrashInfoWindow(this);
        view.addView(mCrashInfoWindow);
        mCrashInfoWindow.setVisibility(View.GONE);
        mCrashInfoWindow.setBackgroundColor(Color.WHITE);

        TextView v = new TextView(this);
        v.setId(ID_NPE);
        v.setTextSize(28);
        v.setText("null pointer");
        v.setOnClickListener(this);
        main.addView(v);

        v = new TextView(this);
        v.setId(ID_WPE);
        v.setTextSize(28);
        v.setText("dangling pointer");
        v.setOnClickListener(this);
        main.addView(v);

        v = new TextView(this);
        v.setId(ID_VPE);
        v.setTextSize(28);
        v.setText("dangling virtual pointer");
        v.setOnClickListener(this);
        main.addView(v);

        v = new TextView(this);
        v.setId(ID_STACK_OVERFLOW);
        v.setTextSize(28);
        v.setText("stack overflow");
        v.setOnClickListener(this);
        main.addView(v);

        v = new TextView(this);
        v.setId(ID_OOM);
        v.setTextSize(28);
        v.setText("oom");
        v.setOnClickListener(this);
        main.addView(v);

        v = new TextView(this);
        v.setId(ID_ABORT);
        v.setTextSize(28);
        v.setText("abort");
        v.setOnClickListener(this);
        main.addView(v);

        v = new TextView(this);
        v.setId(ID_ART_CRASH);
        v.setTextSize(28);
        v.setText("invalidate jni");
        v.setOnClickListener(this);
        main.addView(v);

        v = new TextView(this);
        v.setId(ID_LIBC_CRASH);
        v.setTextSize(28);
        v.setText("libc crash");
        v.setOnClickListener(this);
        main.addView(v);

        v = new TextView(this);
        v.setId(ID_JAVA_CRASH_NPE);
        v.setTextSize(28);
        v.setTextColor(Color.RED);
        v.setText("java null pointer");
        v.setOnClickListener(this);
        main.addView(v);

        v = new TextView(this);
        v.setId(ID_SHOW_CRASH_LOG);
        v.setTextSize(28);
        v.setTextColor(Color.GRAY);
        v.setText("tombstones");
        v.setOnClickListener(this);
        main.addView(v);

        Log.d(TAG,"demo pid " + Process.myPid() + " tid " + Process.myTid());

    }

    @Override
    public void onClick(View v) {
        switch (v.getId()){
            case ID_NPE:
            case ID_WPE:
            case ID_VPE:
            case ID_STACK_OVERFLOW:
            case ID_OOM:
            case ID_ABORT:
            case ID_ART_CRASH:
            case ID_LIBC_CRASH:
                CrashSDK.doCrash(v.getId());
                break;

            case ID_JAVA_CRASH_NPE:{
                throw new NullPointerException("Crash test");
            }
            case ID_SHOW_CRASH_LOG:{
                mCrashInfoWindow.setVisibility(View.GONE);
                mRecordWindow.setVisibility(View.VISIBLE);
                mRecordWindow.initData();
                break;
            }
        }
    }

    @Override
    public void onBackPressed() {
        if(mCrashInfoWindow.getVisibility() == View.VISIBLE){
            mCrashInfoWindow.setVisibility(View.GONE);
            return;
        }

        if(mRecordWindow.getVisibility() == View.VISIBLE){
            mRecordWindow.setVisibility(View.GONE);
            return;
        }

        super.onBackPressed();
    }

    @Override
    public void showCrash(String path) {
        mCrashInfoWindow.showCrash(path);
        mCrashInfoWindow.setVisibility(View.VISIBLE);
    }
}
