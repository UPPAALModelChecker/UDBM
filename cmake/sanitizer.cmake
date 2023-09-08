# Various sanitizers (runtime checks) for debugging
option(SSP "Stack Smashing Protector (GCC/Clang/ApplClang on Unix)" OFF
        )# Available on Windows too
option(UBSAN "Undefined Behavior Sanitizer (GCC/Clang/AppleClang on Unix)" OFF)
option(ASAN "Address Sanitizer (GCC/Clang/AppleClang on Unix, MSVC on Windows)"
        OFF)
option(TSAN "Thread Sanitizer (GCC/Clang/AppleClang on Unix)" OFF)
option(RTC_C "Runtime Checks for Conversions (MSVC on Windows)" OFF)
option(RTC_S "Runtime Checks for Stack (MSVC on Windows)" OFF)
option(RTC_U "Runtime Checks for Uninitialized (MSVC on Windows)" OFF)

if(SSP)
    add_compile_options(-fstack-protector)
    add_link_options(-fstack-protector)
    message(STATUS "Enable Stack Smashing Protector")
endif(SSP)

if(ASAN
        OR UBSAN
        OR TSAN)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

    else()
        add_compile_options(-fno-omit-frame-pointer)
        add_link_options(-fno-omit-frame-pointer)
    endif()
endif()

if(UBSAN)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_compile_options(/fsanitize=undefined)
        message(
                STATUS
                "Please see if MSVC supports undefined behavior sanitizer: https://learn.microsoft.com/en-us/cpp/sanitizers/asan"
        )
    else()
        add_compile_options(-fsanitize=undefined)
        add_link_options(-fsanitize=undefined)
    endif()
    message(STATUS "Enabled Undefined Behavior Sanitizer")
endif(UBSAN)

if(ASAN)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_compile_options(/fsanitize=address)
    else()
        add_compile_options(-fsanitize=address)
        add_link_options(-fsanitize=address)
    endif()
    message(STATUS "Enabled Address Sanitizer")
endif(ASAN)

if(TSAN)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_compile_options(/fsanitize=thread)
        message(
                STATUS
                "Please see if MSVC supports thread sanitizer: https://learn.microsoft.com/en-us/cpp/sanitizers/asan"
        )
    else()
        add_compile_options(-fsanitize=thread)
        add_link_options(-fsanitize=thread)
    endif()
    message(STATUS "Enabled Thread Sanitizer")
endif(TSAN)

if(RTC_C)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_compile_options(/RTCc)
        add_compile_definitions(_ALLOW_RTCc_IN_STL)
        message(STATUS "Enabled Runtime Check Conversions")
    else()
        message(
                WARNING
                "Runtime Check Conversions is not enabled for ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif(RTC_C)

if(RTC_S)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_compile_options(/RTCs)
        message(STATUS "Enabled Runtime Check Stack")
    else()
        message(
                WARNING "Runtime Check Stack is not enabled for ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif(RTC_S)

if(RTC_U)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_compile_options(/RTCu)
        message(STATUS "Enabled Runtime Check Uninitialized")
    else()
        message(
                WARNING
                "Runtime Check Uninitialized is not enabled for ${CMAKE_CXX_COMPILER_ID}"
        )
    endif()
endif(RTC_U)
