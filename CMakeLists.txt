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
    src/skill.cpp
    src/video.cpp
    src/autoplay.cpp
    src/settings.cpp
    src/security.cpp
)

target_include_directories(arcinf PRIVATE src)

# dl = dlopen/dlsym
target_link_libraries(arcinf PRIVATE dl)

set(RELEASE_CXX_FLAGS
    -O2
    -fno-ident
    -fno-exceptions
    -fno-rtti
    -fno-unwind-tables
    -ffunction-sections
    -fdata-sections
    -fno-asynchronous-unwind-tables # release
)
set(RELEASE_LINKER_FLAGS
    -Wl,-s
    -Wl,--gc-sections
    -Wl,--exclude-libs,ALL
    -Wl,--build-id=sha1
    -Wl,-z,relro
    -Wl,-z,now
)

target_compile_options(arcinf PRIVATE "$<$<CONFIG:Release>:${RELEASE_CXX_FLAGS}>")
target_link_options(arcinf PRIVATE "$<$<CONFIG:Release>:${RELEASE_LINKER_FLAGS}>")

# cmake --preset android
# cmake --build --preset android
#
# cmake --workflow --preset android
