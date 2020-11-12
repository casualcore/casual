def filter_servers_by_name(services, name):
    for service in services:
        if service.get('name')  == name:
            return service

    return {}