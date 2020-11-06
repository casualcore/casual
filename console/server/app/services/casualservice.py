from flask import current_app as app
from casual.server.api import call
import json

class CasualService:

  def __init__(self):
    self._get_domain()
    self._get_services()
    self._get_gateways()
    self._get_transactions()
  
  def _get_domain(self):
    domain_call = call(".casual/domain/state", "")
    self.domain_state = json.loads(domain_call).get("result", dict())
    self.groups = self.domain_state.get("groups", [])
    self.executables = self.domain_state.get("executables", [])
    self.servers = self.domain_state.get("servers", [])
    self._set_servers()

  def _get_services(self):
    service_call = call(".casual/service/state", "")
    self.service_state = json.loads(service_call).get("result", dict())

  def _get_gateways(self):
    gateways_call = call(".casual/gateway/state", "")
    self.gateways_state = json.loads(gateways_call).get("result", dict())

  def _get_transactions(self):
    transactions_call = call(".casual/transaction/state", "")
    self.transactions_state = json.loads(transactions_call).get("result", dict())



  def _set_servers(self):
    self._set_group_member_of()

  def get_groups(self):
    return self.groups

  
  def get_executables(self):
    return self.domain_state.get("executables", dict())
  
  def get_servers(self):
    #self._get_domain()
    return self.servers
    
  def get_services(self):
    return self.service_state

  def get_gateways(self):
    return self.gateways_state

  def get_transactions(self):
    return self.transactions_state
  


  def _set_group_member_of(self):
    for server_type in [self.servers, self.executables]:
      #print("SERVERTYTPE")
      for server in server_type:
          #print(server)
          server['memberof'] = []
          for group in self.groups:
              if group['id'] in server['memberships']:
                  server['memberof'].append(group)




