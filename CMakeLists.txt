cmake_minimum_required(VERSION 3.25)
project(arcinf CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

add_library(arcinf SHARED
    src/main.cpp
    src/settings.cpp
    src/skill.cpp
    src/video.cpp
    src/autoplay.cpp
    src/security.cpp
)

target_include_directories(arcinf PRIVATE src)

# dl = dlopen / dlsym / dl_iterate_phdr
target_link_libraries(arcinf PRIVATE dl)

set(ARCINF_CXX_FLAGS
    -O2
    -fno-ident
    -fno-exceptions
    -fno-rtti
    -fno-unwind-tables
    -fno-asynchronous-unwind-tables
    -ffunction-sections
    -fdata-sections
)
set(ARCINF_LINKER_FLAGS
    -Wl,-s
    -Wl,--gc-sections
    -Wl,--exclude-libs,ALL
    -Wl,--build-id=sha1
    -Wl,-z,relro
    -Wl,-z,now
    # 适配 Android 15+ 的 16KB 页设备
    -Wl,-z,max-page-size=16384
    -Wl,-z,common-page-size=16384
)

target_compile_options(arcinf PRIVATE "$<$<CONFIG:Release>:${ARCINF_CXX_FLAGS}>")
target_link_options(arcinf PRIVATE "$<$<CONFIG:Release>:${ARCINF_LINKER_FLAGS}>")

# cmake --preset android
# cmake --build --preset android
# OR:
# cmake --workflow --preset android
