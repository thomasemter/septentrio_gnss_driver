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

find_package(ament_cmake QUIET)
find_package(catkin QUIET)

if(catkin_FOUND)
  ## ROS 1 -----------------------------------------------------------------------------------------------  
  set(ENV{ROS_VERSION} "1")

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
  target_compile_definitions(${PROJECT_NAME}_node PUBLIC ROS1)
 
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
elseif(ament_cmake_FOUND)
  ## ROS 2 -----------------------------------------------------------------------------------------------
  set(ENV{ROS_VERSION} "2")

  unset(CATKIN_INSTALL_INTO_PREFIX_ROOT)
  unset(CATKIN_SYMLINK_INSTALL)

  # Dependencies
  find_package(ament_cmake REQUIRED)
  find_package(rclcpp REQUIRED)
  find_package(rclcpp_components REQUIRED)
  find_package(rosidl_default_generators REQUIRED)
  find_package(diagnostic_msgs REQUIRED)
  find_package(nmea_msgs REQUIRED)
  find_package(sensor_msgs REQUIRED)
  find_package(geometry_msgs REQUIRED)
  find_package(gps_msgs REQUIRED)
  find_package(nav_msgs REQUIRED)
  find_package(tf2 REQUIRED)
  find_package(tf2_eigen REQUIRED)
  find_package(tf2_geometry_msgs REQUIRED)
  find_package(tf2_ros REQUIRED)
  find_package(gtest_vendor REQUIRED)
  find_package(ament_cmake_ros REQUIRED)
  
  ## Declare  messages
  rosidl_generate_interfaces(${PROJECT_NAME}
    "msg/AIMPlusStatus.msg"
    "msg/BaseVectorCart.msg"
    "msg/BaseVectorGeod.msg"
    "${CMAKE_CURRENT_SOURCE_DIR}/msg_ros2:msg/BlockHeader.msg"
    "msg/GALAuthStatus.msg"
    "msg/RFBand.msg"
    "msg/RFStatus.msg"
    "msg/MeasEpoch.msg"
    "msg/MeasEpochChannelType1.msg"
    "msg/MeasEpochChannelType2.msg"
    "msg/PVTCartesian.msg"
    "msg/PVTGeodetic.msg"
    "msg/PosCovCartesian.msg"
    "msg/PosCovGeodetic.msg"
    "msg/ReceiverTime.msg"
    "msg/VelCovCartesian.msg"
    "msg/VelCovGeodetic.msg"
    "msg/AttEuler.msg"
    "msg/AttCovEuler.msg"
    "msg/INSNavCart.msg"
    "msg/INSNavGeod.msg"
    "msg/IMUSetup.msg"
    "msg/VectorInfoCart.msg"
    "msg/VectorInfoGeod.msg"
    "msg/VelSensorSetup.msg"
    "msg/ExtSensorMeas.msg"
    DEPENDENCIES std_msgs
  )

  set(library_name septentrio_gnss_driver_core)
  set(executable_name septentrio_gnss_driver_node)

  set(dependencies
    rclcpp
    rclcpp_components
    diagnostic_msgs
    nmea_msgs
    sensor_msgs
    geometry_msgs
    gps_msgs
    nav_msgs
    tf2
    tf2_eigen
    tf2_geometry_msgs
    tf2_ros
  )

  ## shared library
  add_library(${library_name} SHARED
    src/septentrio_gnss_driver/communication/communication_core.cpp
    src/septentrio_gnss_driver/communication/message_handler.cpp 
    src/septentrio_gnss_driver/communication/telegram_handler.cpp
    src/septentrio_gnss_driver/crc/crc.cpp
    src/septentrio_gnss_driver/node/main.cpp
    src/septentrio_gnss_driver/node/rosaic_node.cpp
    src/septentrio_gnss_driver/parsers/nmea_parsers/gpgga.cpp 
    src/septentrio_gnss_driver/parsers/nmea_parsers/gprmc.cpp 
    src/septentrio_gnss_driver/parsers/nmea_parsers/gpgsa.cpp 
    src/septentrio_gnss_driver/parsers/nmea_parsers/gpgsv.cpp
    src/septentrio_gnss_driver/parsers/parsing_utilities.cpp 
    src/septentrio_gnss_driver/parsers/string_utilities.cpp 
  )
  target_compile_definitions(${library_name} PUBLIC ROS2)
  target_include_directories(${library_name} PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"  
    "$<INSTALL_INTERFACE:include>"
  )
  target_link_libraries(${library_name}
    boost_regex # ordering is important for regex https://stackoverflow.com/questions/17588440/cannot-link-boost-regex
    ${Boost_LIBRARIES}
    pcap
    ${GeographicLib_LIBRARIES}
    ${diagnostic_msgs_TARGETS}
    ${geometry_msgs_TARGETS}
    ${gps_msgs_TARGETS}
    ${nav_msgs_TARGETS}
    ${nmea_msgs_TARGETS}
    ${sensor_msgs_TARGETS}
    rclcpp::rclcpp
    rclcpp_components::component
    rclcpp_components::component_manager
    sensor_msgs::sensor_msgs_library
    tf2::tf2
    tf2_eigen::tf2_eigen
    tf2_geometry_msgs::tf2_geometry_msgs
    tf2_ros::static_transform_broadcaster_node
    tf2_ros::tf2_ros
  )

  if(${rosidl_cmake_VERSION} VERSION_LESS 2.5.0)
    rosidl_target_interfaces(${library_name}
    ${PROJECT_NAME} "rosidl_typesupport_cpp")
  else()
    rosidl_get_typesupport_target(
    cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")
    target_link_libraries(${library_name} "${cpp_typesupport_target}")
    target_compile_definitions(${library_name} PUBLIC ROS2_VER_N250)
    target_compile_definitions(${library_name} INTERFACE ROS2_VER_N250)
  endif()

  ## executable
  add_executable(${executable_name}
    src/septentrio_gnss_driver/node/main.cpp
  )
  target_link_libraries(${executable_name}
    ${library_name}
  )

  rclcpp_components_register_nodes(${library_name} "rosaic_node::ROSaicNode")

  # Testing
  find_package(ament_cmake_gtest REQUIRED)
  add_subdirectory(test)

  # Export Targets
  ament_export_targets(${library_name} HAS_LIBRARY_TARGET)
  ament_export_dependencies(${dependencies} rosidl_default_runtime)

  # Installation
  install(TARGETS
    ${library_name} EXPORT ${library_name}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
  )

  install(TARGETS ${executable_name} EXPORT ${executable_name}
    DESTINATION lib/${PROJECT_NAME}
  )

  install(DIRECTORY include/
    DESTINATION include/
  )

  install(DIRECTORY launch
    DESTINATION share/${PROJECT_NAME}
  )
  install(DIRECTORY config
    DESTINATION share/${PROJECT_NAME}
  )

  ament_package()
  ##-------------------------------------------------------------------------
else()
  message(FATAL_ERROR "Build failed because neither catkin nor ament found.")
endif()

