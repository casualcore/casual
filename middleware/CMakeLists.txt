add_subdirectory(common)
add_subdirectory(cli)
add_subdirectory(manager)

add_subdirectory(configuration)

add_subdirectory(domain) 
add_subdirectory(transaction)
add_subdirectory(service)

add_subdirectory(server)

add_subdirectory(tools)

# here we can build xatmi...
add_subdirectory(xatmi)
add_subdirectory(buffer)
# examples, used for testing below
add_subdirectory(example)

add_subdirectory(event)
add_subdirectory(file)
add_subdirectory(queue)
add_subdirectory(binding)
add_subdirectory(http)

# depends on domain, queue, service
add_subdirectory(gateway)

# admin cli
add_subdirectory(administration)

add_subdirectory(http/source/inbound/nginx/plugin)


