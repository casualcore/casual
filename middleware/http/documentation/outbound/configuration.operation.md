# http outbound configuration

Intended for operations

The example below should be self explanatory.


## example

```yaml

http:
  default:
     headers: # will be applied to all services that do not _override_
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

## environments

name                                   | type    | default | description  
---------------------------------------|---------|---------|------------------------------------------------------------------------------------
`CASUAL_HTTP_CURL_FORCE_FRESH_CONNECT` | `bool`  | `false` | force a new connection for every call
`CASUAL_HTTP_CURL_VERBOSE`             | `bool`  | `false` | internal "verbose" logging from cURL 
`CASUAL_HTTP_FORCE_BINARY_BASE64`      | `bool`  | `false` | force base64 payload encoding for _not text_ buffers. Has to be set to communicate with < 1.6


## attention

As of `1.6` `casual` no longer _base64_ (en|de)codes payloads (which was a mistake to begin with). If your intention is to run
`1.5` and `1.6` at the same time, and use http, you need to explicitly specify to _force base64 encoding_ for `1.6 <--> 1.5` "pairs"
on the `1.6` side.  

This compatibility option will be removed in `2.0`

## domain configuration


### example

```yaml
domain:
  name: some-domain
  servers:
    # http outbound
    - path: ${CASUAL_HOME}/bin/casual-http-outbound
      # a configuration files (glob pattern) with the content described above
      arguments: [ --configuration, "${CASUAL_DOMAIN_HOME}/configuration/http.yaml" ]

      # if the other side is <= 1.5
      environment:
        variables:
          - key: "CASUAL_HTTP_FORCE_BINARY_BASE64"
            value: "true"

  executables:
    # http inbound
    - alias: casual-http-inbound
      path: ${CASUAL_HOME}/nginx/sbin/nginx
      arguments: [ -c, "${CASUAL_DOMAIN_HOME}/configuration/nginx.conf", -p, "${CASUAL_DOMAIN_HOME}" ]

      # if the other side is <= 1.5
      environment:
        variables:
          - key: "CASUAL_HTTP_FORCE_BINARY_BASE64"
            value: "true"
  # ...

```
