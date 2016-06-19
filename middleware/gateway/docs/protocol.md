
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
| message.header.correlation | byte array | 16 | byte array | 16 | correlation id of the message
| message.header.offset | uint64 | 8 | uint64 | 8 | which offset this transport message represent of the complete message
| message.header.count | uint64 | 8 | uint64 | 8 | size of payload in this transport message
| message.header.complete_size | uint64 | 8 | uint64 | 8 | size of the logical complete message
| message.payload | byte array | 16344 | byte array | 16344 | actual structured (part of) message is serialized here

## Service messages

### Service call 


#### common::message::service::call::callee::Request

Sent to and received from other domains when one domain wants call a service in the other domain

message type: 3005

role name                | native type | native size | network type | network size | description                                                        
------------------------ | ----------- | ----------- | ------------ | ------------ | -------------------------------------------------------------------
execution                | byte array  | 16          | byte array   | 16           | uuid of the current execution path                                 
call.descriptor          | int         | 4           | uint64       | 8            | descriptor of the call                                             
sender.pid               | int         | 4           | uint64       | 8            | pid of the sender process                                          
sender.queue             | long        | 8           | uint64       | 8            | ipc queue id of the sender process                                 
service.name.size        | size_t      | 8           | uint64       | 8            | service name size                                                  
service.name.data        | byte array  | 128         | byte array   | 128          | byte array with service name                                       
service.type             | uint64      | 8           | uint64       | 8            | type of the service (plain xatmi, casual.sf, admin, ...)           
service.timeout          | int64       | 8           | uint64       | 8            | timeout of the service in us                                       
service.traffic.size     | size_t      | 8           | uint64       | 8            | number of trafic monitors ipc-queues to follow                     
service.traffic.queue    | long        | 8           | uint64       | 8            | for every service.traffic.size                                     
service.transaction      | uint64      | 8           | uint64       | 8            | type of transaction semantic in the service                        
parent.name.size         | size_t      | 8           | uint64       | 8            | parent service name size                                           
parent.name.data         | byte array  | 128         | byte array   | 128          | byte array with parent service name                                
trid.xid.format          | long        | 8           | uint64       | 8            | xid format type. if 0 no more information of the xid is transported
trid.owner.pid           | int         | 4           | uint64       | 8            | pid of owner of the transaction                                    
trid.owner.queue         | long        | 8           | uint64       | 8            | ipc queue of owner of the transaction                              
trid.xid.gtrid_length    | long        | 8           | uint64       | 8            | length of the transactino gtrid part                               
trid.xid.bqual_length    | long        | 8           | uint64       | 8            | length of the transaction branch part                              
trid.xid.payload         | byte array  | 32          | byte array   | 32           | byte array with the size of gtrid_length + bqual_length (max 128)  
flags                    | int64       | 8           | uint64       | 8            | XATMI flags sent to the service                                    
buffer.type.name.size    | size_t      | 8           | uint64       | 8            | buffer type name size                                              
buffer.type.name.data    | byte array  | 8           | byte array   | 8            | byte array with buffer type name                                   
buffer.type.subname.size | size_t      | 8           | uint64       | 8            | buffer type subname size                                           
buffer.type.subname.data | byte array  | 16          | byte array   | 16           | byte array with buffer type subname                                
buffer.payload.size      | size_t      | 8           | uint64       | 8            | buffer payload size (could be very big)                            
buffer.payload.data      | byte array  | 128         | byte array   | 128          | buffer payload data (with the size of buffer.payload.size)         

#### common::message::service::call::Reply

Reply to call request

message type: 3006

role name                         | native type | native size | network type | network size | description                                                                   
--------------------------------- | ----------- | ----------- | ------------ | ------------ | ------------------------------------------------------------------------------
execution                         | byte array  | 16          | byte array   | 16           | uuid of the current execution path                                            
call.descriptor                   | int         | 4           | uint64       | 8            | descriptor of the call                                                        
call.error                        | int         | 4           | uint64       | 8            | XATMI error code, if any.                                                     
call.code                         | long        | 8           | uint64       | 8            | XATMI user supplied code                                                      
transaction.trid.xid.format       | long        | 8           | uint64       | 8            | xid format type. if 0 no more information of the xid is transported           
transaction.trid.owner.pid        | int         | 4           | uint64       | 8            | pid of owner of the transaction                                               
transaction.trid.owner.queue      | long        | 8           | uint64       | 8            | ipc queue of owner of the transaction                                         
transaction.trid.xid.gtrid_length | long        | 8           | uint64       | 8            | length of the transactino gtrid part                                          
transaction.trid.xid.bqual_length | long        | 8           | uint64       | 8            | length of the transaction branch part                                         
transaction.trid.xid.payload      | byte array  | 32          | byte array   | 32           | byte array with the size of gtrid_length + bqual_length (max 128)             
transaction.state                 | int64       | 8           | uint64       | 8            | state of the transaction TX_ACTIVE, TX_TIMEOUT_ROLLBACK_ONLY, TX_ROLLBACK_ONLY
buffer.type.name.size             | size_t      | 8           | uint64       | 8            | buffer type name size                                                         
buffer.type.name.data             | byte array  | 8           | byte array   | 8            | byte array with buffer type name                                              
buffer.type.subname.size          | size_t      | 8           | uint64       | 8            | buffer type subname size                                                      
buffer.type.subname.data          | byte array  | 16          | byte array   | 16           | byte array with buffer type subname                                           
buffer.payload.size               | size_t      | 8           | uint64       | 8            | buffer payload size (could be very big)                                       
buffer.payload.data               | byte array  | 128         | byte array   | 128          | buffer payload data (with the size of buffer.payload.size)                    

## Transaction messages

### Resource prepare


#### common::message::transaction::resource::domain::prepare::Request

Sent to and received from other domains when one domain wants to prepare a transaction. 

message type: 5300

role name             | native type | native size | network type | network size | description                                                        
--------------------- | ----------- | ----------- | ------------ | ------------ | -------------------------------------------------------------------
execution             | byte array  | 16          | byte array   | 16           | uuid of the current execution path                                 
sender.pid            | int         | 4           | uint64       | 8            | pid of the sender process                                          
sender.queue          | long        | 8           | uint64       | 8            | ipc queue id of the sender process                                 
trid.xid.format       | long        | 8           | uint64       | 8            | xid format type. if 0 no more information of the xid is transported
trid.owner.pid        | int         | 4           | uint64       | 8            | pid of owner of the transaction                                    
trid.owner.queue      | long        | 8           | uint64       | 8            | ipc queue of owner of the transaction                              
trid.xid.gtrid_length | long        | 8           | uint64       | 8            | length of the transactino gtrid part                               
trid.xid.bqual_length | long        | 8           | uint64       | 8            | length of the transaction branch part                              
trid.xid.payload      | byte array  | 32          | byte array   | 32           | byte array with the size of gtrid_length + bqual_length (max 128)  
resource.id           | int         | 4           | uint64       | 8            | RM id of the resource                                              
flags                 | int         | 4           | uint64       | 8            | XA flags to be forward to the resource                             

#### common::message::transaction::resource::domain::prepare::Reply

Sent to and received from other domains when one domain wants to prepare a transaction. 

message type: 5301

role name             | native type | native size | network type | network size | description                                                        
--------------------- | ----------- | ----------- | ------------ | ------------ | -------------------------------------------------------------------
execution             | byte array  | 16          | byte array   | 16           | uuid of the current execution path                                 
sender.pid            | int         | 4           | uint64       | 8            | pid of the sender process                                          
sender.queue          | long        | 8           | uint64       | 8            | ipc queue id of the sender process                                 
trid.xid.format       | long        | 8           | uint64       | 8            | xid format type. if 0 no more information of the xid is transported
trid.owner.pid        | int         | 4           | uint64       | 8            | pid of owner of the transaction                                    
trid.owner.queue      | long        | 8           | uint64       | 8            | ipc queue of owner of the transaction                              
trid.xid.gtrid_length | long        | 8           | uint64       | 8            | length of the transactino gtrid part                               
trid.xid.bqual_length | long        | 8           | uint64       | 8            | length of the transaction branch part                              
trid.xid.payload      | byte array  | 32          | byte array   | 32           | byte array with the size of gtrid_length + bqual_length (max 128)  
resource.state        | int         | 4           | uint64       | 8            | The state of the operation - If successful XA_OK ( 0)              
resource.id           | int         | 4           | uint64       | 8            | RM id of the resource                                              
statistic.start       | int64       | 8           | uint64       | 8            | start time in us                                                   
statistic.end         | int64       | 8           | uint64       | 8            | end time in us                                                     

### Resource commit


#### common::message::transaction::resource::domain::commit::Request

Sent to and received from other domains when one domain wants to commit an already prepared transaction.

message type: 5302

role name             | native type | native size | network type | network size | description                                                        
--------------------- | ----------- | ----------- | ------------ | ------------ | -------------------------------------------------------------------
execution             | byte array  | 16          | byte array   | 16           | uuid of the current execution path                                 
sender.pid            | int         | 4           | uint64       | 8            | pid of the sender process                                          
sender.queue          | long        | 8           | uint64       | 8            | ipc queue id of the sender process                                 
trid.xid.format       | long        | 8           | uint64       | 8            | xid format type. if 0 no more information of the xid is transported
trid.owner.pid        | int         | 4           | uint64       | 8            | pid of owner of the transaction                                    
trid.owner.queue      | long        | 8           | uint64       | 8            | ipc queue of owner of the transaction                              
trid.xid.gtrid_length | long        | 8           | uint64       | 8            | length of the transactino gtrid part                               
trid.xid.bqual_length | long        | 8           | uint64       | 8            | length of the transaction branch part                              
trid.xid.payload      | byte array  | 32          | byte array   | 32           | byte array with the size of gtrid_length + bqual_length (max 128)  
resource.id           | int         | 4           | uint64       | 8            | RM id of the resource                                              
flags                 | int         | 4           | uint64       | 8            | XA flags to be forward to the resource                             

#### common::message::transaction::resource::domain::commit::Reply

Reply to a commit request. 

message type: 5303

role name             | native type | native size | network type | network size | description                                                        
--------------------- | ----------- | ----------- | ------------ | ------------ | -------------------------------------------------------------------
execution             | byte array  | 16          | byte array   | 16           | uuid of the current execution path                                 
sender.pid            | int         | 4           | uint64       | 8            | pid of the sender process                                          
sender.queue          | long        | 8           | uint64       | 8            | ipc queue id of the sender process                                 
trid.xid.format       | long        | 8           | uint64       | 8            | xid format type. if 0 no more information of the xid is transported
trid.owner.pid        | int         | 4           | uint64       | 8            | pid of owner of the transaction                                    
trid.owner.queue      | long        | 8           | uint64       | 8            | ipc queue of owner of the transaction                              
trid.xid.gtrid_length | long        | 8           | uint64       | 8            | length of the transactino gtrid part                               
trid.xid.bqual_length | long        | 8           | uint64       | 8            | length of the transaction branch part                              
trid.xid.payload      | byte array  | 32          | byte array   | 32           | byte array with the size of gtrid_length + bqual_length (max 128)  
resource.state        | int         | 4           | uint64       | 8            | The state of the operation - If successful XA_OK ( 0)              
resource.id           | int         | 4           | uint64       | 8            | RM id of the resource                                              
statistic.start       | int64       | 8           | uint64       | 8            | start time in us                                                   
statistic.end         | int64       | 8           | uint64       | 8            | end time in us                                                     

### Resource rollback


#### common::message::transaction::resource::domain::rollback::Request

Sent to and received from other domains when one domain wants to rollback an already prepared transaction.
That is, when one or more resources has failed to prepare.

message type: 5304

role name             | native type | native size | network type | network size | description                                                        
--------------------- | ----------- | ----------- | ------------ | ------------ | -------------------------------------------------------------------
execution             | byte array  | 16          | byte array   | 16           | uuid of the current execution path                                 
sender.pid            | int         | 4           | uint64       | 8            | pid of the sender process                                          
sender.queue          | long        | 8           | uint64       | 8            | ipc queue id of the sender process                                 
trid.xid.format       | long        | 8           | uint64       | 8            | xid format type. if 0 no more information of the xid is transported
trid.owner.pid        | int         | 4           | uint64       | 8            | pid of owner of the transaction                                    
trid.owner.queue      | long        | 8           | uint64       | 8            | ipc queue of owner of the transaction                              
trid.xid.gtrid_length | long        | 8           | uint64       | 8            | length of the transactino gtrid part                               
trid.xid.bqual_length | long        | 8           | uint64       | 8            | length of the transaction branch part                              
trid.xid.payload      | byte array  | 32          | byte array   | 32           | byte array with the size of gtrid_length + bqual_length (max 128)  
resource.id           | int         | 4           | uint64       | 8            | RM id of the resource                                              
flags                 | int         | 4           | uint64       | 8            | XA flags to be forward to the resource                             

#### common::message::transaction::resource::domain::rollback::Reply

Reply to a rollback request. 

message type: 5305

role name             | native type | native size | network type | network size | description                                                        
--------------------- | ----------- | ----------- | ------------ | ------------ | -------------------------------------------------------------------
execution             | byte array  | 16          | byte array   | 16           | uuid of the current execution path                                 
sender.pid            | int         | 4           | uint64       | 8            | pid of the sender process                                          
sender.queue          | long        | 8           | uint64       | 8            | ipc queue id of the sender process                                 
trid.xid.format       | long        | 8           | uint64       | 8            | xid format type. if 0 no more information of the xid is transported
trid.owner.pid        | int         | 4           | uint64       | 8            | pid of owner of the transaction                                    
trid.owner.queue      | long        | 8           | uint64       | 8            | ipc queue of owner of the transaction                              
trid.xid.gtrid_length | long        | 8           | uint64       | 8            | length of the transactino gtrid part                               
trid.xid.bqual_length | long        | 8           | uint64       | 8            | length of the transaction branch part                              
trid.xid.payload      | byte array  | 32          | byte array   | 32           | byte array with the size of gtrid_length + bqual_length (max 128)  
resource.state        | int         | 4           | uint64       | 8            | The state of the operation - If successful XA_OK ( 0)              
resource.id           | int         | 4           | uint64       | 8            | RM id of the resource                                              
statistic.start       | int64       | 8           | uint64       | 8            | start time in us                                                   
statistic.end         | int64       | 8           | uint64       | 8            | end time in us                                                     
