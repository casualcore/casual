
# casual domain protocol

Defines what messages is sent between domains and exactly what they contain. 


## common::communication::tcp::message::Transport 

This "message" holds all other messages in it's payload, hence act as a placeholder.

message.type is used to dispatch to handler for that particular message, and knows how to (un)marshal and act on the message.

It's probably a good idea to read only the header (including message.type) only, to see how much more one has to read to get
the complete transport message.

1..* transport messages construct one logical complete message



| role name     | native type | native size | network type | network size  | comments
|---------------|-------------|-------------|--------------|---------------|---------
| message.type | long | 8 | uint64 | 8 | type of the message that the payload contains
| message.header.correlation | fixed array | 16 | fixed array | 16 | correlation id of the message
| message.header.offset | uint64 | 8 | uint64 | 8 | which offset this transport message represent of the complete message
| message.header.count | uint64 | 8 | uint64 | 8 | size of payload in this transport message
| message.header.complete_size | uint64 | 8 | uint64 | 8 | size of the logical complete message
| message.payload | byte array | 16344 | byte array | 16344 | actual structured (part of) message is serialized here

## Discovery messages

### domain discovery 


#### message::interdomain::domain::discovery::Request

Sent to and received from other domains when one domain wants discover information abut the other.

message type: **8001**

role name             | native type   | native size | network type  | network size | description                                                  
--------------------- | ------------- | ----------- | ------------- | ------------ | -------------------------------------------------------------
execution             | fixed array   |          16 | fixed array   |           16 | uuid of the current execution path                           
domain.id             | fixed array   |          16 | fixed array   |           16 | uuid of the caller domain                                    
domain.name.size      | size          |           8 | uint64        |            8 | size of the caller domain name                               
domain.name.data      | dynamic array |           0 | dynamic array |            0 | dynamic byte array with the caller domain name               
services.size         | size          |           8 | uint64        |            8 | number of requested services to follow (an array of services)
services.element.size | size          |           8 | uint64        |            8 | size of the current service name                             
services.element.data | dynamic array |         128 | dynamic array |          128 | dynamic byte array of the current service name               

#### message::interdomain::domain::discovery::Reply

Sent to and received from other domains when one domain wants discover information abut the other.

message type: **8002**

role name                    | native type   | native size | network type  | network size | description                                                     
---------------------------- | ------------- | ----------- | ------------- | ------------ | ----------------------------------------------------------------
execution                    | fixed array   |          16 | fixed array   |           16 | uuid of the current execution path                              
domain.id                    | fixed array   |          16 | fixed array   |           16 | uuid of the caller domain                                       
domain.name.size             | size          |           8 | uint64        |            8 | size of the caller domain name                                  
domain.name.data             | dynamic array |           0 | dynamic array |            0 | dynamic byte array with the caller domain name                  
services.size                | size          |           8 | uint64        |            8 | number of services to follow (an array of services)             
services.element.name.size   | size          |           8 | uint64        |            8 | size of the current service name                                
services.element.name.data   | dynamic array |         128 | dynamic array |          128 | dynamic byte array of the current service name                  
services.element.type        | uint64        |           8 | uint64        |            8 | service type                                                    
services.element.timeout     | int64         |           8 | uint64        |            8 | service timeout                                                 
services.element.transaction | uint16        |           2 | uint16        |            2 | service transaction mode (auto, atomic, join, none)             
services.element.hops        | size          |           8 | uint64        |            8 | number of domain hops to the service (local services has 0 hops)

## Service messages

### Service call 


#### message::interdomain::service::call::receive::Request

Sent to and received from other domains when one domain wants call a service in the other domain

message type: **8003**

role name                | native type   | native size | network type  | network size | description                                                        
------------------------ | ------------- | ----------- | ------------- | ------------ | -------------------------------------------------------------------
execution                | fixed array   |          16 | fixed array   |           16 | uuid of the current execution path                                 
call.descriptor          | int           |           4 | uint64        |            8 | descriptor of the call                                             
service.name.size        | size          |           8 | uint64        |            8 | service name size                                                  
service.name.data        | dynamic array |         128 | dynamic array |          128 | byte array with service name                                       
service.timeout          | int64         |           8 | uint64        |            8 | timeout of the service in use                                      
parent.name.size         | size          |           8 | uint64        |            8 | parent service name size                                           
parent.name.data         | dynamic array |         128 | dynamic array |          128 | byte array with parent service name                                
xid.format               | long          |           8 | uint64        |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length         | long          |           8 | uint64        |            8 | length of the transaction gtrid part                               
xid.bqual_length         | long          |           8 | uint64        |            8 | length of the transaction branch part                              
xid.payload              | dynamic array |          32 | dynamic array |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
flags                    | int64         |           8 | uint64        |            8 | XATMI flags sent to the service                                    
buffer.type.name.size    | size          |           8 | uint64        |            8 | buffer type name size (max 8)                                      
buffer.type.name.data    | dynamic array |           8 | dynamic array |            8 | byte array with buffer type name                                   
buffer.type.subname.size | size          |           8 | uint64        |            8 | buffer type subname size (max 16)                                  
buffer.type.subname.data | dynamic array |          16 | dynamic array |           16 | byte array with buffer type subname                                
buffer.payload.size      | size          |           8 | uint64        |            8 | buffer payload size (could be very big)                            
buffer.payload.data      | dynamic array |         128 | dynamic array |          128 | buffer payload data (with the size of buffer.payload.size)         

#### message::interdomain::service::call::receive::Reply

Reply to call request

message type: **8004**

role name                         | native type   | native size | network type  | network size | description                                                                   
--------------------------------- | ------------- | ----------- | ------------- | ------------ | ------------------------------------------------------------------------------
execution                         | fixed array   |          16 | fixed array   |           16 | uuid of the current execution path                                            
call.descriptor                   | int           |           4 | uint64        |            8 | descriptor of the call                                                        
call.error                        | int           |           4 | uint64        |            8 | XATMI error code, if any.                                                     
call.code                         | long          |           8 | uint64        |            8 | XATMI user supplied code                                                      
transaction.trid.xid.format       | long          |           8 | uint64        |            8 | xid format type. if 0 no more information of the xid is transported           
transaction.trid.xid.gtrid_length | long          |           8 | uint64        |            8 | length of the transactino gtrid part                                          
transaction.trid.xid.bqual_length | long          |           8 | uint64        |            8 | length of the transaction branch part                                         
transaction.trid.xid.payload      | dynamic array |          32 | dynamic array |           32 | byte array with the size of gtrid_length + bqual_length (max 128)             
transaction.state                 | int64         |           8 | uint64        |            8 | state of the transaction TX_ACTIVE, TX_TIMEOUT_ROLLBACK_ONLY, TX_ROLLBACK_ONLY
buffer.type.name.size             | size          |           8 | uint64        |            8 | buffer type name size (max 8)                                                 
buffer.type.name.data             | dynamic array |           8 | dynamic array |            8 | byte array with buffer type name                                              
buffer.type.subname.size          | size          |           8 | uint64        |            8 | buffer type subname size (max 16)                                             
buffer.type.subname.data          | dynamic array |          16 | dynamic array |           16 | byte array with buffer type subname                                           
buffer.payload.size               | size          |           8 | uint64        |            8 | buffer payload size (could be very big)                                       
buffer.payload.data               | dynamic array |         128 | dynamic array |          128 | buffer payload data (with the size of buffer.payload.size)                    

## Transaction messages

### Resource prepare


#### message::interdomain::transaction::resource::receive::prepare::Request

Sent to and received from other domains when one domain wants to prepare a transaction. 

message type: **8005**

role name        | native type   | native size | network type  | network size | description                                                        
---------------- | ------------- | ----------- | ------------- | ------------ | -------------------------------------------------------------------
execution        | fixed array   |          16 | fixed array   |           16 | uuid of the current execution path                                 
xid.format       | long          |           8 | uint64        |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length | long          |           8 | uint64        |            8 | length of the transaction gtrid part                               
xid.bqual_length | long          |           8 | uint64        |            8 | length of the transaction branch part                              
xid.payload      | dynamic array |          32 | dynamic array |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
resource.id      | int           |           4 | uint64        |            8 | RM id of the resource - has to correlate with the reply            
flags            | int           |           4 | uint64        |            8 | XA flags to be forward to the resource                             

#### message::interdomain::transaction::resource::receive::prepare::Reply

Sent to and received from other domains when one domain wants to prepare a transaction. 

message type: **8006**

role name        | native type   | native size | network type  | network size | description                                                        
---------------- | ------------- | ----------- | ------------- | ------------ | -------------------------------------------------------------------
execution        | fixed array   |          16 | fixed array   |           16 | uuid of the current execution path                                 
xid.format       | long          |           8 | uint64        |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length | long          |           8 | uint64        |            8 | length of the transaction gtrid part                               
xid.bqual_length | long          |           8 | uint64        |            8 | length of the transaction branch part                              
xid.payload      | dynamic array |          32 | dynamic array |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
resource.id      | int           |           4 | uint64        |            8 | RM id of the resource - has to correlate with the request          
resource.state   | int           |           4 | uint64        |            8 | The state of the operation - If successful XA_OK ( 0)              

### Resource commit


#### message::interdomain::transaction::resource::receive::commit::Request

Sent to and received from other domains when one domain wants to commit an already prepared transaction.

message type: **8007**

role name        | native type   | native size | network type  | network size | description                                                        
---------------- | ------------- | ----------- | ------------- | ------------ | -------------------------------------------------------------------
execution        | fixed array   |          16 | fixed array   |           16 | uuid of the current execution path                                 
xid.format       | long          |           8 | uint64        |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length | long          |           8 | uint64        |            8 | length of the transaction gtrid part                               
xid.bqual_length | long          |           8 | uint64        |            8 | length of the transaction branch part                              
xid.payload      | dynamic array |          32 | dynamic array |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
resource.id      | int           |           4 | uint64        |            8 | RM id of the resource - has to correlate with the reply            
flags            | int           |           4 | uint64        |            8 | XA flags to be forward to the resource                             

#### message::interdomain::transaction::resource::receive::commit::Reply

Reply to a commit request. 

message type: **8008**

role name        | native type   | native size | network type  | network size | description                                                        
---------------- | ------------- | ----------- | ------------- | ------------ | -------------------------------------------------------------------
execution        | fixed array   |          16 | fixed array   |           16 | uuid of the current execution path                                 
xid.format       | long          |           8 | uint64        |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length | long          |           8 | uint64        |            8 | length of the transaction gtrid part                               
xid.bqual_length | long          |           8 | uint64        |            8 | length of the transaction branch part                              
xid.payload      | dynamic array |          32 | dynamic array |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
resource.id      | int           |           4 | uint64        |            8 | RM id of the resource - has to correlate with the request          
resource.state   | int           |           4 | uint64        |            8 | The state of the operation - If successful XA_OK ( 0)              

### Resource rollback


#### message::interdomain::transaction::resource::receive::rollback::Request

Sent to and received from other domains when one domain wants to rollback an already prepared transaction.
That is, when one or more resources has failed to prepare.

message type: **8009**

role name        | native type   | native size | network type  | network size | description                                                        
---------------- | ------------- | ----------- | ------------- | ------------ | -------------------------------------------------------------------
execution        | fixed array   |          16 | fixed array   |           16 | uuid of the current execution path                                 
xid.format       | long          |           8 | uint64        |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length | long          |           8 | uint64        |            8 | length of the transaction gtrid part                               
xid.bqual_length | long          |           8 | uint64        |            8 | length of the transaction branch part                              
xid.payload      | dynamic array |          32 | dynamic array |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
resource.id      | int           |           4 | uint64        |            8 | RM id of the resource - has to correlate with the reply            
flags            | int           |           4 | uint64        |            8 | XA flags to be forward to the resource                             

#### message::interdomain::transaction::resource::receive::rollback::Reply

Reply to a rollback request. 

message type: **8010**

role name        | native type   | native size | network type  | network size | description                                                        
---------------- | ------------- | ----------- | ------------- | ------------ | -------------------------------------------------------------------
execution        | fixed array   |          16 | fixed array   |           16 | uuid of the current execution path                                 
xid.format       | long          |           8 | uint64        |            8 | xid format type. if 0 no more information of the xid is transported
xid.gtrid_length | long          |           8 | uint64        |            8 | length of the transaction gtrid part                               
xid.bqual_length | long          |           8 | uint64        |            8 | length of the transaction branch part                              
xid.payload      | dynamic array |          32 | dynamic array |           32 | byte array with the size of gtrid_length + bqual_length (max 128)  
resource.id      | int           |           4 | uint64        |            8 | RM id of the resource - has to correlate with the request          
resource.state   | int           |           4 | uint64        |            8 | The state of the operation - If successful XA_OK ( 0)              
