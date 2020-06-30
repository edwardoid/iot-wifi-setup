cmake_minimum_required(VERSION 3.5)
project(iotwifi VERSION 1.0 DESCRIPTION "A library to establish WiFI conenction using NetworkManager")
set(CMAKE_CXX_STANDARD 11)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR})

if (TARGET_DEVICE)
  set(CMAKE_BUILD_TYPE Release)
  add_definitions(-DTARGET_DEVICE)
  include_directories("$ENV{SYSROOT_DIR}/usr/include")
endif()

find_package(PkgConfig)

find_package(Threads)

include_directories("${CMAKE_CURRENT_LIST_DIR}/3dp/s2s")
PKG_CHECK_MODULES(GLIB glib-2.0)
include_directories(${GLIB_INCLUDE_DIRS})
include_directories(${NETWORKMANAGER_INCLUDE_DIRS})
link_directories(${GLIB_LIBRARY_DIRS})
add_definitions(${GLIB_CFLAGS_OTHER})
add_definitions($(NETWORKMANAGER_CFLAGS))
find_package(NetworkManager REQUIRED)

file(GLOB SOURCES "${CMAKE_CURRENT_LIST_DIR}/include/*.cpp")
file(GLOB INCLUDES "${CMAKE_CURRENT_LIST_DIR}/include/*.h")

set(IOTWIFI_LIB_SHARED ${PROJECT_NAME}_shared)
set(IOTWIFI_LIB_STATIC ${PROJECT_NAME}_static)

if (TEST)
   file(GLOB SOURCES "tests/*.cpp")
   add_executable(${PROJECT_NAME} ${SOURCES})
   target_link_libraries(${PROJECT_NAME} ${NETWORKMANAGER_LIBRARIES})
   target_link_libraries (${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
else()
   add_library(${PROJECT_NAME} OBJECT ${SOURCES})
   set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE 1)
   add_library(${PROJECT_NAME}_shared SHARED $<TARGET_OBJECTS:${PROJECT_NAME}>)
   add_library(${PROJECT_NAME}_static STATIC $<TARGET_OBJECTS:${PROJECT_NAME}>)
   set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION 1)
   set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER include/wifi_setup.h)
   set_target_properties(${PROJECT_NAME}_shared PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
   set_target_properties(${PROJECT_NAME}_static PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
   install(FILES "${CMAKE_CURRENT_LIST_DIR}/include/wifi_setup.h" DESTINATION include)
   install(FILES "${CMAKE_CURRENT_LIST_DIR}/3dp/s2s/s2s.h" DESTINATION include)
   install(FILES "${CMAKE_CURRENT_LIST_DIR}/3dp/s2s/s2s_property.h" DESTINATION include)
   install(TARGETS ${PROJECT_NAME}_static DESTINATION lib)
   install(TARGETS ${PROJECT_NAME}_shared DESTINATION lib)
endif()

#export(TARGETS ${PROJECT_NAME}_shared ${PROJECT_NAME}_static FILE iotwifi-exports.cmake)
include_directories(${PROJECT_NAME} ${NETWORKMANAGER_INCLUDE_DIRS})


