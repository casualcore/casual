from flask import current_app as app
from casual.server.api import call
import casual.server.buffer as buffer
import json
import serverutil as server_util
import serviceutil as service_util
from datetime import datetime

class CasualService(object):

    def __init__(self):
      self._get_domain()
      self._get_services()
      self._get_gateways()
      self._get_transactions()
      self.current_server = {}
    
    def _get_domain(self):
      domain_call = call(".casual/domain/state", "")
      self.domain_state = json.loads(domain_call).get("result", dict())
      self.groups = self.domain_state.get("groups", [])
      self.executables = self.domain_state.get("executables", [])
      self.servers = self.domain_state.get("servers", [])
      self._set_servers()
      self._set_server_instances()

    def _get_services(self):
      service_call = call(".casual/service/state", "")
      self.service_state = json.loads(service_call).get("result", dict())
      self.services = self.service_state['services']
      self._set_service_metrics()

    def _get_gateways(self):
      gateways_call = call(".casual/gateway/state", "")
      self.gateways_state = json.loads(gateways_call).get("result", dict())

    def _get_transactions(self):
      transactions_call = call(".casual/transaction/state", "")
      self.transactions_state = json.loads(transactions_call).get("result", dict())

    def _reset_metrics(self, services):
        service_reset_call = call(".casual/service/metric/reset", json.dumps({"services":services}))
        service_reset = json.loads(service_reset_call).get("result", list())
        return service_reset




    def _set_servers(self):
      self._set_group_member_of()

    def get_groups(self):
        self._set_group_members()
        return self.groups

    
    def get_executables(self):
      return self.domain_state.get("executables", dict())
    
    def get_servers(self):
      #self._get_domain()
      return self.servers

    def get_server_by_alias(self, alias):
        self.current_server = server_util.filter_servers_by_alias(self.servers, alias)
        #self._set_server_services()
        server_util.set_server_services(self.current_server, self.services)
        return self.current_server

    def get_executable_by_alias(self, alias):
        self.current_server = server_util.filter_servers_by_alias(self.executables, alias)
        #self._set_server_services()
        server_util.set_server_services(self.current_server, self.services)
        return self.current_server
      
    def get_services(self, admin=False):
        #return self.service_state
        if admin:
            return self.service_state
        else:
            self.service_state['services'] = [x for x in self.service_state['services'] if not x['category'].startswith('.admin')]
            return self.service_state

    def get_service_by_name(self, name):
        service = service_util.filter_servers_by_name(self.services,name)

        service['parent'] = service_util.get_parent_server(service, self.servers)

        return service
    def get_gateways(self):
      return self.gateways_state

    def get_transactions(self):
      return self.transactions_state
    

    def _set_group_member_of(self):
      for group in self.groups:
          group['members'] = []
      for server_type in [self.servers, self.executables]:
        for server in server_type:
            server['memberof'] = []
            for group in self.groups:
                if group.get('id') in server['memberships']:
                        group['members'].append(server)
                        server['memberof'].append(group.get('name'))
      

    def _set_group_members(self):
        pass

    def _set_service_metrics(self):
        for service in self.services:
            last = service.get('metric').get('last')
            service['metric']['last'] = last / (1000*1000)

            for metric_type in ['invoked', 'pending']:
                metric = service['metric'].get(metric_type)
                min = metric['limit']['min'] / float(1000000)
                max = metric['limit']['max'] / float(1000000)
                count = metric['count']
                total = metric['total'] / float(1000000)
                avg = total / count if count > 0 else 0
                service['metric'][metric_type] = {"min":min,"max":max,"count":count,"total":total,   "avg":avg}

    def _set_server_instances(self):
        for server in self.servers:
            instances = server.get('instances', [])

            server['instances_count'] = server_util.format_instanses(instances)


    def _set_server_services(self):
        services = self.services
        server_util.set_server_services(self.current_server, services)


    def reset_metrics(self, services):
        return self._reset_metrics(services)

