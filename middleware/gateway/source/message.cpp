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
         namespace manager
         {
            namespace listener
            {
               std::ostream& operator << ( std::ostream& out, const Event& value)
               {
                  auto get_state = []( const Event& e) {
                     switch( e.state)
                     {
                        case Event::State::running: return "running";
                        case Event::State::exit: return "exit";
                        case Event::State::error: return "error";
                     }
                  };

                  return out << "{ correlation: " << value.correlation
                        << ", state: " << get_state( value)
                        << '}';
               }


            } // listener
         } // manager

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

         namespace tcp
         {
            std::ostream& operator << ( std::ostream& out, const Connect& value)
            {
               return out << "{ descriptor: " << value.descriptor << '}';
            }

         } // tcp

         namespace worker
         {


         } // worker


      } // message

   } // gateway


} // casual
