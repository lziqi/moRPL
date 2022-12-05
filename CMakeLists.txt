cmake_minimum_required(VERSION 3.0)

project(moRPL)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++11") # -O2

set(CMAKE_BUILD_TYPE Debug)

# 设置rpath
set(CMAKE_INSTALL_RPATH "../lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# gdal
find_library(GDAL_LIB gdal HINTS /usr/lib)

# mpi
find_package(MPI REQUIRED)

# opencl
find_library(OPENCL_LIB OpenCL HINTS /usr/lib/x86_64-linux-gnu)

# tiff
find_library(TIFF_LIB tiff HINTS /home/lziqi/Code/software/tiff/lib)

include_directories(./)

aux_source_directory(./ SRC)

add_executable(main ${SRC})

# gdal mpi opencl tiff
include_directories(/usr/include/gdal ${MPI_INCLUDE_PATH} /usr/include/CL)
target_link_libraries(main ${GDAL_LIB} ${MPI_LIBRARIES} ${OPENCL_LIB} ${TIFF_LIB})

# 安装
install(TARGETS main RUNTIME DESTINATION bin)

# cmake -DCMAKE_INSTALL_PREFIX=./ ..
