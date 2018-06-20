# What it for

  Capture crash in Java & Native(arm/x86), and dump crash information to a file locate in sdcard, crash information contains call stack, system version, architecture, and registers state for native crash.
  
# Structure of this project
  
  This is an Android project create by Android studio, the module 'crashsdk' is an aar lib and that is what we need implememt&optimize; 
  
  The c++ part of module 'crashsdk' is not associate with android studio, so it must be build manually, cd to jni directory and type command 'ndk-build'

# Implementation

  1. For crash in Java, use Thread#setDefaultUncaughtExceptionHandler to catch crash; 
  
  2. For crash in Native, we register a signal handler to handle signals such as SIGSEGV; In our handler, we need do:
    1) parse registers state when crash occur,
    2) unwind call stack;
    3) write crash information to file;
    Since we are do these in a signal handler, we should take signal safety into consideration.
