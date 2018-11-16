# What it for

  Catch crashes in Java & Native(arm/x86), and dump crash information to a file located in sdcard, crash information includes call stack, system version, architecture, and registers state for native crash, etc.
  
# Structure of this project
  
  This is an Android project created via Android Studio, the module 'crashsdk' is an aar library used by the demo app;
  
  The c++ part in the crashsdk is not configured in gradle, so you need build it manually, go to jni directory and type 'ndk-build' to build the target native libs.

# Implementation

  1. For crash in Java, we use Thread#setDefaultUncaughtExceptionHandler to handle it; 
  
  2. For crash in Native, we need a signal handler to get a chance to handle signals such as SIGSEGV; In the handle, we unwind the call stack and dump infomation we interest. Since we are doing these in a signal handler, we should take signal safety into consideration. 

# Unwind Native Call Stack
	
  When gcc generates code that handles exceptions, it produces tables that describe how to unwind the stack. These tables are found in the .eh_frame section; Read https://refspecs.linuxfoundation.org/LSB_3.0.0/LSB-Embedded/LSB-Embedded/ehframechpt.html for details of .eh_frame format. The definition of Call Frame Instructions in FDE can be found in DWARF(Debugging With Attributed Record Formats) specification , you can find it here: http://dwarfstd.org

  The process of unwinding can be describe as below:
  1. Get IP(instruction pointer) where the program crashed(by parsing the third parameter of sa_handler callback);
  2. Find the target library in which the crash address located;
  3. Parse .eh_frame segment in the library and find the corresponding FDE, restore this call frame with instructions described in CIE&FDE;
  4. After restoring call frame we can find out the return address and CFA(call frame address) of previous call frame;
  5. With the return address and CFA we can unwind the previous call frame and repeat it until reach the root call frame ( (where the CFA&IP no more change);

  In step 1, we may get a wrong crash IP if the crash is caused by wrong instruction pointer. In this case we need try to unwind this frame by calling convention;

  Unfortunately, .eh_frame section not always exist, for ARM, a section .ARM.exidx exist and it is similar to .eh_frame. So, if unwind with eh_frame failed we can try .ARM.exidx; For more information of EXIDX , look here: http://infocenter.arm.com/help/topic/com.arm.doc.ihi0038b/IHI0038B_ehabi.pdf
