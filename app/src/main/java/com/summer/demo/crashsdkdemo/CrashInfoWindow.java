package com.summer.demo.crashsdkdemo;

import android.content.Context;
import android.graphics.Color;
import android.os.Debug;
import android.os.Handler;
import android.os.Looper;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

/**
 * Created by summer on 26/06/2018.
 */

public class CrashInfoWindow extends FrameLayout {

    private static final String TAG = "CrashInfoWindow";

    private ScrollView mScrollView;
    private TextView mCrashInfo;

    private String mCrashFile;

    public CrashInfoWindow(Context context) {
        super(context);

        mScrollView = new ScrollView(context);
        mScrollView.setBackgroundColor(Color.TRANSPARENT);
        mCrashInfo = new TextView(context);
        mScrollView.addView(mCrashInfo, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        addView(mScrollView, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
    }

    public void showCrash(String crashFilePath){
        mCrashFile = crashFilePath;

        update();
    }

    private int mTaskSeq = 0;
    private void update(){

        ++mTaskSeq;

        mCrashInfo.setText("Loading...");


        new Thread() {
            final private int taskSeq = mTaskSeq;
            StringBuffer sb;

            @Override
            public void run() {

                sb = FileUtils.readSmallFileText(mCrashFile);
                if(sb != null){

                    if(mTaskSeq == taskSeq){
                        new Handler(Looper.getMainLooper()).post(new Runnable() {
                            @Override
                            public void run() {
                                if(mTaskSeq == taskSeq){
                                    mCrashInfo.setText(sb.toString());
                                }
                            }
                        });
                    }
                }
            }
        }.start();
    }

}
