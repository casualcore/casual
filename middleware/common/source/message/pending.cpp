//!
//! casual 
//!

#include "common/message/pending.h"


namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace pending
         {
            Message::Message( communication::message::Complete&& complete, targets_type&& targets, Targets task)
               : targets( std::move( targets)), complete( std::move( complete)), task( task)
            {

            }

            Message::Message( communication::message::Complete&& complete, targets_type&& targets)
             : Message{ std::move( complete), std::move( targets), Targets::all}
            {

            }

            Message::Message( Message&&) = default;
            Message& Message::operator = ( Message&&) = default;

            bool Message::sent() const
            {
               return targets.empty();
            }

            Message::operator bool () const
            {
               return sent();
            }

            std::ostream& operator << ( std::ostream& out, const Message& value)
            {
               return out << "{ targets: " << range::make( value.targets) << ", complete: " << value.complete << "}";
            }

            namespace policy
            {
               bool consume_unavalibe::operator () () const
               {
                  try
                  {
                     throw;
                  }
                  catch( const exception::queue::Unavailable&)
                  {
                     return true;
                  }
               }

            } // policy

            namespace non
            {
               namespace blocking
               {
                  bool send( Message& message, const communication::error::type& handler)
                  {
                     return send( message, communication::ipc::policy::non::Blocking{}, handler);
                  }

               } // blocking
            } // non

         } // pending
      } // message
   } // common
} // casual