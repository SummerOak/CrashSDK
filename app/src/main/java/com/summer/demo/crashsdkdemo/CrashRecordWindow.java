package com.summer.demo.crashsdkdemo;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.BaseAdapter;
import android.widget.FrameLayout;
import android.widget.ListView;
import android.widget.TextView;

import com.summer.crashsdk.CrashSDK;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by summer on 25/06/2018.
 */

public class CrashRecordWindow extends FrameLayout implements View.OnClickListener{

    private static final String TAG = "CrashRecordWindow";

    private ListView mListView;
    private MyAdapter mAdapter;
    private List<String> mData = new ArrayList<>();
    private ICallback mCallback;

    public CrashRecordWindow(Context context, ICallback cb) {
        super(context);

        mListView = new ListView(context);
        addView(mListView, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        mAdapter = new MyAdapter();
        mListView.setAdapter(mAdapter);

        mCallback = cb;

        initData();
    }

    protected TextView createItemView(int position) {
        TextView v = new TextView(getContext());
        v.setGravity(Gravity.CENTER_VERTICAL);
        v.setOnClickListener(CrashRecordWindow.this);
        v.setTextSize(23);
        v.setLayoutParams(new AbsListView.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,  ViewGroup.LayoutParams.WRAP_CONTENT));

        return v;
    }

    private int mTaskSeq = 0;
    public void initData(){

        ++mTaskSeq;


        new Thread() {
            final private int taskSeq = mTaskSeq;
            final List<String> ls = new ArrayList<>();

            @Override
            public void run() {

                String logDir = CrashSDK.getTombstonesDirectory();
                File fLogDir = new File(logDir);
                if(fLogDir.isDirectory()){
                    String[] logs = fLogDir.list();

                    if(logs != null){
                        for(int i=logs.length-1; i>=0;--i){
                            ls.add(logs[i]);
                        }
                    }
                }


                if(mTaskSeq == taskSeq){
                    new Handler(Looper.getMainLooper()).post(new Runnable() {
                        @Override
                        public void run() {
                            if(mTaskSeq == taskSeq){
                                mData.clear();
                                mData.addAll(ls);
                                mAdapter.notifyDataSetChanged();
                            }

                        }
                    });
                }

            }
        }.start();
    }

    @Override
    public void onClick(View v) {
        if(v.getTag() instanceof String){
            String name = (String)v.getTag();
            String directory = CrashSDK.getTombstonesDirectory();
            if(directory == null || name == null){
                return;
            }

            String fullPath = directory;
            if(!directory.endsWith(File.separator)){
                fullPath += File.separator;
            }
            fullPath += name;

            mCallback.showCrash(fullPath);
        }
    }

    private class MyAdapter extends BaseAdapter{

        @Override
        public int getCount() {
            return mData.size();
        }

        @Override
        public String getItem(int position) {
            return mData.get(position);
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            if(convertView == null){
                convertView = createItemView(position);
            }

            TextView tv = (TextView)convertView;
            tv.setText(getItem(position));
            tv.setTag(getItem(position));
            return convertView;
        }
    }

    interface ICallback{
        void showCrash(String path);
    }


}
