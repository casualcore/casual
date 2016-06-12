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
               std::ostream& operator << ( std::ostream& out, const Event::State& value)
               {
                  switch( value)
                  {
                     case Event::State::running: return out << "running";
                     case Event::State::exit: return out << "exit";
                     case Event::State::signal: return out << "signal";
                     case Event::State::error: return out << "error";
                  }
                  return out;
               }
               std::ostream& operator << ( std::ostream& out, const Event& value)
               {
                  return out << "{ correlation: " << value.correlation
                        << ", state: " << value.state
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
                        << '}';
               }

               std::ostream& operator << ( std::ostream& out, const Reply& value)
               {
                  return out << "{ process: " << value.process
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
            std::ostream& operator << ( std::ostream& out, Disconnect::Reason value)
            {
               switch( value)
               {
                  case Disconnect::Reason::signal: return out << "signal";
                  case Disconnect::Reason::disconnect: return out << "disconnect";
                  default: return out << "invalid";
               }
            }
            std::ostream& operator << ( std::ostream& out, const Disconnect& value)
            {
               return out << "{ reason: " << value.reason << ", remote: " << value.remote << '}';
            }


         } // worker


      } // message

   } // gateway


} // casual
