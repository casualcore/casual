
# environment variables that affect operations

This document defines all environment variables that in some way affect operations


## paths

name                 | type     | default                            | description  
---------------------|----------|------------------------------------|----------------------------------------------------------------------------
`CASUAL_HOME`        | `string` | **has to be set**                  | where `casual` is installed
`CASUAL_DOMAIN_HOME` | `string` | **has to be set**                  | points to _home_ of current `casual domain`. 
`CASUAL_LOG_PATH`    | `string` | `$CASUAL_DOMAIN_HOME/casual.log`   | where to write logs



## terminal output

name                        | type      | default | description  
----------------------------|-----------|---------|----------------------------------------------------------------------------
`CASUAL_TERMINAL_PRECISION` | `integer` | `3`     | how many decimal points should be used on output to terminal
`CASUAL_TERMINAL_COLOR`     | `bool`    | `true`  | if output should be color enhanced or not
`CASUAL_TERMINAL_HEADER`    | `bool`    | `true`  | if output should print a header or not
`CASUAL_TERMINAL_PORCELAIN` | `bool`    | `false` | if output should be in an easy to parse format (overrides `CASUAL_TERMINAL_COLOR` and `CASUAL_TERMINAL_HEADER`)

These can be overridden with options when `CLI` is invoked.
