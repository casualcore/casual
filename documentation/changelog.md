# Changelog
This is the changelog for `casual` and all changes are listed in this document.

## [Unreleased]

## [1.5.2] - 2021-09-17

### Fixes
- domain - fixed so order of groups (configuration) can be arbitrary, regardless of dependencies.
- discovery - made sure we _flush send_ to mitigate deadlocks when interacting with `casual-domain-discovery`

## [1.5.1] - 2021-09-11

### Fixes
- gateway - respect hops - round-robin on the partition with smallest hops
- gateway - fix interdomain protocol example tool
- paths - made sure we do not use std::filesystem::equivalent with non existing file
- cli - improved (subjectively) event::Notification print out (new for 1.5)
- configuration - fix bug with empty aliases for gateway
- configuration - fixed bug when 'update' an _alias key_ 'entity'. We moved from the one we supposed to add to...

## [1.5.0] - 2021-09-01
### Added
- configuration - runtime configuration _stage 1_ 
  - cli for _get, post, put_, and _edit_ which spawns an editor and when the configuration is _saved_ 
    `casual` tries to conform to the (possible) new configuration.
  - **attention:** under (heavy) load there might be some noise still, we aim to fix this in _stage 2_
- cli - version information without a domain running
- cli - added value `auto` to option `--header`. With `auto`, headers are **only** used if tty is bound to stdout
- gateway - handle queue- and service information in connection state viewable in cli
- xatmi - extended - function to browse instance services
- cli - made sure wrong written cli command doesn't gives error in log
- queue - handle zombie queues - queues that are not configured anymore, but still exists in the _queuebase_
- discovery - gateway inbound discovery forward
- discovery - central 'agent' for handling discoveries

### Changed
- transcoding - platform-independent third-party-header-only-lib for base64-transcoding
- paths - using std filesystem
  - **attention:** this might change semantics which _temporary path_ is used.
  - https://en.cppreference.com/w/cpp/filesystem/temp_directory_path
  - _On POSIX systems, the path may be the one specified in the environment variables_
      _TMPDIR, TMP, TEMP, TEMPDIR, and, if none of them are specified, the path "/tmp" is returned_
- python binding - upgraded to python3


### Fixes
- serialize - some simplifications - std::filesystem::path logs correctly
- common - 'helper' for `directory::create` to handle corner cases with links
- log - made the reopen on SIGHUP more robust
- common - less semi confusing logging
- xatmi - placed definition of functions where they should be to avoid confusion
- cygwin compatibility
- build - calculate version from tags -> more robust version sequences

### Internal
- configuration - better model - all managers supply their configuration
- unittest - more deterministic unittests
- common - algorithm clean up - moved some stuff to separate headers
- build - improved and fixed some build related stuff - simplification
- maintenance - simplify traits - some improvements in algorithm
- signal - simplified signal to only have one handler for a given signal at a given time
- build - fixed problem with linking on Ubuntu
- build - turning archive to a shared library to avoid ODR
- common - strong::Type - made a strong::Type that replaces "value::id" and "value::optional"
- common - error handling and log fixes
- build - casual-make - use new build system


## [1.4.20] - 2021-06-15
### Fixes
- queue - improved performance while dequeuing messages

## [1.4.18] - 2021-06-09
### Fixes
- gateway - tcp connect/accept errors might lead to inbound/outbound exit/restart
- service - removed false error log when a timeout occur and the callee is killed
- gateway - false error during tcp consume of logical (large) message 

## [1.4.16] - 2021-05-29
### Fixes
- gateway - improved tcp connection phase robustness -> spawns a process that take cares of the logical connection phase

## [1.4.15] - 2021-04-14
### Fixes
- service - pending lookups did not work correctly for _routes_
- gateway - made sure we terminate a detected 'loop' for calls
- gateway - improved connect to be more robust

## [1.4.14] - 2021-03-28
### Fixes
- gateway - outbound handle no-entry call reply correctly
- gateway - outbound correlates multiple branched trids correctly
- gateway - inbound respect service route name

## [1.4.12] - 2021-03-05
### Fixes
- gateway - asynchronous writes - mitigate deadlock, performance improvements
- gateway - handles bad tcp addresses in a more robust way
- gateway - honour outbound group order for priority
- configuration - gateway.connections -> outbound-group per connection
- cli - call keep logical message to one correlation id

## [1.4.9] - 2021-03-04
### Fixes
- gateway - outbound should prioritize replies from other domains
- cli - call not use pending message and use the 'flush send' idiom.

## [1.4.8] - 2021-03-03
### Fixes
- gateway - outbound/reverse-inbound might try to receive on wrong socket and get 'stuck'

## [1.4.7] - 2021-03-03
### Fixes
- domain - shutdown/restart makes 'noise' when provided services are under load

## [1.4.6] - 2021-02-25
### Fixes
- discovery - did not work correctly on service with multiple routes

## [1.4.5] - 2021-02-24
### Changed
- updated this changelog - missed it in 1.4.4

## [1.4.4] - 2021-02-24
### Fixes
- discovery - did not work correctly on service routes

## [1.4.3] - 2021-02-13
### Changed
- cli - updated documentation for --color with option auto

### Fixed
- cli - fixed so colors is false for Directive::plain()

## [1.4.2] - 2021-02-13
### Fixed
- service - added timeout contract to cli output
- queue - improved cli
- service - assassination target caller instead of callee

## [1.4.1] - 2021-02-07
### Added
- queue - concurrent queue-forward that is part of casual-queue
- service - manager replies with requested queue on service discovery
- domain runtime configuration core functionality
- gateway - reverse connection for inbound/outbound
- gateway - multiplexing (reverse) inbound/outbound
- gateway - 'soft' disconnect from inbound
- framework - serialize const lvalues
- gateway - inbound/outbound groups to group connections
- event - added event that signals when a discoverable is 'on-line'
- enable keepalive for tcp connections
- added a auto to cli --color
- making filesystem path serializable
- transaction - resource-property-files are optional
- possibility to terminate called servers when service timeout occurs
- configuration - validate not empty values

### Changed
- upgrade to c++17
- queue - reworked group and forward to be more rigid
- service - make sure service-manager honour server-alias restrictions
- remove 600 call trying limit in nginx

### Fixed
- gateway - did not add pending metric to request from service lookup
- transaction - tx_begin did 'start' the transaction even if RM xa_start failed
- defect in error logging
- handle null value of queue message payload
- queue-forward shall not handle new dequeue 'flows' in shutdown mode


## [1.3.15] - 2020-12-24
### Fixed
- queue - missing error_condition equivalent implementation for queue::code
  all values for queue::code was logged as `error`

## [1.3.13] - 2020-11-27
### Fixed
- queue - enqueue does not handle 'shutdown' correctly

## [1.3.7] - 2020-10-14
### Fixed
- service - same instance could be added to a service multiple times

## [1.3.6] - 2020-10-09
### Fixed
- gateway - rediscovery failed when there was unconnected outbounds

## [1.3.5] - 2020-09-22
### Fixed
- transaction - did not remove 'consumed' transaction

## [1.3.4] - 2020-09-17
### Added
- cli transaction support
- handle glob pattern selecting configuration files
- serialization - enable (de)serializing 'unnamed' objects
- buffer - field-from-string more relaxed when dealing with string values
- queue - improvement in performance

### Changed
- cli help improvements

### Fixed
- http outbound not marking transaction rollback-only on error reply
- forward service causing error in metrics
- handle null buffer over http
- queue - dequeue with id
- gateway - signal handling
- queue - missing composite index on message.gtrid and message.state
- queue - fix bad performance with enqueue when forwards are active
- xatmi - did not call tpsrvdone when shutting down server

## [1.2.0] - 2020-04-23
### Added
- gateway rediscover functionality
- group restart in domain

### Changed
- improved ipc message pump semantics
- cli improvement in aggregate information from managers 
- logging with iso-8601
- queue - metrics and cli
- reopen resource if failing

## [1.1.0] - 2020-03-08
### Added
- http inbound and outbound handles xatmi codes in header
- support for returning header parameters in http
- build executables with resources
- non blocking service lookup
- casual queue c-api
- support for queue retry and delay

### Changed
- logging reworked
- improved metrics semantics
- raw memory abstractions
- improvement in cli
- new implementation in http inbound
- reworked user configuration
- reworked marshal and serialize
- better route semantics
- administration api

### Fixed
- signal handling in non-blocking-send
- multiplexing problem
- protocol deduction error in http
- http outbound blocking
- base64 encoding/decoding in http
- handling transaction branches
- handling remote trid correct
- cobol binding
- service logging

## [1.0.0] - 2018-08-13
### Added
- first released version 6,5 years after first commit.
- all basic functionality supported

    

---

#### footnote
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).











