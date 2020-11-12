
def filter_servers_by_alias(servers, alias):
    for server in servers:
        if alias == server.get('alias'):
                return server
    


def set_server_services(server, services):
    server['services'] = []
    for service in services:
        pass
    return server




def format_instanses(instances):
    ins = {}
    ins['running'] = list()
    ins['configured'] = list()
    
    for inst in instances:
        if inst['state'] == 0:
            ins['running'].append(inst['handle']['pid'])
        else:
            ins['configured'].append(inst['handle']['pid'])

    return ins