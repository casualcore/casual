
# API 

`casual` has several administration services that can be called from users.
If `casual-http-inbound` is started for a domain, users can call these over 
http. Which makes it possible to create arbitrary administration tools to manage 
`casual`.

**Attention** `casual` might change administration service API between _minors_.
This due to the cost to keep compatibility. In the future we aim to lock the 
compatibility between majors.

## Available services

To list all available administration services use the following command:

```bash
$ casual service --list-admin-services
```

### `state`

Every _manager_ has a `state` service, that delivers the manager total state. 
These state-services are mainly used for listing different CLI views of the 
manager state.

The `state` services might give a more complete view than other CLI views, and
might give further insights.

## describe services

### CLI
To describe a casual-service just use the following command:

```bash
$ casual describe <service>
```

### http request

If the header `casual-service-describe` is set to `true`, the describe mechanism
will kick in and the payload will be the description (model) of the service.


Unfortunately, _describe_ does not contain _description/documentation_ about
what the service does, and how it works. Only what the service takes as input
and gives as output (this will be added in the future)


