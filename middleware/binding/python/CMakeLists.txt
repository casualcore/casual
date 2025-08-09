set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

install(
    FILES
      python-example-server.py
    DESTINATION
      example/bin
)

install(
    FILES
      slack-server.py
    DESTINATION
      example/bin
)

install(
    FILES
      casual/__init__.py
    DESTINATION
      python/casual/
)

install(
    FILES
      casual/server/__init__.py
      casual/server/api.py
      casual/server/buffer.py
      casual/server/exception.py
      casual/server/log.py
    DESTINATION
      python/casual/server/
)

install(
    FILES
      casual/xatmi/__init__.py
      casual/xatmi/xatmi.py
    DESTINATION
      python/casual/xatmi/
)

