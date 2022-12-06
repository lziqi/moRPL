cmake_minimum_required(VERSION 3.0)

project(moRPL)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++11") # -O2 -g

set(CMAKE_BUILD_TYPE Debug)# Debug Release

# 设置rpath
set(CMAKE_INSTALL_RPATH "../lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# spdlog
find_library(LOG_LIB spdlog PATHS ${PROJECT_SOURCE_DIR}/lib/spdlog/lib) #CMAKE_FIND_ROOT_PATH_BOTH

# tiff
find_library(TIFF_LIB tiff PATHS ${PROJECT_SOURCE_DIR}/lib/tiff/lib)

# gdal
find_library(GDAL_LIB gdal PATHS ${PROJECT_SOURCE_DIR}/lib/gdal/lib)

# mpi
find_package(MPI REQUIRED)

# opencl
find_library(OPENCL_LIB OpenCL HINTS /usr/lib/x86_64-linux-gnu)


include_directories(include)

aux_source_directory(src SRC)

add_executable(main main.cpp ${SRC})

# tiff spdlog gdal mpi opencl 
include_directories(${PROJECT_SOURCE_DIR}/lib/tiff/include ${PROJECT_SOURCE_DIR}/lib/spdlog/include ${PROJECT_SOURCE_DIR}/lib/gdal/include ${MPI_INCLUDE_PATH} /usr/include/CL )
target_link_libraries(main ${TIFF_LIB} ${LOG_LIB} ${GDAL_LIB} ${MPI_LIBRARIES} ${OPENCL_LIB})

# 安装
install(TARGETS main RUNTIME DESTINATION bin)

# cmake -DCMAKE_INSTALL_PREFIX=./ ..
# cmake --install . --prefix /xxx/xxx/xxx
