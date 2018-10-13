
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
header.type        | uint64       |            8 | type of the message that the payload contains
header.correlation | fixed array  |           16 | correlation id of the message                
header.size        | uint64       |            8 | the size of the payload that follows         

## domain connect messages

messages that is used to set up a connection


### common::message::gateway::domain::connect::Request
   
Connection requests from another domain that wants to connect
   
   message type: **7200**

role name                 | network type  | network size | description                                            
------------------------- | ------------- | ------------ | -------------------------------------------------------
execution                 | fixed array   |           16 | uuid of the current execution path                     
domain.id                 | fixed array   |           16 | uuid of the outbound domain                            
domain.name.size          | uint64        |            8 | size of the outbound domain name                       
domain.name.data          | dynamic array |            8 | dynamic byte array with the outbound domain name       
protocol.versions.size    | uint64        |            8 | number of protocol versions outbound domain can 'speak'
protocol.versions.element | uint64        |            8 | a protocol version                                     

### common::message::gateway::domain::connect::Reply
   
Connection reply
   
   message type: **7201**

role name        | network type  | network size | description                                                       
---------------- | ------------- | ------------ | ------------------------------------------------------------------
execution        | fixed array   |           16 | uuid of the current execution path                                
domain.id        | fixed array   |           16 | uuid of the inbound domain                                        
domain.name.size | uint64        |            8 | size of the inbound domain name                                   
domain.name.data | dynamic array |            8 | dynamic byte array with the inbound domain name                   
protocol.version | uint64        |            8 | the chosen protocol version to use, or invalid (0) if incompatible

## Discovery messages

### domain discovery 


#### message::gateway::domain::discover::Request

Sent to and received from other domains when one domain wants discover information abut the other.

message type: **7300**

role name             | network type  | network size | description                                                  
--------------------- | ------------- | ------------ | -------------------------------------------------------------
execution             | fixed array   |           16 | uuid of the current execution path                           
domain.id             | fixed array   |           16 | uuid of the caller domain                                    
domain.name.size      | uint64        |            8 | size of the caller domain name                               
domain.name.data      | dynamic array |            8 | dynamic byte array with the caller domain name               
services.size         | uint64        |            8 | number of requested services to follow (an array of services)
services.element.size | uint64        |            8 | size of the current service name                             
services.element.data | dynamic array |          128 | dynamic byte array of the current service name               
queues.size           | uint64        |            8 | number of requested queues to follow (an array of queues)    
queues.element.size   | uint64        |            8 | size of the current queue name                               
queues.element.data   | dynamic array |          128 | dynamic byte array of the current queue name                 

#### message::gateway::domain::discover::Reply

Sent to and received from other domains when one domain wants discover information abut the other.

message type: **7301**

role name                      | network type  | network size | description                                                     
------------------------------ | ------------- | ------------ | ----------------------------------------------------------------
execution                      | fixed array   |           16 | uuid of the current execution path                              
domain.id                      | fixed array   |           16 | uuid of the caller domain                                       
domain.name.size               | uint64        |            8 | size of the caller domain name                                  
domain.name.data               | dynamic array |            0 | dynamic byte array with the caller domain name                  
services.size                  | uint64        |            8 | number of services to follow (an array of services)             
services.element.name.size     | uint64        |            8 | size of the current service name                                
services.element.name.data     | dynamic array |          128 | dynamic byte array of the current service name                  
services.element.category.size | uint64        |            8 | size of the current service category                            
services.element.category.data | dynamic array |            0 | dynamic byte array of the current service category              
services.element.transaction   | uint16        |            2 | service transaction mode (auto, atomic, join, none)             
services.element.timeout       | uint64        |            8 | service timeout                                                 
services.element.hops          | uint64        |            8 | number of domain hops to the service (local services has 0 hops)
queues.size                    | uint64        |            8 | number of requested queues to follow (an array of queues)       
queues.element.size            | uint64        |            8 | size of the current queue name                                  
queues.element.data            | dynamic array |          128 | dynamic byte array of the current queue name                    
queues.element.retries         | uint64        |            8 | how many 'retries' the queue has                                

## Service messages

### Service call 


#### message::service::call::Request

Sent to and received from other domains when one domain wants call a service in the other domain

message type: **3100**

role name           | network type  | network size | description                                                        
------------------- | ------------- | ------------ | -------------------------------------------------------------------
execution           | fixed array   |           16 | uuid of the current execution path                                 
service.name.size   | uint64        |            8 | service name size                                                  
service.name.data   | dynamic array |          128 | byte array with service name                                       
service.timeout     | uint64        |            8 | timeout of the service in use (in microseconds)                    
parent.name.size    | uint64        |            8 | parent service name size                                           
parent.name.data    | dynamic array |          128 | byte array with parent service name                                
xid.format          | uint64        |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length    | uint64        |            8 | length of the transaction gtrid part                               
xid.bqual_length    | uint64        |            8 | length of the transaction branch part                              
xid.payload         | dynamic array |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
flags               | uint64        |            8 | XATMI flags sent to the service                                    
buffer.type.size    | uint64        |            8 | buffer type name size                                              
buffer.type.data    | dynamic array |           25 | byte array with buffer type in the form 'type/subtype'             
buffer.payload.size | uint64        |            8 | buffer payload size (could be very big)                            
buffer.payload.data | dynamic array |         1024 | buffer payload data (with the size of buffer.payload.size)         

#### message::service::call::Reply

Reply to call request

message type: **3101**

role name                         | network type  | network size | description                                                                   
--------------------------------- | ------------- | ------------ | ------------------------------------------------------------------------------
execution                         | fixed array   |           16 | uuid of the current execution path                                            
call.status                       | uint32        |            4 | XATMI error code, if any.                                                     
call.code                         | uint64        |            8 | XATMI user supplied code                                                      
transaction.trid.xid.format       | uint64        |            8 | xid format type. if 0 no more information of the xid is transported           
transaction.trid.xid.gtrid_length | uint64        |            8 | length of the transaction gtrid part                                          
transaction.trid.xid.bqual_length | uint64        |            8 | length of the transaction branch part                                         
transaction.trid.xid.payload      | dynamic array |           32 | byte array with the size of gtrid_length + bqual_length (max 128)             
transaction.state                 | uint8         |            1 | state of the transaction TX_ACTIVE, TX_TIMEOUT_ROLLBACK_ONLY, TX_ROLLBACK_ONLY
buffer.type.size                  | uint64        |            8 | buffer type name size                                                         
buffer.type.data                  | dynamic array |           25 | byte array with buffer type in the form 'type/subtype'                        
buffer.payload.size               | uint64        |            8 | buffer payload size (could be very big)                                       
buffer.payload.data               | dynamic array |         1024 | buffer payload data (with the size of buffer.payload.size)                    

## Transaction messages

### Resource prepare


#### message::transaction::resource::prepare::Request

Sent to and received from other domains when one domain wants to prepare a transaction. 

message type: **5201**

role name        | network type  | network size | description                                                        
---------------- | ------------- | ------------ | -------------------------------------------------------------------
execution        | fixed array   |           16 | uuid of the current execution path                                 
xid.format       | uint64        |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length | uint64        |            8 | length of the transaction gtrid part                               
xid.bqual_length | uint64        |            8 | length of the transaction branch part                              
xid.payload      | dynamic array |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
resource.id      | uint32        |            4 | RM id of the resource - has to correlate with the reply            
flags            | uint64        |            8 | XA flags to be forward to the resource                             

#### message::transaction::resource::prepare::Reply

Sent to and received from other domains when one domain wants to prepare a transaction. 

message type: **5202**

role name        | network type  | network size | description                                                        
---------------- | ------------- | ------------ | -------------------------------------------------------------------
execution        | fixed array   |           16 | uuid of the current execution path                                 
xid.format       | uint64        |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length | uint64        |            8 | length of the transaction gtrid part                               
xid.bqual_length | uint64        |            8 | length of the transaction branch part                              
xid.payload      | dynamic array |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
resource.id      | uint32        |            4 | RM id of the resource - has to correlate with the request          
resource.state   | uint32        |            4 | The state of the operation - If successful XA_OK ( 0)              

### Resource commit


#### message::transaction::resource::commit::Request

Sent to and received from other domains when one domain wants to commit an already prepared transaction.

message type: **5203**

role name        | network type  | network size | description                                                        
---------------- | ------------- | ------------ | -------------------------------------------------------------------
execution        | fixed array   |           16 | uuid of the current execution path                                 
xid.format       | uint64        |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length | uint64        |            8 | length of the transaction gtrid part                               
xid.bqual_length | uint64        |            8 | length of the transaction branch part                              
xid.payload      | dynamic array |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
resource.id      | uint32        |            4 | RM id of the resource - has to correlate with the reply            
flags            | uint64        |            8 | XA flags to be forward to the resource                             

#### message::transaction::resource::commit::Reply

Reply to a commit request. 

message type: **5204**

role name        | network type  | network size | description                                                        
---------------- | ------------- | ------------ | -------------------------------------------------------------------
execution        | fixed array   |           16 | uuid of the current execution path                                 
xid.format       | uint64        |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length | uint64        |            8 | length of the transaction gtrid part                               
xid.bqual_length | uint64        |            8 | length of the transaction branch part                              
xid.payload      | dynamic array |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
resource.id      | uint32        |            4 | RM id of the resource - has to correlate with the request          
resource.state   | uint32        |            4 | The state of the operation - If successful XA_OK ( 0)              

### Resource rollback


#### message::transaction::resource::rollback::Request

Sent to and received from other domains when one domain wants to rollback an already prepared transaction.
That is, when one or more resources has failed to prepare.

message type: **5205**

role name        | network type  | network size | description                                                        
---------------- | ------------- | ------------ | -------------------------------------------------------------------
execution        | fixed array   |           16 | uuid of the current execution path                                 
xid.format       | uint64        |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length | uint64        |            8 | length of the transaction gtrid part                               
xid.bqual_length | uint64        |            8 | length of the transaction branch part                              
xid.payload      | dynamic array |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
resource.id      | uint32        |            4 | RM id of the resource - has to correlate with the reply            
flags            | uint64        |            8 | XA flags to be forward to the resource                             

#### message::transaction::resource::rollback::Reply

Reply to a rollback request. 

message type: **5206**

role name        | network type  | network size | description                                                        
---------------- | ------------- | ------------ | -------------------------------------------------------------------
execution        | fixed array   |           16 | uuid of the current execution path                                 
xid.format       | uint64        |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length | uint64        |            8 | length of the transaction gtrid part                               
xid.bqual_length | uint64        |            8 | length of the transaction branch part                              
xid.payload      | dynamic array |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
resource.id      | uint32        |            4 | RM id of the resource - has to correlate with the request          
resource.state   | uint32        |            4 | The state of the operation - If successful XA_OK ( 0)              

## queue messages

### enqueue 


#### message::queue::enqueue::Request

Represent enqueue request.

message type: **6100**

role name                         | network type  | network size | description                                                        
--------------------------------- | ------------- | ------------ | -------------------------------------------------------------------
execution                         | fixed array   |           16 | uuid of the current execution path                                 
name.size                         | uint64        |            8 | size of queue name                                                 
name.data                         | dynamic array |          128 | data of queue name                                                 
transaction.trid.xid.format       | uint64        |            8 | xid format type. if 0 no more information of the xid is transported
transaction.trid.xid.gtrid_length | uint64        |            8 | length of the transaction gtrid part                               
transaction.trid.xid.bqual_length | uint64        |            8 | length of the transaction branch part                              
transaction.trid.xid.payload      | dynamic array |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
message.id                        | fixed array   |           16 | id of the message                                                  
message.properties.size           | uint64        |            8 | length of message properties                                       
message.properties.data           | dynamic array |            0 | data of message properties                                         
message.reply.size                | uint64        |            8 | length of the reply queue                                          
message.reply.data                | dynamic array |            0 | data of reply queue                                                
message.available                 | uint64        |            8 | when the message is available for dequeue (us since epoc)          
message.type.size                 | uint64        |            8 | length of the type string                                          
message.type.data                 | dynamic array |            0 | data of the type string                                            
message.payload.size              | uint64        |            8 | size of the payload                                                
message.payload.data              | dynamic array |         1024 | data of the payload                                                

#### message::queue::enqueue::Reply

Represent enqueue reply.

message type: **6101**

role name | network type | network size | description                       
--------- | ------------ | ------------ | ----------------------------------
execution | fixed array  |           16 | uuid of the current execution path
id        | fixed array  |           16 | id of the enqueued message        

### dequeue 

#### message::queue::dequeue::Request

Represent dequeue request.

message type: **6200**

role name                         | network type  | network size | description                                                        
--------------------------------- | ------------- | ------------ | -------------------------------------------------------------------
execution                         | fixed array   |           16 | uuid of the current execution path                                 
name.size                         | uint64        |            8 | size of the queue name                                             
name.data                         | dynamic array |          128 | data of the queue name                                             
transaction.trid.xid.format       | uint64        |            8 | xid format type. if 0 no more information of the xid is transported
transaction.trid.xid.gtrid_length | uint64        |            8 | length of the transaction gtrid part                               
transaction.trid.xid.bqual_length | uint64        |            8 | length of the transaction branch part                              
transaction.trid.xid.payload      | dynamic array |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
selector.properties.size          | uint64        |            8 | size of the selector properties (ignored if empty)                 
selector.properties.data          | dynamic array |            0 | data of the selector properties (ignored if empty)                 
selector.id                       | fixed array   |           16 | selector uuid (ignored if 'empty'                                  
block                             | uint8         |            1 | dictates if this is a blocking call or not                         

#### message::queue::dequeue::Reply

Represent dequeue reply.

message type: **6201**

role name                       | network type  | network size | description                                               
------------------------------- | ------------- | ------------ | ----------------------------------------------------------
execution                       | fixed array   |           16 | uuid of the current execution path                        
message.size                    | uint64        |            8 | number of messages dequeued                               
message.element.id              | fixed array   |           16 | id of the message                                         
message.element.properties.size | uint64        |            8 | length of message properties                              
message.element.properties.data | dynamic array |          128 | data of message properties                                
message.element.reply.size      | uint64        |            8 | length of the reply queue                                 
message.element.reply.data      | dynamic array |          128 | data of reply queue                                       
message.element.available       | uint64        |            8 | when the message was available for dequeue (us since epoc)
message.element.type.size       | uint64        |            8 | length of the type string                                 
message.element.type.data       | dynamic array |          128 | data of the type string                                   
message.element.payload.size    | uint64        |            8 | size of the payload                                       
message.element.payload.data    | dynamic array |         1024 | data of the payload                                       
message.element.redelivered     | uint64        |            8 | how many times the message has been redelivered           
message.element.timestamp       | uint64        |            8 | when the message was enqueued (us since epoc)             

## conversation messages

### connect 


#### message::conversation::connect::Request

Sent to establish a conversation

message type: **3200**

role name                         | network type  | network size | description                                                        
--------------------------------- | ------------- | ------------ | -------------------------------------------------------------------
execution                         | fixed array   |           16 | uuid of the current execution path                                 
service.name.size                 | uint64        |            8 | size of the service name                                           
service.name.data                 | dynamic array |          128 | data of the service name                                           
service.name.timeout              | uint64        |            8 | timeout (in us                                                     
parent.size                       | uint64        |            8 | size of the parent service name (the caller)                       
parent.data                       | dynamic array |          128 | data of the parent service name (the caller)                       
transaction.trid.xid.format       | uint64        |            8 | xid format type. if 0 no more information of the xid is transported
transaction.trid.xid.gtrid_length | uint64        |            8 | length of the transaction gtrid part                               
transaction.trid.xid.bqual_length | uint64        |            8 | length of the transaction branch part                              
transaction.trid.xid.payload      | dynamic array |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
flags                             | uint64        |            8 | xatmi flag                                                         
recording.nodes.size              | uint64        |            8 | size of the recording of 'passed nodes'                            
recording.nodes.element.address   | fixed array   |           16 | 'address' of a node'                                               
buffer.type.size                  | uint64        |            8 | buffer type name size                                              
buffer.type.data                  | dynamic array |           25 | byte array with buffer type in the form 'type/subtype'             
buffer.payload.size               | uint64        |            8 | buffer payload size (could be very big)                            
buffer.payload.data               | dynamic array |         1024 | buffer payload data (with the size of buffer.payload.size)         

#### message::conversation::connect::Reply

Reply for a conversation

message type: **3201**

role name                       | network type | network size | description                            
------------------------------- | ------------ | ------------ | ---------------------------------------
execution                       | fixed array  |           16 | uuid of the current execution path     
route.nodes.size                | uint64       |            8 | size of the established route          
route.nodes.element.address     | fixed array  |           16 | 'address' of a 'node' in the route     
recording.nodes.size            | uint64       |            8 | size of the recording of 'passed nodes'
recording.nodes.element.address | fixed array  |           16 | 'address' of a node'                   
status                          | uint32       |            4 | status of the connection               

### send

#### message::conversation::Send

Represent a message sent 'over' an established connection

message type: **3202**

role name                   | network type  | network size | description                                               
--------------------------- | ------------- | ------------ | ----------------------------------------------------------
execution                   | fixed array   |           16 | uuid of the current execution path                        
route.nodes.size            | uint64        |            8 | size of the established route                             
route.nodes.element.address | fixed array   |           16 | 'address' of a 'node' in the route                        
events                      | uint64        |            8 | events                                                    
status                      | uint32        |            4 | status of the connection                                  
buffer.type.size            | uint64        |            8 | buffer type name size                                     
buffer.type.data            | dynamic array |           25 | byte array with buffer type in the form 'type/subtype'    
buffer.payload.size         | uint64        |            8 | buffer payload size (could be very big)                   
buffer.payload.data         | dynamic array |         1024 | buffer payload data (with the size of buffer.payload.size)

### disconnect

#### message::conversation::Disconnect

Sent to abruptly disconnect the conversation

message type: **3203**

role name                   | network type | network size | description                       
--------------------------- | ------------ | ------------ | ----------------------------------
execution                   | fixed array  |           16 | uuid of the current execution path
route.nodes.size            | uint64       |            8 | size of the established route     
route.nodes.element.address | fixed array  |           16 | 'address' of a 'node' in the route
events                      | uint64       |            8 | events                            
