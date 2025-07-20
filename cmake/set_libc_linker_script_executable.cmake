if(CMAKE_CROSSCOMPILING)
    execute_process(COMMAND bash -c "echo '/* GNU ld script */\nOUTPUT_FORMAT(elf32-littlearm)\nGROUP ( ../../lib/libc.so.6 ../../lib/libc.a )' > /root/gcc-linaro_arm-linux-gnueabihf/arm-linux-gnueabihf/libc/usr/lib/libc.so")
endif()
