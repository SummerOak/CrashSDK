package com.summer.demo.crashsdkdemo;

import java.io.Closeable;
import java.io.IOException;

/**
 * Created by summer on 26/06/2018.
 */

public class IOUtils {

    public static final void safeClose(Closeable io){
        if(io != null){
            try {
                io.close();
            } catch (IOException e) {
                ExceptionHandler.handleException(e);
            }
        }
    }

}
