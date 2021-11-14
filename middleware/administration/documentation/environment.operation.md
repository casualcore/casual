
# environment variables that affect operations

This document defines all environment variables that in some way affect operations


## paths

name                                 | default                                     | description  
-------------------------------------|---------------------------------------------|------------------------------------------------
`CASUAL_HOME`                        | **has to be set**                           | where `casual` is installed
`CASUAL_DOMAIN_HOME`                 | **has to be set**                           | points to _home_ of current `casual domain`. 
`CASUAL_LOG_PATH`                    | `$CASUAL_DOMAIN_HOME/casual.log`            | where to write logs
`CASUAL_TRANSIENT_DIRECTORY`         | `($TMPDIR\|$TEMP\|$TMP)/.casual`              | where transient files are stored
`CASUAL_PERSISTENT_DIRECTORY`        | `$CASUAL_DOMAIN_HOME/.casual`               | where persistent files are stored
`CASUAL_IPC_DIRECTORY`               | `$CASUAL_TRANSIENT_DIRECTORY/ipc`           | where ipc files are stored
`CASUAL_TRANSACTION_DIRECTORY`       | `$CASUAL_PERSISTENT_DIRECTORY/transaction`  | where transaction database files are stored
`CASUAL_QUEUE_DIRECTORY`             | `$CASUAL_PERSISTENT_DIRECTORY/queue`        | where queue database files are stored (if not stated in configuration)
`CASUAL_SYSTEM_CONFIGURATION_GLOB`   | `$CASUAL_HOME/configuration/resources.yaml` | glob pattern for system configuration (including resource)
_`CASUAL_RESOURCE_CONFIGURATION_FILE`_ | _`$CASUAL_HOME/configuration/resources.yaml`_ | **deprecated**

## directives

name            | default        | description  
----------------|----------------|------------------------------------------------
`CASUAL_LOG`    |                | only matched log categories are logged. `error` is logged regardless


## terminal output

name                        | type                   | default | description  
----------------------------|------------------------|---------|----------------------------------------------------------------------------
`CASUAL_TERMINAL_PRECISION` | `integer`              |    `3`  | how many decimal points should be used on output to terminal
`CASUAL_TERMINAL_COLOR`     | `[true, false, auto]`  | `true`  | if output should be color enhanced or not. If auto, colors are used if tty is bound to stdout
`CASUAL_TERMINAL_HEADER`    | `[true, false, auto]`  | `true`  | if output should print a header or not. If auto, headers are used if tty is bound to stdout
`CASUAL_TERMINAL_BLOCK`     | `bool`                 | `true`  | if _invocations_ should block or not. If false, return control to user as soon as possible
`CASUAL_TERMINAL_VERBOSE`   | `bool`                 | `false` | if verbose mode is on or not. If true, additional information will printed (were possible)
`CASUAL_TERMINAL_PORCELAIN` | `bool`                 | `false` | if output should be in an easy to parse format (overrides `CASUAL_TERMINAL_COLOR` and `CASUAL_TERMINAL_HEADER`)

These can be overridden with options when the `CLI` is invoked.
