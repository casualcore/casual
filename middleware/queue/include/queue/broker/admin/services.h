//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_QUEUE_INCLUDE_QUEUE_BROKER_ADMIN_SERVICES_H_
#define CASUAL_MIDDLEWARE_QUEUE_INCLUDE_QUEUE_BROKER_ADMIN_SERVICES_H_


namespace casual
{
   namespace queue
   {
      namespace broker
      {
         namespace admin
         {
            namespace service
            {
               namespace name
               {
                  constexpr auto state() { return ".casual/queue/state";}
                  constexpr auto restore() { return ".casual/queue/restore";}
                  constexpr auto list_messages() { return ".casual/queue/list/messages";}
               } // name

            } // service

         } // admin
      } // broker
   } // queue

} // casual

#endif // CASUAL_MIDDLEWARE_QUEUE_INCLUDE_QUEUE_BROKER_ADMIN_SERVICES_H_
