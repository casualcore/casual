
# environment variables that affect operations

This document defines all environment variables that in some way affect operations




## paths

### `<temp-dir>`

`casual` uses `<temp-dir>` as defined by the C++ standard utility 
[`std::filesystem::temp_directory_path`]( https://en.cppreference.com/w/cpp/filesystem/temp_directory_path).

> On POSIX systems, the path may be the one specified in the environment variables TMPDIR, TMP, TEMP, TEMPDIR, and, if none of 
> them are specified, the path "/tmp" is returned.


### uniqueness

Some paths/files need to be unique to the domain (and machine). That is, the absolute path.
This often resolve itself if every domain are actual users, with `HOME` directories. But if other _strategies_ are used, 
care needs to be taken regarding uniqueness of paths.


name                                   | default                                         | unique | description  
---------------------------------------|-------------------------------------------------|--------|----------------------------------------------
`CASUAL_HOME`                          | **has to be set**                               | `false`| where `casual` is installed
`CASUAL_DOMAIN_HOME`                   | **has to be set**                               | `true` | points to _home_ of current `casual domain`. 
`CASUAL_LOG_PATH`                      | `${CASUAL_DOMAIN_HOME}/casual.log`              | `false`| where to write logs
`CASUAL_TRANSIENT_DIRECTORY`           | `<temp-dir>/.casual`                            | `false`| where transient files are stored
`CASUAL_PERSISTENT_DIRECTORY`          | `${CASUAL_DOMAIN_HOME}/.casual`                 | `true` | where persistent files are stored
`CASUAL_IPC_DIRECTORY`                 | `${CASUAL_TRANSIENT_DIRECTORY}/ipc`             | `false`| where ipc _files_ are stored
`CASUAL_TRANSACTION_DIRECTORY`         | `${CASUAL_PERSISTENT_DIRECTORY}/transaction`    | `true` | where transaction 'database' file is stored (if not stated in configuration).
`CASUAL_QUEUE_DIRECTORY`               | `${CASUAL_PERSISTENT_DIRECTORY}/queue`          | `true` | where queue 'database' files are stored (if not stated in configuration).
`CASUAL_SYSTEM_CONFIGURATION_GLOB`     | `${CASUAL_HOME}/configuration/resources.yaml`   | `false`| glob pattern for system configuration (including resource)
_`CASUAL_RESOURCE_CONFIGURATION_FILE`_ | _`${CASUAL_HOME}/configuration/resources.yaml`_ | -      | **deprecated**

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
