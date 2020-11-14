def filter_servers_by_name(services, name):
    for service in services:
        if service.get('name')  == name:
            return service

    return {}



def get_parent_server(service, servers):
    concurrent = service['instances']['concurrent']
    sequential = service['instances']['sequential']

    for seq in sequential:
        for s in servers:
            if seq['pid'] in s['instances_count']['running']:
                return s['alias']
