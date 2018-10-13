package com.summer.demo.crashsdkdemo;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;

/**
 * Created by summer on 26/06/2018.
 */

public class FileUtils {

    public static final StringBuffer readSmallFileText(String filePath){
        StringBuffer sb = new StringBuffer();
        File file = new File(filePath);
        BufferedReader reader = null;
        FileReader fr = null;
        try {
            fr = new FileReader(file);
            reader = new BufferedReader(fr);
            String buffer = null;
            while ((buffer = reader.readLine()) != null) {
                sb.append(buffer).append("\n\r");
            }
        } catch (IOException e) {
            ExceptionHandler.handleException(e);
        } finally {
            IOUtils.safeClose(fr);
            IOUtils.safeClose(reader);
        }

        return sb;
    }

}
