set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

include_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../configuration/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../configuration/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../xatmi/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../manager/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../domain/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../domain/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../service/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../service/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../queue/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../queue/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../file/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../transaction/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../gateway/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../gateway/unittest/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../tools/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cli/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../buffer/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/database/include>
)

link_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../administration/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../common/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../configuration/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../domain/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../buffer/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../queue/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../file/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../transaction/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../service/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../server/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../gateway/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../tools/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../buffer/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../xatmi/bin>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../cli/bin>
)

add_library(casual-administration-cli SHARED)

target_sources(casual-administration-cli
  PRIVATE
    source/cli.cpp
)


target_link_libraries(casual-administration-cli PRIVATE
  casual-common
  casual-cli
  casual-domain-admin-cli
  casual-domain-discovery-admin-cli
  casual-service-admin-cli
  casual-queue-admin-cli
  casual-file-admin-cli
  casual-transaction-admin-cli
  casual-gateway-admin-cli
  casual-tools-service-cli
  casual-buffer-cli
  casual-configuration-cli
)


add_executable(casual)

target_sources(casual
  PRIVATE
    source/main.cpp
)

target_link_libraries(casual PRIVATE
  casual-administration-cli
  casual-common
)


add_executable(casual-administration-cli-documentation)

target_sources(casual-administration-cli-documentation
  PRIVATE
    source/documentation/cli.cpp
)


target_link_libraries(casual-administration-cli-documentation PRIVATE
  casual-administration-cli
  casual-common
  gtest::gtest
)


add_library(casual-administration-unittest SHARED)

target_sources(casual-administration-unittest
  PRIVATE
    unittest/source/cli/command.cpp
    unittest/source/build/compile.cpp
)

target_link_libraries(casual-administration-unittest PRIVATE
  casual-common
  casual-unittest
)


add_executable(test-casual-administration)

target_sources(test-casual-administration
  PRIVATE
    unittest/source/cli/test_casual.cpp
    unittest/source/cli/test_call.cpp
    unittest/source/cli/test_domain.cpp
    unittest/source/cli/test_discovery.cpp
    unittest/source/cli/test_queue.cpp
    unittest/source/cli/configuration/test_normalize.cpp
    unittest/source/cli/configuration/test_set_operations.cpp
    unittest/source/cli/configuration/test_runtime.cpp
    unittest/source/cli/test_internal.cpp
    unittest/source/cli/test_transaction.cpp
    unittest/source/cli/test_gateway.cpp
    unittest/source/cli/test_service.cpp
    unittest/source/cli/test_pipe.cpp
    unittest/source/cli/test_file.cpp
    unittest/source/build/test_rm.cpp
    unittest/source/build/test_server.cpp
    unittest/source/build/test_executable.cpp
)

target_link_libraries(test-casual-administration PRIVATE
  casual-administration-unittest
  casual-common
  casual-unittest
  casual-domain-unittest
  casual-service-unittest
  casual-transaction
  casual-xatmi
  casual-gateway-unittest
  casual-configuration
  casual-queue-unittest
  casual-queue-api
  casual-file-api
  gtest::gtest
  gtest_main
  sqlite3
)


enable_testing()
include(GoogleTest)
gtest_discover_tests(test-casual-administration "" AUTO)

install(
    TARGETS
      casual
    DESTINATION
      bin
)

install(
    TARGETS
      casual-administration-cli
    DESTINATION
      lib
)

if(DEFINED ENV{INSTALL_ETC})
  install(
      FILES
        etc/bash_completion.d/casual
      DESTINATION
        /etc/bash_completion.d/
  )

  install(
      FILES
        etc/bash_completion.d/casual-log
      DESTINATION
        /etc/bash_completion.d/
  )
endif()

# Documentation
add_custom_target(documentation-adminstration
    #COMMAND echo "Generating documentation for buffer"
    COMMAND
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>/bin/casual-administration-cli-documentation 
      --root $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/documentation
    VERBATIM
    DEPENDS
      casual-administration-cli-documentation
)

