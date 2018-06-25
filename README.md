# What it for

  Catch crash in Java & Native(arm/x86), and dump crash information to a file locate in sdcard, crash information contains call stack, system version, architecture, and registers state for native crash.
  
# Structure of this project
  
  This is an Android project create by Android studio, the module 'crashsdk' is an aar lib and that is what we need implememt&optimize; 
  
  The c++ part of module 'crashsdk' is not associate with android studio, so it must be build manually, cd to jni directory and type command 'ndk-build'

# Implementation

  1. For crash in Java, use Thread#setDefaultUncaughtExceptionHandler to catch crash; 
  
  2. For crash in Native, we register a signal handler to handle signals such as SIGSEGV; In our handler, we need do:
    1) Get registers state while crash raise;
    2) Unwind call stack;
    3) Dump crash information;
    Since we are do these in a signal handler, we should take signal safety into consideration. 

# Unwind Native Call Stack
	
  When gcc generates code that handles exceptions, it produces tables that describe how to unwind the stack. These tables are found in the .eh_frame section; Read https://refspecs.linuxfoundation.org/LSB_3.0.0/LSB-Embedded/LSB-Embedded/ehframechpt.html for details of .eh_frame format. The definition of Call Frame Instructions in FDE can be found in DWARF(Debugging With Attributed Record Formats) specification , you can download it here: http://dwarfstd.org

  The process of unwinding can be describe as below:
  1. Get IP(instruction pointer) where program crashed(by parsing the third parameter of sa_handler);
  2. Find the target library in which the crash address located;
  3. Parse .eh_frame segment in the library and find exactly FDE in which the ip located, restore this call frame with CIE&FDE instructions;
  4. After restoring call frame we can get the return address and CFA(call frame address) of previous call frame;
  5. With the return address and CFA we unwind the previous call frame, and unwind recursivly until reach the root(CFA&IP no more change);

  In step 1, we may get a wrong crash IP if the crash is caused by wrong instruction pointer. In this case there are no library corresponded to the wrong IP, we need try to unwind this frame by calling convention;

  Unfortunately, .eh_frame section not always exist, For ARM, a section .ARM.exidx exist and it is similar to .eh_frame. So, if unwind with eh_frame failed we can try .ARM.exidx; For more information of EXIDX , look here: http://infocenter.arm.com/help/topic/com.arm.doc.ihi0038b/IHI0038B_ehabi.pdf
