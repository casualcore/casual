set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

include_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cli/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../domain/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../service/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../transaction/include>
)

link_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../configuration/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
)

add_library(casual-configuration SHARED)

target_sources(casual-configuration
  PRIVATE
    source/common.cpp
    source/user.cpp
    source/model.cpp
    source/system.cpp
    source/group.cpp
    source/user/environment.cpp
    source/model/transform.cpp
    source/model/load.cpp
    source/model/validate.cpp
    source/build/model.cpp
    source/build/model/load.cpp
)

target_link_libraries(casual-configuration PRIVATE
  casual-common
)


add_library(casual-configuration-example STATIC)

target_sources(casual-configuration-example
  PRIVATE
    source/example/model.cpp
    source/example/build/model.cpp
)


add_executable(test-casual-configuration)

target_sources(test-casual-configuration
  PRIVATE
    unittest/source/test_model.cpp
    unittest/source/test_environment.cpp
    unittest/source/test_group.cpp
    unittest/source/test_build.cpp
    unittest/source/user/test_validate.cpp
    unittest/source/user/test_backward.cpp
    unittest/source/model/test_validate.cpp
    unittest/source/model/test_system.cpp
)


target_link_libraries(test-casual-configuration PRIVATE
  casual-configuration-example
  casual-configuration
  casual-common
  casual-unittest
  gtest::gtest
  gtest_main
)


enable_testing()
include(GoogleTest)
gtest_add_tests(test-casual-configuration "" AUTO)

add_executable(casual-configuration-samples)

target_sources(casual-configuration-samples
  PRIVATE
    source/documentation/main.cpp
)

target_link_libraries(casual-configuration-samples PRIVATE
  casual-configuration-example
  casual-configuration
  casual-common
)


add_library(casual-configuration-cli SHARED)

target_sources(casual-configuration-cli
  PRIVATE
    source/admin/cli.cpp
)

target_link_libraries(casual-configuration-cli PRIVATE
  casual-configuration
  casual-common
  casual-service
  casual-transaction
)

install(
  TARGETS
    casual-configuration
  DESTINATION
    lib
)

install(
  FILES
    resources.yaml
  DESTINATION
    configuration/example/
)

if(DEFINED ENV{INSTALL_ETC})
  install(
    FILES
      resources.yaml
    DESTINATION
      /etc/casual
  )
endif()

install(
  TARGETS
    casual-configuration-cli
  DESTINATION
    lib
)

# Documentation
add_custom_target(documentation-configuration
  #COMMAND echo "Generating documentation for buffer"
  COMMAND
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>/bin/casual-configuration-samples 
    --root $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/documentation
  VERBATIM
  DEPENDS
    casual-configuration-samples
)
