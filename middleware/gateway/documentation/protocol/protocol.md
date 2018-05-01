
# casual domain protocol _version 1000_

Attention, this documentation refers to **version 1000** (aka, version 1)




Defines what messages is sent between domains and exactly what they contain. 

Some definitions:

* `fixed array`: an array of fixed size where every element is an 8 bit byte.
* `dynamic array`: an array with dynamic size, where every element is an 8 bit byte.

If an attribute name has `element` in it, for example: `services.element.timeout`, the
data is part of an element in a container. You should read it as `container.element.attribute`


## common::communication::message::complete::network::Header 

This header will be the first part of every message below, hence it's name, _Header_

message.type is used to dispatch to handler for that particular message, and knows how to (un)marshal and act on the message.

It's probably a good idea (probably the only way) to read the header only, to see how much more one has to read to get
the rest of the message.


role name          | network type | network size | description                                  
------------------ | ------------ | ------------ | ---------------------------------------------
[0mheader.type       [0m | [0muint64      [0m | [0m           8[0m | [0mtype of the message that the payload contains[0m
[0mheader.correlation[0m | [0mfixed array [0m | [0m          16[0m | [0mcorrelation id of the message                [0m
[0mheader.size       [0m | [0muint64      [0m | [0m           8[0m | [0mthe size of the payload that follows         [0m

## domain connect messages

messages that is used to set up a connection


### common::message::gateway::domain::connect::Request
   
Connection requests from another domain that wants to connect
   
   message type: **7200**

role name                 | network type  | network size | description                                            
------------------------- | ------------- | ------------ | -------------------------------------------------------
[0mexecution                [0m | [0mfixed array  [0m | [0m          16[0m | [0muuid of the current execution path                     [0m
[0mdomain.id                [0m | [0mfixed array  [0m | [0m          16[0m | [0muuid of the outbound domain                            [0m
[0mdomain.name.size         [0m | [0muint64       [0m | [0m           8[0m | [0msize of the outbound domain name                       [0m
[0mdomain.name.data         [0m | [0mdynamic array[0m | [0m           8[0m | [0mdynamic byte array with the outbound domain name       [0m
[0mprotocol.versions.size   [0m | [0muint64       [0m | [0m           8[0m | [0mnumber of protocol versions outbound domain can 'speak'[0m
[0mprotocol.versions.element[0m | [0muint64       [0m | [0m           8[0m | [0ma protocol version                                     [0m

### common::message::gateway::domain::connect::Reply
   
Connection reply
   
   message type: **7201**

role name        | network type  | network size | description                                                       
---------------- | ------------- | ------------ | ------------------------------------------------------------------
[0mexecution       [0m | [0mfixed array  [0m | [0m          16[0m | [0muuid of the current execution path                                [0m
[0mdomain.id       [0m | [0mfixed array  [0m | [0m          16[0m | [0muuid of the inbound domain                                        [0m
[0mdomain.name.size[0m | [0muint64       [0m | [0m           8[0m | [0msize of the inbound domain name                                   [0m
[0mdomain.name.data[0m | [0mdynamic array[0m | [0m           8[0m | [0mdynamic byte array with the inbound domain name                   [0m
[0mprotocol.version[0m | [0muint64       [0m | [0m           8[0m | [0mthe chosen protocol version to use, or invalid (0) if incompatible[0m

## Discovery messages

### domain discovery 


#### message::gateway::domain::discover::Request

Sent to and received from other domains when one domain wants discover information abut the other.

message type: **7300**

role name             | network type  | network size | description                                                  
--------------------- | ------------- | ------------ | -------------------------------------------------------------
[0mexecution            [0m | [0mfixed array  [0m | [0m          16[0m | [0muuid of the current execution path                           [0m
[0mdomain.id            [0m | [0mfixed array  [0m | [0m          16[0m | [0muuid of the caller domain                                    [0m
[0mdomain.name.size     [0m | [0muint64       [0m | [0m           8[0m | [0msize of the caller domain name                               [0m
[0mdomain.name.data     [0m | [0mdynamic array[0m | [0m           8[0m | [0mdynamic byte array with the caller domain name               [0m
[0mservices.size        [0m | [0muint64       [0m | [0m           8[0m | [0mnumber of requested services to follow (an array of services)[0m
[0mservices.element.size[0m | [0muint64       [0m | [0m           8[0m | [0msize of the current service name                             [0m
[0mservices.element.data[0m | [0mdynamic array[0m | [0m         128[0m | [0mdynamic byte array of the current service name               [0m
[0mqueues.size          [0m | [0muint64       [0m | [0m           8[0m | [0mnumber of requested queues to follow (an array of queues)    [0m
[0mqueues.element.size  [0m | [0muint64       [0m | [0m           8[0m | [0msize of the current queue name                               [0m
[0mqueues.element.data  [0m | [0mdynamic array[0m | [0m         128[0m | [0mdynamic byte array of the current queue name                 [0m

#### message::gateway::domain::discover::Reply

Sent to and received from other domains when one domain wants discover information abut the other.

message type: **7301**

role name                      | network type  | network size | description                                                     
------------------------------ | ------------- | ------------ | ----------------------------------------------------------------
[0mexecution                     [0m | [0mfixed array  [0m | [0m          16[0m | [0muuid of the current execution path                              [0m
[0mdomain.id                     [0m | [0mfixed array  [0m | [0m          16[0m | [0muuid of the caller domain                                       [0m
[0mdomain.name.size              [0m | [0muint64       [0m | [0m           8[0m | [0msize of the caller domain name                                  [0m
[0mdomain.name.data              [0m | [0mdynamic array[0m | [0m           0[0m | [0mdynamic byte array with the caller domain name                  [0m
[0mservices.size                 [0m | [0muint64       [0m | [0m           8[0m | [0mnumber of services to follow (an array of services)             [0m
[0mservices.element.name.size    [0m | [0muint64       [0m | [0m           8[0m | [0msize of the current service name                                [0m
[0mservices.element.name.data    [0m | [0mdynamic array[0m | [0m         128[0m | [0mdynamic byte array of the current service name                  [0m
[0mservices.element.category.size[0m | [0muint64       [0m | [0m           8[0m | [0msize of the current service category                            [0m
[0mservices.element.category.data[0m | [0mdynamic array[0m | [0m           0[0m | [0mdynamic byte array of the current service category              [0m
[0mservices.element.transaction  [0m | [0muint16       [0m | [0m           2[0m | [0mservice transaction mode (auto, atomic, join, none)             [0m
[0mservices.element.timeout      [0m | [0muint64       [0m | [0m           8[0m | [0mservice timeout                                                 [0m
[0mservices.element.hops         [0m | [0muint64       [0m | [0m           8[0m | [0mnumber of domain hops to the service (local services has 0 hops)[0m
[0mqueues.size                   [0m | [0muint64       [0m | [0m           8[0m | [0mnumber of requested queues to follow (an array of queues)       [0m
[0mqueues.element.size           [0m | [0muint64       [0m | [0m           8[0m | [0msize of the current queue name                                  [0m
[0mqueues.element.data           [0m | [0mdynamic array[0m | [0m         128[0m | [0mdynamic byte array of the current queue name                    [0m
[0mqueues.element.retries        [0m | [0muint64       [0m | [0m           8[0m | [0mhow many 'retries' the queue has                                [0m

## Service messages

### Service call 


#### message::service::call::Request

Sent to and received from other domains when one domain wants call a service in the other domain

message type: **3100**

role name           | network type  | network size | description                                                        
------------------- | ------------- | ------------ | -------------------------------------------------------------------
[0mexecution          [0m | [0mfixed array  [0m | [0m          16[0m | [0muuid of the current execution path                                 [0m
[0mservice.name.size  [0m | [0muint64       [0m | [0m           8[0m | [0mservice name size                                                  [0m
[0mservice.name.data  [0m | [0mdynamic array[0m | [0m         128[0m | [0mbyte array with service name                                       [0m
[0mservice.timeout    [0m | [0muint64       [0m | [0m           8[0m | [0mtimeout of the service in use (in microseconds)                    [0m
[0mparent.name.size   [0m | [0muint64       [0m | [0m           8[0m | [0mparent service name size                                           [0m
[0mparent.name.data   [0m | [0mdynamic array[0m | [0m         128[0m | [0mbyte array with parent service name                                [0m
[0mxid.format         [0m | [0muint64       [0m | [0m           8[0m | [0mxid format type. if 0 no more information of the xid is transported[0m
[0mxid.gtrid_length   [0m | [0muint64       [0m | [0m           8[0m | [0mlength of the transaction gtrid part                               [0m
[0mxid.bqual_length   [0m | [0muint64       [0m | [0m           8[0m | [0mlength of the transaction branch part                              [0m
[0mxid.payload        [0m | [0mdynamic array[0m | [0m          32[0m | [0mbyte array with the size of gtrid_length + bqual_length (max 128)  [0m
[0mflags              [0m | [0muint64       [0m | [0m           8[0m | [0mXATMI flags sent to the service                                    [0m
[0mbuffer.type.size   [0m | [0muint64       [0m | [0m           8[0m | [0mbuffer type name size                                              [0m
[0mbuffer.type.data   [0m | [0mdynamic array[0m | [0m          25[0m | [0mbyte array with buffer type in the form 'type/subtype'             [0m
[0mbuffer.payload.size[0m | [0muint64       [0m | [0m           8[0m | [0mbuffer payload size (could be very big)                            [0m
[0mbuffer.payload.data[0m | [0mdynamic array[0m | [0m        1024[0m | [0mbuffer payload data (with the size of buffer.payload.size)         [0m

#### message::service::call::Reply

Reply to call request

message type: **3101**

role name                         | network type  | network size | description                                                                   
--------------------------------- | ------------- | ------------ | ------------------------------------------------------------------------------
[0mexecution                        [0m | [0mfixed array  [0m | [0m          16[0m | [0muuid of the current execution path                                            [0m
[0mcall.status                      [0m | [0muint32       [0m | [0m           4[0m | [0mXATMI error code, if any.                                                     [0m
[0mcall.code                        [0m | [0muint64       [0m | [0m           8[0m | [0mXATMI user supplied code                                                      [0m
[0mtransaction.trid.xid.format      [0m | [0muint64       [0m | [0m           8[0m | [0mxid format type. if 0 no more information of the xid is transported           [0m
[0mtransaction.trid.xid.gtrid_length[0m | [0muint64       [0m | [0m           8[0m | [0mlength of the transaction gtrid part                                          [0m
[0mtransaction.trid.xid.bqual_length[0m | [0muint64       [0m | [0m           8[0m | [0mlength of the transaction branch part                                         [0m
[0mtransaction.trid.xid.payload     [0m | [0mdynamic array[0m | [0m          32[0m | [0mbyte array with the size of gtrid_length + bqual_length (max 128)             [0m
[0mtransaction.state                [0m | [0muint8        [0m | [0m           1[0m | [0mstate of the transaction TX_ACTIVE, TX_TIMEOUT_ROLLBACK_ONLY, TX_ROLLBACK_ONLY[0m
[0mbuffer.type.size                 [0m | [0muint64       [0m | [0m           8[0m | [0mbuffer type name size                                                         [0m
[0mbuffer.type.data                 [0m | [0mdynamic array[0m | [0m          25[0m | [0mbyte array with buffer type in the form 'type/subtype'                        [0m
[0mbuffer.payload.size              [0m | [0muint64       [0m | [0m           8[0m | [0mbuffer payload size (could be very big)                                       [0m
[0mbuffer.payload.data              [0m | [0mdynamic array[0m | [0m        1024[0m | [0mbuffer payload data (with the size of buffer.payload.size)                    [0m

## Transaction messages

### Resource prepare


#### message::transaction::resource::prepare::Request

Sent to and received from other domains when one domain wants to prepare a transaction. 

message type: **5201**

role name        | network type  | network size | description                                                        
---------------- | ------------- | ------------ | -------------------------------------------------------------------
[0mexecution       [0m | [0mfixed array  [0m | [0m          16[0m | [0muuid of the current execution path                                 [0m
[0mxid.format      [0m | [0muint64       [0m | [0m           8[0m | [0mxid format type. if 0 no more information of the xid is transported[0m
[0mxid.gtrid_length[0m | [0muint64       [0m | [0m           8[0m | [0mlength of the transaction gtrid part                               [0m
[0mxid.bqual_length[0m | [0muint64       [0m | [0m           8[0m | [0mlength of the transaction branch part                              [0m
[0mxid.payload     [0m | [0mdynamic array[0m | [0m          32[0m | [0mbyte array with the size of gtrid_length + bqual_length (max 128)  [0m
[0mresource.id     [0m | [0muint32       [0m | [0m           4[0m | [0mRM id of the resource - has to correlate with the reply            [0m
[0mflags           [0m | [0muint64       [0m | [0m           8[0m | [0mXA flags to be forward to the resource                             [0m

#### message::transaction::resource::prepare::Reply

Sent to and received from other domains when one domain wants to prepare a transaction. 

message type: **5202**

role name        | network type  | network size | description                                                        
---------------- | ------------- | ------------ | -------------------------------------------------------------------
[0mexecution       [0m | [0mfixed array  [0m | [0m          16[0m | [0muuid of the current execution path                                 [0m
[0mxid.format      [0m | [0muint64       [0m | [0m           8[0m | [0mxid format type. if 0 no more information of the xid is transported[0m
[0mxid.gtrid_length[0m | [0muint64       [0m | [0m           8[0m | [0mlength of the transaction gtrid part                               [0m
[0mxid.bqual_length[0m | [0muint64       [0m | [0m           8[0m | [0mlength of the transaction branch part                              [0m
[0mxid.payload     [0m | [0mdynamic array[0m | [0m          32[0m | [0mbyte array with the size of gtrid_length + bqual_length (max 128)  [0m
[0mresource.id     [0m | [0muint32       [0m | [0m           4[0m | [0mRM id of the resource - has to correlate with the request          [0m
[0mresource.state  [0m | [0muint32       [0m | [0m           4[0m | [0mThe state of the operation - If successful XA_OK ( 0)              [0m

### Resource commit


#### message::transaction::resource::commit::Request

Sent to and received from other domains when one domain wants to commit an already prepared transaction.

message type: **5203**

role name        | network type  | network size | description                                                        
---------------- | ------------- | ------------ | -------------------------------------------------------------------
[0mexecution       [0m | [0mfixed array  [0m | [0m          16[0m | [0muuid of the current execution path                                 [0m
[0mxid.format      [0m | [0muint64       [0m | [0m           8[0m | [0mxid format type. if 0 no more information of the xid is transported[0m
[0mxid.gtrid_length[0m | [0muint64       [0m | [0m           8[0m | [0mlength of the transaction gtrid part                               [0m
[0mxid.bqual_length[0m | [0muint64       [0m | [0m           8[0m | [0mlength of the transaction branch part                              [0m
[0mxid.payload     [0m | [0mdynamic array[0m | [0m          32[0m | [0mbyte array with the size of gtrid_length + bqual_length (max 128)  [0m
[0mresource.id     [0m | [0muint32       [0m | [0m           4[0m | [0mRM id of the resource - has to correlate with the reply            [0m
[0mflags           [0m | [0muint64       [0m | [0m           8[0m | [0mXA flags to be forward to the resource                             [0m

#### message::transaction::resource::commit::Reply

Reply to a commit request. 

message type: **5204**

role name        | network type  | network size | description                                                        
---------------- | ------------- | ------------ | -------------------------------------------------------------------
[0mexecution       [0m | [0mfixed array  [0m | [0m          16[0m | [0muuid of the current execution path                                 [0m
[0mxid.format      [0m | [0muint64       [0m | [0m           8[0m | [0mxid format type. if 0 no more information of the xid is transported[0m
[0mxid.gtrid_length[0m | [0muint64       [0m | [0m           8[0m | [0mlength of the transaction gtrid part                               [0m
[0mxid.bqual_length[0m | [0muint64       [0m | [0m           8[0m | [0mlength of the transaction branch part                              [0m
[0mxid.payload     [0m | [0mdynamic array[0m | [0m          32[0m | [0mbyte array with the size of gtrid_length + bqual_length (max 128)  [0m
[0mresource.id     [0m | [0muint32       [0m | [0m           4[0m | [0mRM id of the resource - has to correlate with the request          [0m
[0mresource.state  [0m | [0muint32       [0m | [0m           4[0m | [0mThe state of the operation - If successful XA_OK ( 0)              [0m

### Resource rollback


#### message::transaction::resource::rollback::Request

Sent to and received from other domains when one domain wants to rollback an already prepared transaction.
That is, when one or more resources has failed to prepare.

message type: **5205**

role name        | network type  | network size | description                                                        
---------------- | ------------- | ------------ | -------------------------------------------------------------------
[0mexecution       [0m | [0mfixed array  [0m | [0m          16[0m | [0muuid of the current execution path                                 [0m
[0mxid.format      [0m | [0muint64       [0m | [0m           8[0m | [0mxid format type. if 0 no more information of the xid is transported[0m
[0mxid.gtrid_length[0m | [0muint64       [0m | [0m           8[0m | [0mlength of the transaction gtrid part                               [0m
[0mxid.bqual_length[0m | [0muint64       [0m | [0m           8[0m | [0mlength of the transaction branch part                              [0m
[0mxid.payload     [0m | [0mdynamic array[0m | [0m          32[0m | [0mbyte array with the size of gtrid_length + bqual_length (max 128)  [0m
[0mresource.id     [0m | [0muint32       [0m | [0m           4[0m | [0mRM id of the resource - has to correlate with the reply            [0m
[0mflags           [0m | [0muint64       [0m | [0m           8[0m | [0mXA flags to be forward to the resource                             [0m

#### message::transaction::resource::rollback::Reply

Reply to a rollback request. 

message type: **5206**

role name        | network type  | network size | description                                                        
---------------- | ------------- | ------------ | -------------------------------------------------------------------
[0mexecution       [0m | [0mfixed array  [0m | [0m          16[0m | [0muuid of the current execution path                                 [0m
[0mxid.format      [0m | [0muint64       [0m | [0m           8[0m | [0mxid format type. if 0 no more information of the xid is transported[0m
[0mxid.gtrid_length[0m | [0muint64       [0m | [0m           8[0m | [0mlength of the transaction gtrid part                               [0m
[0mxid.bqual_length[0m | [0muint64       [0m | [0m           8[0m | [0mlength of the transaction branch part                              [0m
[0mxid.payload     [0m | [0mdynamic array[0m | [0m          32[0m | [0mbyte array with the size of gtrid_length + bqual_length (max 128)  [0m
[0mresource.id     [0m | [0muint32       [0m | [0m           4[0m | [0mRM id of the resource - has to correlate with the request          [0m
[0mresource.state  [0m | [0muint32       [0m | [0m           4[0m | [0mThe state of the operation - If successful XA_OK ( 0)              [0m

## queue messages

### enqueue 


#### message::queue::enqueue::Request

Represent enqueue request.

message type: **6100**

role name                         | network type  | network size | description                                                        
--------------------------------- | ------------- | ------------ | -------------------------------------------------------------------
[0mexecution                        [0m | [0mfixed array  [0m | [0m          16[0m | [0muuid of the current execution path                                 [0m
[0mname.size                        [0m | [0muint64       [0m | [0m           8[0m | [0msize of queue name                                                 [0m
[0mname.data                        [0m | [0mdynamic array[0m | [0m         128[0m | [0mdata of queue name                                                 [0m
[0mtransaction.trid.xid.format      [0m | [0muint64       [0m | [0m           8[0m | [0mxid format type. if 0 no more information of the xid is transported[0m
[0mtransaction.trid.xid.gtrid_length[0m | [0muint64       [0m | [0m           8[0m | [0mlength of the transaction gtrid part                               [0m
[0mtransaction.trid.xid.bqual_length[0m | [0muint64       [0m | [0m           8[0m | [0mlength of the transaction branch part                              [0m
[0mtransaction.trid.xid.payload     [0m | [0mdynamic array[0m | [0m          32[0m | [0mbyte array with the size of gtrid_length + bqual_length (max 128)  [0m
[0mmessage.id                       [0m | [0mfixed array  [0m | [0m          16[0m | [0mid of the message                                                  [0m
[0mmessage.properties.size          [0m | [0muint64       [0m | [0m           8[0m | [0mlength of message properties                                       [0m
[0mmessage.properties.data          [0m | [0mdynamic array[0m | [0m           0[0m | [0mdata of message properties                                         [0m
[0mmessage.reply.size               [0m | [0muint64       [0m | [0m           8[0m | [0mlength of the reply queue                                          [0m
[0mmessage.reply.data               [0m | [0mdynamic array[0m | [0m           0[0m | [0mdata of reply queue                                                [0m
[0mmessage.available                [0m | [0muint64       [0m | [0m           8[0m | [0mwhen the message is available for dequeue (us since epoc)          [0m
[0mmessage.type.size                [0m | [0muint64       [0m | [0m           8[0m | [0mlength of the type string                                          [0m
[0mmessage.type.data                [0m | [0mdynamic array[0m | [0m           0[0m | [0mdata of the type string                                            [0m
[0mmessage.payload.size             [0m | [0muint64       [0m | [0m           8[0m | [0msize of the payload                                                [0m
[0mmessage.payload.data             [0m | [0mdynamic array[0m | [0m        1024[0m | [0mdata of the payload                                                [0m

#### message::queue::enqueue::Reply

Represent enqueue reply.

message type: **6101**

role name | network type | network size | description                       
--------- | ------------ | ------------ | ----------------------------------
[0mexecution[0m | [0mfixed array [0m | [0m          16[0m | [0muuid of the current execution path[0m
[0mid       [0m | [0mfixed array [0m | [0m          16[0m | [0mid of the enqueued message        [0m

### dequeue 

#### message::queue::dequeue::Request

Represent dequeue request.

message type: **6200**

role name                         | network type  | network size | description                                                        
--------------------------------- | ------------- | ------------ | -------------------------------------------------------------------
[0mexecution                        [0m | [0mfixed array  [0m | [0m          16[0m | [0muuid of the current execution path                                 [0m
[0mname.size                        [0m | [0muint64       [0m | [0m           8[0m | [0msize of the queue name                                             [0m
[0mname.data                        [0m | [0mdynamic array[0m | [0m         128[0m | [0mdata of the queue name                                             [0m
[0mtransaction.trid.xid.format      [0m | [0muint64       [0m | [0m           8[0m | [0mxid format type. if 0 no more information of the xid is transported[0m
[0mtransaction.trid.xid.gtrid_length[0m | [0muint64       [0m | [0m           8[0m | [0mlength of the transaction gtrid part                               [0m
[0mtransaction.trid.xid.bqual_length[0m | [0muint64       [0m | [0m           8[0m | [0mlength of the transaction branch part                              [0m
[0mtransaction.trid.xid.payload     [0m | [0mdynamic array[0m | [0m          32[0m | [0mbyte array with the size of gtrid_length + bqual_length (max 128)  [0m
[0mselector.properties.size         [0m | [0muint64       [0m | [0m           8[0m | [0msize of the selector properties (ignored if empty)                 [0m
[0mselector.properties.data         [0m | [0mdynamic array[0m | [0m           0[0m | [0mdata of the selector properties (ignored if empty)                 [0m
[0mselector.id                      [0m | [0mfixed array  [0m | [0m          16[0m | [0mselector uuid (ignored if 'empty'                                  [0m
[0mblock                            [0m | [0muint8        [0m | [0m           1[0m | [0mdictates if this is a blocking call or not                         [0m

#### message::queue::dequeue::Reply

Represent dequeue reply.

message type: **6201**

role name                       | network type  | network size | description                                               
------------------------------- | ------------- | ------------ | ----------------------------------------------------------
[0mexecution                      [0m | [0mfixed array  [0m | [0m          16[0m | [0muuid of the current execution path                        [0m
[0mmessage.size                   [0m | [0muint64       [0m | [0m           8[0m | [0mnumber of messages dequeued                               [0m
[0mmessage.element.id             [0m | [0mfixed array  [0m | [0m          16[0m | [0mid of the message                                         [0m
[0mmessage.element.properties.size[0m | [0muint64       [0m | [0m           8[0m | [0mlength of message properties                              [0m
[0mmessage.element.properties.data[0m | [0mdynamic array[0m | [0m         128[0m | [0mdata of message properties                                [0m
[0mmessage.element.reply.size     [0m | [0muint64       [0m | [0m           8[0m | [0mlength of the reply queue                                 [0m
[0mmessage.element.reply.data     [0m | [0mdynamic array[0m | [0m         128[0m | [0mdata of reply queue                                       [0m
[0mmessage.element.available      [0m | [0muint64       [0m | [0m           8[0m | [0mwhen the message was available for dequeue (us since epoc)[0m
[0mmessage.element.type.size      [0m | [0muint64       [0m | [0m           8[0m | [0mlength of the type string                                 [0m
[0mmessage.element.type.data      [0m | [0mdynamic array[0m | [0m         128[0m | [0mdata of the type string                                   [0m
[0mmessage.element.payload.size   [0m | [0muint64       [0m | [0m           8[0m | [0msize of the payload                                       [0m
[0mmessage.element.payload.data   [0m | [0mdynamic array[0m | [0m        1024[0m | [0mdata of the payload                                       [0m
[0mmessage.element.redelivered    [0m | [0muint64       [0m | [0m           8[0m | [0mhow many times the message has been redelivered           [0m
[0mmessage.element.timestamp      [0m | [0muint64       [0m | [0m           8[0m | [0mwhen the message was enqueued (us since epoc)             [0m

## conversation messages

### connect 


#### message::conversation::connect::Request

Sent to establish a conversation

message type: **3200**

role name                         | network type  | network size | description                                                        
--------------------------------- | ------------- | ------------ | -------------------------------------------------------------------
[0mexecution                        [0m | [0mfixed array  [0m | [0m          16[0m | [0muuid of the current execution path                                 [0m
[0mservice.name.size                [0m | [0muint64       [0m | [0m           8[0m | [0msize of the service name                                           [0m
[0mservice.name.data                [0m | [0mdynamic array[0m | [0m         128[0m | [0mdata of the service name                                           [0m
[0mservice.name.timeout             [0m | [0muint64       [0m | [0m           8[0m | [0mtimeout (in us                                                     [0m
[0mparent.size                      [0m | [0muint64       [0m | [0m           8[0m | [0msize of the parent service name (the caller)                       [0m
[0mparent.data                      [0m | [0mdynamic array[0m | [0m         128[0m | [0mdata of the parent service name (the caller)                       [0m
[0mtransaction.trid.xid.format      [0m | [0muint64       [0m | [0m           8[0m | [0mxid format type. if 0 no more information of the xid is transported[0m
[0mtransaction.trid.xid.gtrid_length[0m | [0muint64       [0m | [0m           8[0m | [0mlength of the transaction gtrid part                               [0m
[0mtransaction.trid.xid.bqual_length[0m | [0muint64       [0m | [0m           8[0m | [0mlength of the transaction branch part                              [0m
[0mtransaction.trid.xid.payload     [0m | [0mdynamic array[0m | [0m          32[0m | [0mbyte array with the size of gtrid_length + bqual_length (max 128)  [0m
[0mflags                            [0m | [0muint64       [0m | [0m           8[0m | [0mxatmi flag                                                         [0m
[0mrecording.nodes.size             [0m | [0muint64       [0m | [0m           8[0m | [0msize of the recording of 'passed nodes'                            [0m
[0mrecording.nodes.element.address  [0m | [0muint64       [0m | [0m           8[0m | [0m'address' of a node'                                               [0m
[0mbuffer.type.size                 [0m | [0muint64       [0m | [0m           8[0m | [0mbuffer type name size                                              [0m
[0mbuffer.type.data                 [0m | [0mdynamic array[0m | [0m          25[0m | [0mbyte array with buffer type in the form 'type/subtype'             [0m
[0mbuffer.payload.size              [0m | [0muint64       [0m | [0m           8[0m | [0mbuffer payload size (could be very big)                            [0m
[0mbuffer.payload.data              [0m | [0mdynamic array[0m | [0m        1024[0m | [0mbuffer payload data (with the size of buffer.payload.size)         [0m

#### message::conversation::connect::Reply

Reply for a conversation

message type: **3201**

role name                       | network type | network size | description                            
------------------------------- | ------------ | ------------ | ---------------------------------------
[0mexecution                      [0m | [0mfixed array [0m | [0m          16[0m | [0muuid of the current execution path     [0m
[0mroute.nodes.size               [0m | [0muint64      [0m | [0m           8[0m | [0msize of the established route          [0m
[0mroute.nodes.element.address    [0m | [0muint64      [0m | [0m           8[0m | [0m'address' of a 'node' in the route     [0m
[0mrecording.nodes.size           [0m | [0muint64      [0m | [0m           8[0m | [0msize of the recording of 'passed nodes'[0m
[0mrecording.nodes.element.address[0m | [0muint64      [0m | [0m           8[0m | [0m'address' of a node'                   [0m
[0mstatus                         [0m | [0muint32      [0m | [0m           4[0m | [0mstatus of the connection               [0m

### send

#### message::conversation::Send

Represent a message sent 'over' an established connection

message type: **3202**

role name                   | network type  | network size | description                                               
--------------------------- | ------------- | ------------ | ----------------------------------------------------------
[0mexecution                  [0m | [0mfixed array  [0m | [0m          16[0m | [0muuid of the current execution path                        [0m
[0mroute.nodes.size           [0m | [0muint64       [0m | [0m           8[0m | [0msize of the established route                             [0m
[0mroute.nodes.element.address[0m | [0muint64       [0m | [0m           8[0m | [0m'address' of a 'node' in the route                        [0m
[0mevents                     [0m | [0muint64       [0m | [0m           8[0m | [0mevents                                                    [0m
[0mstatus                     [0m | [0muint32       [0m | [0m           4[0m | [0mstatus of the connection                                  [0m
[0mbuffer.type.size           [0m | [0muint64       [0m | [0m           8[0m | [0mbuffer type name size                                     [0m
[0mbuffer.type.data           [0m | [0mdynamic array[0m | [0m          25[0m | [0mbyte array with buffer type in the form 'type/subtype'    [0m
[0mbuffer.payload.size        [0m | [0muint64       [0m | [0m           8[0m | [0mbuffer payload size (could be very big)                   [0m
[0mbuffer.payload.data        [0m | [0mdynamic array[0m | [0m        1024[0m | [0mbuffer payload data (with the size of buffer.payload.size)[0m

### disconnect

#### message::conversation::Disconnect

Sent to abruptly disconnect the conversation

message type: **3203**

role name                   | network type | network size | description                       
--------------------------- | ------------ | ------------ | ----------------------------------
[0mexecution                  [0m | [0mfixed array [0m | [0m          16[0m | [0muuid of the current execution path[0m
[0mroute.nodes.size           [0m | [0muint64      [0m | [0m           8[0m | [0msize of the established route     [0m
[0mroute.nodes.element.address[0m | [0muint64      [0m | [0m           8[0m | [0m'address' of a 'node' in the route[0m
[0mevents                     [0m | [0muint64      [0m | [0m           8[0m | [0mevents                            [0m
