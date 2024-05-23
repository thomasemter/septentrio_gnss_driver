cmake_minimum_required(VERSION 3.5)
project(septentrio_gnss_driver)

## Compile as C++17
add_compile_options(-std=c++17)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
message(STATUS "Setting build type to Release as none was specified.")
set(CMAKE_BUILD_TYPE "Release" CACHE
    STRING "Choose the type of build." FORCE)
# Set the possible values of build type for cmake-gui
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
  "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set(WARNFLAGS "-Wall -Wextra -Wpedantic -Wcast-align -Wnon-virtual-dtor -Woverloaded-virtual -Wcast-qual -Wctor-dtor-privacy")

## System dependencies are found with CMake's conventions
find_package(Boost REQUIRED)
LIST(APPEND CMAKE_MODULE_PATH "/usr/share/cmake/geographiclib")
find_package(GeographicLib REQUIRED)

## For PCAP file handling
find_library(libpcap_LIBRARIES pcap)
if ("${libpcap_LIBRARIES}" STREQUAL "pcap-NOTFOUND")
    set(libpcap_FOUND FALSE)
else ()
    set(libpcap_FOUND TRUE)
endif ()

## ROS 1 -----------------------------------------------------------------------------------------------  
add_compile_definitions(ROS1)

## Dependencies
find_package(catkin REQUIRED COMPONENTS
roscpp
nmea_msgs
sensor_msgs
geometry_msgs
nav_msgs
diagnostic_msgs
gps_common
message_generation
tf2
tf2_eigen
tf2_geometry_msgs
tf2_ros
)

## Declare messages
add_message_files(
  FILES
  AIMPlusStatus.msg
  BaseVectorCart.msg
  BaseVectorGeod.msg
  BlockHeader.msg
  GALAuthStatus.msg
  RFBand.msg
  RFStatus.msg
  MeasEpoch.msg
  MeasEpochChannelType1.msg
  MeasEpochChannelType2.msg
  PVTCartesian.msg
  PVTGeodetic.msg
  PosCovCartesian.msg
  PosCovGeodetic.msg
  ReceiverTime.msg
  VelCovCartesian.msg
  VelCovGeodetic.msg
  AttEuler.msg
  AttCovEuler.msg
  INSNavCart.msg
  INSNavGeod.msg
  IMUSetup.msg
  VectorInfoCart.msg
  VectorInfoGeod.msg
  VelSensorSetup.msg
  ExtSensorMeas.msg
)
## Generate messages
generate_messages(
  DEPENDENCIES
  std_msgs
  sensor_msgs
  diagnostic_msgs
  gps_common
)

# Catkin package
catkin_package(
  INCLUDE_DIRS include
  LIBRARIES 
  CATKIN_DEPENDS roscpp message_runtime
  DEPENDS Boost
)

# Build
include_directories(
  include
  ${catkin_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${GeographicLib_INCLUDE_DIRS}
)
add_executable(${PROJECT_NAME}_node
  src/septentrio_gnss_driver/communication/communication_core.cpp
  src/septentrio_gnss_driver/communication/message_handler.cpp 
  src/septentrio_gnss_driver/communication/telegram_handler.cpp
  src/septentrio_gnss_driver/crc/crc.cpp
  src/septentrio_gnss_driver/node/rosaic_node_ros1.cpp
  src/septentrio_gnss_driver/parsers/nmea_parsers/gpgga.cpp 
  src/septentrio_gnss_driver/parsers/nmea_parsers/gprmc.cpp 
  src/septentrio_gnss_driver/parsers/nmea_parsers/gpgsa.cpp 
  src/septentrio_gnss_driver/parsers/nmea_parsers/gpgsv.cpp
  src/septentrio_gnss_driver/parsers/parsing_utilities.cpp 
  src/septentrio_gnss_driver/parsers/string_utilities.cpp  
)
add_dependencies(${PROJECT_NAME}_node ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})
target_link_libraries(${PROJECT_NAME}_node 
  ${catkin_LIBRARIES}
  boost_regex # ordering is important for regex https://stackoverflow.com/questions/17588440/cannot-link-boost-regex
  ${Boost_LIBRARIES} 
  ${libpcap_LIBRARIES}
  ${GeographicLib_LIBRARIES}
)

# Installation
install(TARGETS ${PROJECT_NAME}_node
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)
install(DIRECTORY include/${PROJECT_NAME}/
  DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
)
install(DIRECTORY config launch DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})
#-------------------------------------------------------------------------------------------------------