package com.summer.demo.crashsdkdemo;

import android.app.Activity;
import android.os.Bundle;
import android.os.Process;
import android.util.Log;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.summer.crashsdk.CrashSDK;


public class MainActivity extends Activity implements View.OnClickListener{

    private static final String TAG = "MainActivity";

    private final static int ID_NPE = 1;
    private final static int ID_WPE = 2;
    private final static int ID_VPE = 3;
    private final static int ID_STACK_OVERFLOW = 4;
    private final static int ID_OOM = 5;
    private final static int ID_ABORT = 6;
    private final static int ID_ART_CRASH = 7;
    private final static int ID_LIBC_CRASH = 8;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        CrashSDK.init(getApplicationContext());

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        setContentView(root);

        TextView v = new TextView(this);
        v.setId(ID_NPE);
        v.setTextSize(28);
        v.setText("null pointer");
        v.setOnClickListener(this);
        root.addView(v);

        v = new TextView(this);
        v.setId(ID_WPE);
        v.setTextSize(28);
        v.setText("dangling pointer");
        v.setOnClickListener(this);
        root.addView(v);

        v = new TextView(this);
        v.setId(ID_VPE);
        v.setTextSize(28);
        v.setText("dangling virtual pointer");
        v.setOnClickListener(this);
        root.addView(v);

        v = new TextView(this);
        v.setId(ID_STACK_OVERFLOW);
        v.setTextSize(28);
        v.setText("stack overflow");
        v.setOnClickListener(this);
        root.addView(v);

        v = new TextView(this);
        v.setId(ID_OOM);
        v.setTextSize(28);
        v.setText("oom");
        v.setOnClickListener(this);
        root.addView(v);

        v = new TextView(this);
        v.setId(ID_ABORT);
        v.setTextSize(28);
        v.setText("abort");
        v.setOnClickListener(this);
        root.addView(v);

        v = new TextView(this);
        v.setId(ID_ART_CRASH);
        v.setTextSize(28);
        v.setText("invalidate jni");
        v.setOnClickListener(this);
        root.addView(v);

        v = new TextView(this);
        v.setId(ID_LIBC_CRASH);
        v.setTextSize(28);
        v.setText("libc crash");
        v.setOnClickListener(this);
        root.addView(v);

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
        }
    }
}
