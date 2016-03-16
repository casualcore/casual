//!
//! casual 
//!

#include "gateway/message.h"

namespace casual
{
   namespace gateway
   {
      namespace message
      {

         namespace outbound
         {

         } // outbound

         namespace ipc
         {
            namespace connect
            {

               std::ostream& operator << ( std::ostream& out, const Request& value)
               {
                  return out << "{ process: " << value.process
                        << ", remote: " << value.remote
                        << '}';
               }

               std::ostream& operator << ( std::ostream& out, const Reply& value)
               {
                  return out << "{ process: " << value.process
                        << ", remote: " << value.remote
                        << '}';
               }

            } // connect



         } // ipc


         namespace worker
         {


         } // worker


      } // message

   } // gateway


} // casual
