package com.summer.demo.crashsdkdemo;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.summer.crashsdk.CrashSDK;


public class MainActivity extends Activity {

    private static final String TAG = "MainActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        CrashSDK.init();

        LinearLayout root = new LinearLayout(this);
        setContentView(root);

        TextView demo = new TextView(this);
        demo.setTextSize(32);
        demo.setText("CrashSDK Demo");
        root.addView(demo);



        demo.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
//                String a = null;
//                if (a.equals("jjj")) {
//                    Log.d(TAG,"jjjj");
//                }

                CrashSDK.test();
            }
        });
    }
}
