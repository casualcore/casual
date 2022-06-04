/** 
 ** Copyright (c) 2015, The casual project
 **
 ** This software is licensed under the MIT license, https://opensource.org/licenses/MIT
 **/

#pragma once

#include <uuid/uuid.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef long casual_message_descriptor_t;
typedef long casual_selector_descriptor_t;

struct casual_buffer_t 
{
   char* data;
   long size;
};

typedef struct casual_buffer_t casual_buffer_t;

/* constants  
 */
#define CASUAL_QUEUE_NO_SELECTOR -1l


/* error codes
 */
#define CASUAL_QE_OK 0
#define CASUAL_QE_NO_MESSAGE 1
#define CASUAL_QE_NO_QUEUE 10
#define CASUAL_QE_INVALID_ARGUMENTS 20
#define CASUAL_QE_SYSTEM 30


extern int casual_queue_get_errno();
#define casual_qerrno casual_queue_get_errno()



/* create a message
*/
extern casual_message_descriptor_t casual_queue_message_create( casual_buffer_t buffer);

/* clean up memory for the message
* @attention Will not call tpfree on the associated buffers.
*/
extern int casual_queue_message_delete( const casual_message_descriptor_t message);

/* sets the _property_ string, which can be used to select on during dequeue, hence be able 
   to correlate messages
   @returns 0 on success -1 on error and casual_qerrno is set 
*/
extern int casual_queue_message_attribute_set_properties( casual_message_descriptor_t message, const char* properties);
extern const char* casual_queue_message_attribute_get_properties( casual_message_descriptor_t message);

/* sets the reply queue for the message
   @returns 0 on success -1 on error and casual_qerrno is set 
*/
extern int casual_queue_message_attribute_set_reply( casual_message_descriptor_t message, const char* queue);

/* when the message should be available for dequeue, ms since epoch, hence absolute time 
   @returns 0 on success -1 on error and casual_qerrno is set 
*/
extern int casual_queue_message_attribute_set_available( casual_message_descriptor_t message, long ms_since_epoch);

/* (re)sets the associated buffer to the message 
  @returns 0 on success -1 on error and casual_qerrno is set
*/
extern int casual_queue_message_set_buffer( casual_message_descriptor_t message, casual_buffer_t buffer);
/* @returns 0 on success -1 on error and casual_qerrno is set */
extern int casual_queue_message_get_buffer( casual_message_descriptor_t message, casual_buffer_t* buffer);

/* explicitly set the message id. casual will generate a unique id otherwise */
extern int casual_queue_message_set_id( casual_message_descriptor_t message, const uuid_t* id);
/* @returns 0 on success -1 on error and casual_qerrno is set */
extern int casual_queue_message_get_id( casual_message_descriptor_t message, uuid_t* id);

extern casual_selector_descriptor_t casual_queue_selector_create();
/* deletes the selector 
  @returns 0 on success -1 on error and casual_qerrno is set
*/
extern int casual_queue_selector_delete( casual_selector_descriptor_t selector);


/* selector correlate only to messages that matches properties exactly */
extern int casual_queue_selector_set_properties( casual_selector_descriptor_t selector, const char* properties);

/* selector correlate only to the explicit message id set*/
extern int casual_queue_selector_set_id( casual_selector_descriptor_t selector, const uuid_t* id);


/* enqueue the message associated with the provided message descriptor, to the `queue`
   @returns 0 on success -1 on error and casual_qerrno is set
 */
extern int casual_queue_enqueue( const char* queue, const casual_message_descriptor_t message);

/* tries to dequeue a message from the given queue 
   @param queue the queue to dequeue from
   @param selector a descriptor to a previously created selector, or `CASUAL_QUEUE_NO_SELECTOR` if no selector.
   @returns descriptor to the messages, or -1 if there where some errors (including CASUAL_QE_NO_MESSAGE)
   
   @attention it's the caller responsibility to call `casual_queue_message_delete` on the returned message descriptor
     and call `tpfree` on the buffer associated with the descriptor
 */
extern casual_message_descriptor_t casual_queue_dequeue( const char* queue, casual_selector_descriptor_t selector);


/* tries to peek on a message from the given queue
   @param queue the queue to dequeue from
   @param selector a descriptor to a previously created selector, or `CASUAL_QUEUE_NO_SELECTOR` if no selector.
   @returns descriptor to the messages, or -1 if there where some errors (including CASUAL_QE_NO_MESSAGE)

   @attention it's the caller responsibility to call `casual_queue_message_delete` on the returned message descriptor
     and call `tpfree` on the buffer associated with the descriptor
 */
extern casual_message_descriptor_t casual_queue_peek( const char* queue, casual_selector_descriptor_t selector);


#ifdef __cplusplus
}
#endif
