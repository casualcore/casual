# casual-test-mockup-string-server

A mockup server that emulates _string like buffer services_.

Match input against a regular expression and replies with the configured result.


## configuration

### start
To start from `casual`

```yaml

domain: 
   servers: 
      - path: "${CASUAL_HOME}/test/bin/casual-test-mockup-string-server"
        arguments: [ --configuration, "path/to/configuration/file" ]
```


### example

```yaml 

mockup: 
   entries: 
      - service: foo
        match: "^bar.*"
        # default X_OCTET
        type: X_OCTET
        result: "some string result"

      - service: foo
        match: "^foo.*"
        result: "some other string result with X_OCTET buffer"

      - service: bar
        match: "^banana.*"
        result: "some string result for service bar"
```