# Changelog
This is the changelog for `casual` and all changes are listed in this document.

## [1.7.0]

### Potentially breaking changes
### CLI
- `service --list-instances` column `state`, value `remote` has changed to `external` 
   to be consistent with other CLI values. We use _internal_ and _external_, where 
   _internal_ is stuff that is known locally to the "manager", and _external_ is stuff
   that is not known locally (could still be _external_ within a domain).
- `transaction --list-instances` in the `1.6` form has changed name to `transaction --list-internal-instances`.
   `transaction --list-instances` now lists all instances of all resources, _internal_
   and _external_. This makes it consistent with _service --list-instances_. 

## [1.6.19] - 2024-04-25
### Fixes
- http: improved error logging in http-outbound (#367)
- http: use route name in outbound when ack:ing service calls (#364)
- queue: add example server with enqueue and dequeue services (#348)
- documentation: improved configuration documentation
- documentation: updated and removed invalid api-documentation
- example: fix casual/example/work (#349)

## [1.6.18] - 2024-03-18
### Fixes
- domain: print domain name in shutdown task (#336)
- common: do not set environment when (re)setting execution-id (#333)
- http: propagate execution id over http (#326)
- gateway: fix markdown documentation for interdomain protocol (#319)
- sf: 'attribues' is misspelled in describe model (#264)
- domain: improve error messages (#315)       
- http: use a single correlation for entire service call (#318)
- ipc: fix discard while message is only partially received (#311)
- queue: clear available when moving message to error queue (#312)
- domain: assassinate needs more precise logging (#309)
- service: handle assassinate during shutdown (#299)
- gateway: outbound sent discovery to disconnect-mode connections (#297)

## [1.6.17] - 2023-11-13
### Fixes
- transaction: TM asserts when transaction is not found in prepare-handle (#288)
- cli: header output in porcelain if --header true (#283)
- service: perform discovery on external_discovery (#269)
- xatmi: comply to specification signal handling (#271)
- service: cli --list-services does not add - when _contract_ is missing (#281)
- cli: add precondition casual log can't be tied to stdout (#259)
- domain: propagate restart to configuration-get (#240)
- domain: handle restarts attribute correct (#277)
- transaction: cli - add legend for list-resources
- transaction: show metric for resource-proxy pending requests in cli (#273)
- transaction: fix resource-proxy metric (#273)
- cli: fixed and simplified the transaction cli (#272)
- queue: dequeue with id as selector should ignore available (#270)

## [1.6.16] - 2023-10-10
### Fixes
- queue: send reply on failed dequeues (#262)
- gateway: pending 'calls' could be sent after a rollback (#261)
- buffer: buffer-field-serialize should generate with [[maybe_unused]] (#255)

## [1.6.15] - 2023-09-14
### Fixes
- transaction: TM commits when it act as a resource and gets a prepare (#256)

## [1.6.14] - 2023-09-03
### Fixes
- event: make sure we try to reopen the event file if it's "broken" (#250)
- log: do not throw if we can't open logfile -> error to stderr (#250)
- cli: add _internal_ cli for generic message count metric 
- service: SM sends error-reply to no-reply request on timeout/core (#247)
- documentation: add topology::Update to protocol documentation
- service: improve handling of service timeout (config/cli) (#244)

## [1.6.13] - 2023-08-22
### Fixes
- http: nginx could block caller for ever with payloads > ~50k (#242, #243)
- configuration: prevent global contract setting being default overridden (#244)

## [1.6.12] - 2023-07-18
### Fixes
- discovery: hops are not correctly propagated if services are known

## [1.6.11] - 2023-06-22
### Fixes
- xatmi: error log unexpected error codes (#226)
- service: handle multiple busy lookups (#217)
- common: reduce complexity for the fan::Out abstraction
- queue: forward stability during sqlite error (#222)
- queue: group to handle > 1 message between persistent writes

## [1.6.10] - 2023-06-09
### Fixes
- gateway: outbound does not wait for in-flight transactions
- gateway: error-log if connection is lost unexpectedly (#216)
- gateway: inbound does not handle prepare::Reply with read_only correct
- service: avoid double busy lookup (#217)

## [1.6.9] - 2023-06-08
### Fixes
- gateway: inbound must wait for in-flight transactions during shutdown

## [1.6.8] - 2023-06-01
### Fixes
- discovery: "local" known services/queues are not added to discovery reply
- discovery: replies should only contain a subset of the requested
- service: include parent in service metrics


## [1.6.7] - 2023-06-01
### Fixes
- discovery: aggregate external discovery requests for performance
- domain: handle shutdown signals (#207)

## [1.6.6] - 2023-05-29
### Fixes
- discovery: handle prospect services (#198)
- domain: try to remove ipc device from exited 'singletons'
- gateway: handle routing problems (#197)
- gateway: outbound order of connection for a service gets too "orderly"
- gateway: outbound might core if connection lost during QM "restart"

## [1.6.5] - 2023-05-19
### Fixes
- ipc: improve detection of removed ipc devices (files) (#194)
- service: always send timeout duration in lookup reply
- service: service timeout could render zombie transactions (#191)
- gateway: always advertise discovered resources (#188) (#190)
- gateway: shutdown doesn't wait for inbounds before outbounds (#186)

## [1.6.4] - 2023-04-25
### Fixes
- buffer: fix field doesn't track 'service buffer' during auto realloc (#179)
- queue: prevent forward causing service-manager coredump (#182) (ported from 1.5.15)

## [1.6.3] - 2023-04-09
### Fixes
- sql: make sure sqlite uses synchronous=NORMAL (#177)
- queue: fix memory leak by not (implicitly) using setenv repeatedly (#175)

## [1.6.2] - 2023-03-25
### Fixes
- queue: extend c-api for get reply and get available
- configuration: fix queue configuration default directory
- queue: send error reply on failed enqueue

## [1.6.1] - 2023-03-13
### Fixes
- queue: fix pending data structure slicing (memory leak)
- documentation: fix sphinx-config
- build: fix version.py to handle new minor

## [1.6.0] - 2023-03-06
### Added
- cli: add force service/queue discovery to cli (#110)
- configuration: add set operations union, intersection, difference
- discovery: add build configuration option to set a service visibility (#86)
- discovery: add domain configuration option to set a service visibility (#86)
- discovery: add state and metric information to discovery (#110)
- discovery: add directive to limit service discovery from other domains
- domain: add start time for domain in information (#132)
- domain: log name of exiting process (#105)
- event: add event-logging for significant stuff that is done
- event: add service sequential/concurrent order to event-service-log (#115)
- general:  all "managers" use multiplex send
- http: outbound configuration to control caller fresh connect (#65)
- http: make http payload base64 encoding optional
- http: call::Context prepare for multiplexing and 'http-forward'
- http: use multiplexing in nginx inbound
- queue: add browse-peek to queue api
- queue: recover queue message (#74)
- transaction: list external resources from cli
- xatmi: extend c-api with execution service name (#140)

### Fixes
- build: package resources.yaml in /etc/casual/
- cli: fix gateway --list-connections to indicate inbound discovery forward
- cli: fix domain --configuration-get not outputing 'system' (#108)
- cli: generalized --state option and the serialization to stdout
- cli: fix stderr printout on error from `casual call ...`
- cli: add legend for gateway --list-connections
- cli: fix transaction cli output
- cli: corrected legend for --list-services option to match header
- common: add error logging on assert (#88)
- configuration: improve configuration update semantics
- configuration: default to one instance everywhere
- discovery: don't forward discovery if service/queue is known (#141)
- domain: core servers must restart on unexpected exit
- domain: keep service and queues sorted in discovery (#77)
- event: fix event-service-log to also reopen 'casual.log' on SIGHUP
- event: show trid in metric when trid is created by callee (#142)
- event: make sure service-log listen to events before "connect" 
- environment: don't create directories when constructing internal environment paths (#19)
- forward: generate new execution id for each service call
- forward: fix forwards dying when scaling aliases
- gateway: reduce batch read/write in tcp handler
- gateway: don't let children inherit tcp file descriptors
- gateway: fix tcp-logical-connect to detect loss of destination ipc
- http: correct http inbound to handle keepalive
- http: upgrade nginx to latest stable 1.22.0
- http: remove debug log level as default (#79)
- log: add api to configure _log_ during runtime
- service: fix pending "error" reply when server cores/exit
- transaction: fix core when resources in distributed prepare (service) fails
- queue: fix local lookup to also search external known queues
- queue: log queuename when failing operation on queues (#106)
- queue: fix cli to handle queue message with message attributes
- queue: fix --list-remote
- xatmi: tpsrvdone should never be called if tpsrvinit has not been called

## [1.5.14] - 2022-09-08
### Fixes
- conversation: fix a bug where tprecv and tpsend failed to set the global tpurcode.
- conversation: generate events in tpsend when partner calls tpreturn or tpdiscon.
- conversation: fix handling of server process exit/core.

## [1.5.13] - 2022-08-16
### Fixes
- queue: add lookup restrictions depending on caller location
- service: add lookup restrictions depending on caller location
- queue: fix pre/post statements for "tuning"
- http: fix outbound to advertise services with order 0
- discovery: fix using routes for services correctly

## [1.5.12] - 2022-05-10
### Fixes
- cli: fix configuration --normalize
- gateway: fix outbound to handle 'gateway loop'
- gateway: fix inbound to stop consume from a removed connection
- service: use multiplex send as much as possible
- service: fix lookup-forget-reply has _replied_ for remote services
- gateway: improve multiplex send, and consume ratio between ipc and tcp
- discovery: reduce amount of topology related discoveries
- discovery: use batch topology update, semantics for performance
- tcp: guard errors in socket::address::host/peer
- gateway: fix outbound to always remove transaction state
- transaction: fix external resource prepare/commit request
- gateway: handle lost connection in tcp send correctly
- transaction: fix notification to TM for services with branch semantics
- transaction: fix problem when to use TMJOIN in xa_start
- transaction: be conformant with flags to xa_end
- discovery: use multiplex send as much as possible

## [1.5.11] - 2022-04-19
### Fixes
- cli: fix confusing arguments for casual service --legend
- cli - remove extra root `domain`: casual configuration --normalize
- gateway: make gateway more robust in error situations
- xatmi: suspend involved resources during tpcall
- gateway: absorb tcp errors and only effect the relevant tcp-connection

### Changes
- `casual` has moved to GitHub

## [1.5.10] - 2022-03-21
### Fixes
- gateway: use multiplexing on the tcp-connection phase, no blocking if the
  tcp-connect takes a long time.
- cli: fix `casual domain --list-servers` to show correct number of configured instances
- transaction: fix local transaction with multiple resources
   - `casual` did not handle this correct (at all)
- domain: fix shutdown if an instance of a server has died 

## [1.5.9] - 2022-02-18
### Fixes
- discovery: propagate topology updates upstream
   - When a domain logically connects (_connector_) to another domain (_connectee_), the _connector_
     propagates `topology update` to all "it's inbounds" that has `discovery:forward true` 
     configured. Hence, enable (auto) discovery for stuff arbitrary number of _nodes_ upstream.
- gateway: "move" listeners and connectors to failed on fatal errors.
   - Listeners: on error we move the listener directly to _failed_
   - Connectors: on fatal errors we move the connection to _failed_,
      on non fatal we try again after some time (as before).
- cobol: set return status in Cobol api TPRETURN
- gateway: keep trying to connect if the logical connection fails

## [1.5.8] - 2022-01-08
### Fixes
- transaction: fix distributed transaction bug in complex topology
- queue: forward react if queue group "dies"
- xatmi: conversation disconnect not properly handled
- queue: return `no_queue` iso `no_message` when writing to non existing queue
- discovery - on discoverable::Available event: we only discover when it makes sense

### Internal
- log: made sure we only use our own stream::write and got rid of global ostream stream operator
- common - flag abstraction degradation: equality operator for underlying enum type messed up semantics

## [1.5.7] - 2021-11-26
### Fixes
- transaction: fix distributed commit when RM fails -> rollback
- discovery: registration for _discoverables_ more robust with request->reply
   - Primarily for unittests when booting, testing and shutdown of multiple domains 
     within a couple of 100ms or so.

## [1.5.6] - 2021-11-15

### Changed
- configuration: added `system` at the same level as `domain`
   - `system` holds system wide configuration, and for the time being only `system.resource` which
      replaces the _resource.properties file_ (`casual` can still handle the deprecated file though).
- cli: reverted the 'locale' awareness for the `CLI`
   - To many subtle and not to subtle problems - not worth it.
- configuration: service restrictions for servers are now treated as regex
   - If set, only services that matches at least one regex are advertised.

## [1.5.5] - 2021-11-09

### Fixes
- xatmi - conversation: major refactoring
- xatmi: fix leak of descriptors for conversational services.
- cli - configuration edit: made sure we sink the SIGCHILD signal from the spawned editor
- documentation: fixed the example configuration files
- queue: forward to remote queues did not work correctly

## [1.5.4] - 2021-10-23

### Fixes
- transaction - if rm:s report error during xa_start: reply with TPESVCERR
- configuration|cli - validate: only positive integers for instances
- cli: version information, lowercase key names, compiler version
- queue: support queue-peek in c-api

## [1.5.3] - 2021-09-22

### Fixes
- http: outbound handle discovery (_http-services_ are treated _external_ as they should)
- xatmi: support for conversations (primarily targeting COBOL)
- gateway: fixed so outbound holds unique connections per service
- discovery: service-manager only replies with _local_ services
   - **attention** if the intention is that a domain should expose services that the domain has
     found in other domains (not _local_), the domains inbound connections should have the directive
     `discovery.forward = true` set. This also applies to `http` exposed services (since they're not local)

## [1.5.2] - 2021-09-17

### Fixes
- domain: fixed so order of groups (configuration) can be arbitrary, regardless of dependencies.
- discovery: made sure we _flush send_ to mitigate deadlocks when interacting with `casual-domain-discovery`

## [1.5.1] - 2021-09-11

### Fixes
- gateway - respect hops: round-robin on the partition with smallest hops
- gateway: fix interdomain protocol example tool
- paths: made sure we do not use std::filesystem::equivalent with non existing file
- cli: improved (subjectively) event::Notification print out (new for 1.5)
- configuration: fix bug with empty aliases for gateway
- configuration: fixed bug when 'update' an _alias key_ 'entity'. We moved from the one we supposed to add to...

## [1.5.0] - 2021-09-01
### Added
- configuration: runtime configuration _stage 1_ 
  - cli for _get, post, put_, and _edit_ which spawns an editor and when the configuration is _saved_ 
    `casual` tries to conform to the (possible) new configuration.
  - **attention:** under (heavy) load there might be some noise still, we aim to fix this in _stage 2_
- cli: version information without a domain running
- cli: added option to reopen _casual.log_. 
   Prints information of all processes that `casual` are not sure can handle a `SIGHUP`, hence it easier for users
   to _automate_ log-rotation.
- cli: make `casual` cli user 'locale' aware, to play nicer with `unix` tools, such as `sort`
- cli: added value `auto` to option `--header`. With `auto`, headers are **only** used if tty is bound to stdout
- gateway: handle queue- and service information in connection state viewable in cli
- xatmi - extended: function to browse instance services
- cli: made sure wrong written cli command doesn't gives error in log
- queue - handle zombie queues: queues that are not configured anymore, but still exists in the _queuebase_
- discovery: gateway inbound discovery forward
- discovery: central 'agent' for handling discoveries

### Changed
- transcoding: platform-independent third-party-header-only-lib for base64-transcoding
- paths: using std filesystem
  - **attention:** this might change semantics which _temporary path_ is used.
  - https://en.cppreference.com/w/cpp/filesystem/temp_directory_path
  - _On POSIX systems, the path may be the one specified in the environment variables_
      _TMPDIR, TMP, TEMP, TEMPDIR, and, if none of them are specified, the path "/tmp" is returned_
- python binding: upgraded to python3


### Fixes
- serialize - some simplifications: std::filesystem::path logs correctly
- common: 'helper' for `directory::create` to handle corner cases with links
- log: made the reopen on SIGHUP more robust
- common: less semi confusing logging
- xatmi: placed definition of functions where they should be to avoid confusion
- cygwin compatibility
- build: calculate version from tags -> more robust version sequences

### Internal
- configuration - better model: all managers supply their configuration
- unittest: more deterministic unittests
- common - algorithm clean up: moved some stuff to separate headers
- build - improved and fixed some build related stuff: simplification
- maintenance - simplify traits: some improvements in algorithm
- signal: simplified signal to only have one handler for a given signal at a given time
- build: fixed problem with linking on Ubuntu
- build: turning archive to a shared library to avoid ODR
- common - strong::Type: made a strong::Type that replaces "value::id" and "value::optional"
- common: error handling and log fixes
- build - casual-make: use new build system


## [1.4.20] - 2021-06-15
### Fixes
- queue: improved performance while dequeuing messages

## [1.4.18] - 2021-06-09
### Fixes
- gateway: tcp connect/accept errors might lead to inbound/outbound exit/restart
- service: removed false error log when a timeout occur and the callee is killed
- gateway: false error during tcp consume of logical (large) message 

## [1.4.16] - 2021-05-29
### Fixes
- gateway: improved tcp connection phase robustness -> spawns a process that take cares of the logical connection phase

## [1.4.15] - 2021-04-14
### Fixes
- service: pending lookups did not work correctly for _routes_
- gateway: made sure we terminate a detected 'loop' for calls
- gateway: improved connect to be more robust

## [1.4.14] - 2021-03-28
### Fixes
- gateway: outbound handle no-entry call reply correctly
- gateway: outbound correlates multiple branched trids correctly
- gateway: inbound respect service route name

## [1.4.12] - 2021-03-05
### Fixes
- gateway - asynchronous writes: mitigate deadlock, performance improvements
- gateway: handles bad tcp addresses in a more robust way
- gateway: honour outbound group order for priority
- configuration: gateway.connections -> outbound-group per connection
- cli: call keep logical message to one correlation id

## [1.4.9] - 2021-03-04
### Fixes
- gateway: outbound should prioritize replies from other domains
- cli: call not use pending message and use the 'flush send' idiom.

## [1.4.8] - 2021-03-03
### Fixes
- gateway: outbound/reverse-inbound might try to receive on wrong socket and get 'stuck'

## [1.4.7] - 2021-03-03
### Fixes
- domain: shutdown/restart makes 'noise' when provided services are under load

## [1.4.6] - 2021-02-25
### Fixes
- discovery: did not work correctly on service with multiple routes

## [1.4.5] - 2021-02-24
### Changed
- updated this changelog: missed it in 1.4.4

## [1.4.4] - 2021-02-24
### Fixes
- discovery: did not work correctly on service routes

## [1.4.3] - 2021-02-13
### Changed
- cli: updated documentation for --color with option auto

### Fixed
- cli: fixed so colors is false for Directive::plain()

## [1.4.2] - 2021-02-13
### Fixed
- service: added timeout contract to cli output
- queue: improved cli
- service: assassination target caller instead of callee

## [1.4.1] - 2021-02-07
### Added
- queue: concurrent queue-forward that is part of casual-queue
- service: manager replies with requested queue on service discovery
- domain runtime configuration core functionality
- gateway: reverse connection for inbound/outbound
- gateway: multiplexing (reverse) inbound/outbound
- gateway: 'soft' disconnect from inbound
- framework: serialize const lvalues
- gateway: inbound/outbound groups to group connections
- event: added event that signals when a discoverable is 'on-line'
- enable keepalive for tcp connections
- added a auto to cli --color
- making filesystem path serializable
- transaction: resource-property-files are optional
- possibility to terminate called servers when service timeout occurs
- configuration: validate not empty values

### Changed
- upgrade to c++17
- queue: reworked group and forward to be more rigid
- service: make sure service-manager honour server-alias restrictions
- remove 600 call trying limit in nginx

### Fixed
- gateway: did not add pending metric to request from service lookup
- transaction: tx_begin did 'start' the transaction even if RM xa_start failed
- defect in error logging
- handle null value of queue message payload
- queue-forward shall not handle new dequeue 'flows' in shutdown mode


## [1.3.15] - 2020-12-24
### Fixed
- queue: missing error_condition equivalent implementation for queue::code
  all values for queue::code was logged as `error`

## [1.3.13] - 2020-11-27
### Fixed
- queue: enqueue does not handle 'shutdown' correctly

## [1.3.7] - 2020-10-14
### Fixed
- service: same instance could be added to a service multiple times

## [1.3.6] - 2020-10-09
### Fixed
- gateway: rediscovery failed when there was unconnected outbounds

## [1.3.5] - 2020-09-22
### Fixed
- transaction: did not remove 'consumed' transaction

## [1.3.4] - 2020-09-17
### Added
- cli transaction support
- handle glob pattern selecting configuration files
- serialization: enable (de)serializing 'unnamed' objects
- buffer: field-from-string more relaxed when dealing with string values
- queue: improvement in performance

### Changed
- cli help improvements

### Fixed
- http outbound not marking transaction rollback-only on error reply
- forward service causing error in metrics
- handle null buffer over http
- queue: dequeue with id
- gateway: signal handling
- queue: missing composite index on message.gtrid and message.state
- queue: fix bad performance with enqueue when forwards are active
- xatmi: did not call tpsrvdone when shutting down server

## [1.2.0] - 2020-04-23
### Added
- gateway rediscover functionality
- group restart in domain

### Changed
- improved ipc message pump semantics
- cli improvement in aggregate information from managers 
- logging with iso-8601
- queue: metrics and cli
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











