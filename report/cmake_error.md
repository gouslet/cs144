# CMake Error: The following variables are used in this project, but they are set to NOTFOUND——LIBPCAP
- 问题
    ```
    $> cmake -DCMAKE_BUILD_TYPE=Debug ..
    --   NOTE: You can choose a build type by calling cmake with one of:
    --     -DCMAKE_BUILD_TYPE=Release   -- full optimizations
    --     -DCMAKE_BUILD_TYPE=Debug     -- better debugging experience in gdb
    --     -DCMAKE_BUILD_TYPE=RelASan   -- full optimizations plus address and undefined-behavior sanitizers
    --     -DCMAKE_BUILD_TYPE=DebugASan -- debug plus sanitizers
    -- Could NOT find Doxygen (missing: DOXYGEN_EXECUTABLE) 
    -- Configuring done
    CMake Error: The following variables are used in this project, but they are set to NOTFOUND.
    Please set them or make sure they are set and tested correctly in the CMake files:
    LIBPCAP
        linked by target "tcp_parser" in directory /root/cs144/sponge/tests
        linked by target "tcp_parser" in directory /root/cs144/sponge/tests

    -- Generating done
    CMake Generate step failed.  Build files cannot be regenerated correctly.
    ```
- 