#CMake minimum version
cmake_minimum_required(VERSION 2.8)

set(project_name CameraCalibration)
#--------------------------------------------
project( ${project_name} )

#Find libraries
#--------------------------------------------
find_package( OpenCV REQUIRED )


#Include directories
#--------------------------------------------
include_directories(${OpenCV_INCLUDE_DIRS})


#Link directories
#--------------------------------------------


#Files to be compiled
#--------------------------------------------
add_executable( ${project_name} main.cpp calibration.cpp)


#Link libraries
#--------------------------------------------
target_link_libraries( ${project_name} ${OpenCV_LIBS})
