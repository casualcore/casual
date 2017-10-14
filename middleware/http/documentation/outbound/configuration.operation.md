# http outbound configuration

Intended for operation

The example below should be self explanatory.


## example



```yaml

http:
  default:
     headers: # will be applied to all services that does not _override_
       - name: some-header-name
         value: some-header-value
         
  services:
    - name: some/xatmi/service/name/a # service name that will be advertised within the domain.
      url: http://some.host.org/casual/some/xatmi/service/name/a # url that call will be forwarded to.
    
    - name: some.xatmi.service.name.b
      url: http://some.host.org/casual/some.xatmi.service.name.b
      headers: # overrides headers
        - name: some-header-name
          value: some-header-value



```
