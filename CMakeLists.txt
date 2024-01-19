cmake_minimum_required(VERSION 3.4.1)

project(rknn_yolov5_demo)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-pthread")

# skip 3rd-party lib dependencies
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--allow-shlib-undefined")

# install target and libraries
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install/rknn_yolov5_demo_${CMAKE_SYSTEM_NAME})

set(CMAKE_SKIP_INSTALL_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# rknn api
set(RKNN_API_PATH ${CMAKE_SOURCE_DIR}/../../runtime/RK3588/${CMAKE_SYSTEM_NAME}/librknn_api)
set(LIB_ARCH aarch64)
set(RKNN_RT_LIB ${CMAKE_SOURCE_DIR}/include/librknnrt.so)

#rga
set(RGA_PATH ${CMAKE_SOURCE_DIR}/include/3rdparty/rga/RK3588)
set(RGA_LIB ${RGA_PATH}/lib/Linux//${LIB_ARCH}/librga.so)


include_directories(${RKNN_API_PATH}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/3rdparty)
include_directories(${RGA_PATH}/include)

set(CMAKE_INSTALL_RPATH "lib")


# rknn_yolov5_demo
include_directories( ${CMAKE_SOURCE_DIR}/include)

option(CROSS_COMPILE "Enable CROSS_COMPILE" ON)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64")
  if(CROSS_COMPILE)
    message(STATUS "使能交叉编译")
    set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld")
    set(CMAKE_TOOLCHAIN_FILE "${PROJECT_SOURCE_DIR}/toolchains/aarch64-linux-gnu.toolchain.cmake")
    set(CMAKE_SYSROOT "/usr/lib/aarch64-linux-gnu;/usr/local/lib/aarch64-linux-gnu")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -target aarch64-linux-gnu -Os -fopenmp=libomp")
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
    include_directories("/usr/local/lib/aarch64-linux-gnu/include;/usr/local/lib/aarch64-linux-gnu")
    link_directories("/usr/local/lib/aarch64-linux-gnu/lib")
    set(CMAKE_MODULE_PATH "/usr/local/lib/aarch64-linux-gnu")
    set(CMAKE_PREFIX_PATH "/usr/local/lib/aarch64-linux-gnu")
  endif()
endif()

# opencv
find_package(OpenCV REQUIRED opencv_core opencv_imgcodecs opencv_imgproc opencv_videoio opencv_video opencv_highgui)

add_executable(rknn_yolov5_demo
        src/main.cc
        src/postprocess.cc
        src/preprocess.cc
        src/rkYolov5s.cc
        src/lock_queue.cpp
        src/bindCpu.cpp
)

target_link_libraries(rknn_yolov5_demo
  ${RKNN_RT_LIB}
  ${OpenCV_LIBS}
  ${RGA_LIB}
)


# install target and libraries
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install/rknn_yolov5_demo_${CMAKE_SYSTEM_NAME})
install(TARGETS rknn_yolov5_demo DESTINATION ./)
install(PROGRAMS ${RKNN_RT_LIB} DESTINATION lib)
install(PROGRAMS ${RGA_LIB} DESTINATION lib)
install(DIRECTORY model DESTINATION ./)
