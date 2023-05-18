cmake_minimum_required(VERSION 3.25)
project(WalkingDeadSurvival)

set(CMAKE_CXX_STANDARD 14)

# 設定 WinBGIm 函式庫路徑
include_directories(
        C:/MinGW/include
        C:/MinGW/mingw32/include
        C:/MinGW/lib/gcc/mingw32/6.3.0/include
        C:/MinGW/lib/gcc/mingw32/6.3.0/include/c++
)

link_directories(
        C:/MinGW/lib
        C:/MinGW/mingw32/lib
)

add_executable(WalkingDeadSurvival
        source/survival.cpp
        )

# 鏈結 WinBGIm 函式庫
target_link_libraries(WalkingDeadSurvival
        -m32
        -lgdi32
        -lbgi
        -luser32
        -static-libgcc
        -g3
        )
