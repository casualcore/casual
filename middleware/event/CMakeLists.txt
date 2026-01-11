set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

include_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../manager/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../configuration/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../service/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../service/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../domain/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../domain/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/database/include>
)

link_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../event/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../manager/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../xatmi/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../domain/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../service/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../configuration/bin>
)

add_library(casual-event-api SHARED)

target_sources(casual-event-api
  PRIVATE
    source/dispatch.cpp
)

target_link_libraries(casual-event-api PRIVATE
  casual-common
)


add_executable(casual-event-service-monitor)

target_sources(casual-event-service-monitor
  PRIVATE
    source/service/monitor/database.cpp
)

target_link_libraries(casual-event-service-monitor PRIVATE
  casual-common
  sqlite3
)

add_executable(casual-event-service-log)

target_sources(casual-event-service-log
  PRIVATE
    source/service/log/main.cpp
)


target_link_libraries(casual-event-service-log PRIVATE
  casual-common
)


add_executable(casual-event-service-monitor-server)

target_sources(casual-event-service-monitor-server
  PRIVATE
    source/service/monitor/server.cpp
)

target_link_libraries(casual-event-service-monitor-server PRIVATE
  casual-common
  casual-manager
  sqlite3
)

add_executable(test-casual-event)

target_sources(test-casual-event
  PRIVATE
    unittest/source/test_dispatch.cpp
    unittest/source/service/test_log.cpp
)

target_link_libraries(test-casual-event PRIVATE
  casual-event-api
  casual-common
  casual-unittest
  casual-domain-api
  casual-domain-utility
  casual-domain-unittest
  casual-service-unittest
  gtest::gtest
  gtest_main
)


enable_testing()
include(GoogleTest)
gtest_discover_tests(test-casual-event "" AUTO)
install(
    TARGETS
      casual-event-api
    DESTINATION
      lib
)

install(
    TARGETS
      casual-event-service-monitor
      casual-event-service-log
      casual-event-service-monitor-server
    DESTINATION
      bin
)

install(
    FILES
      include/casual/event/model.h
      include/casual/event/dispatch.h
    DESTINATION
      include/casual/event
)

