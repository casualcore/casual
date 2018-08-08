//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
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
            Message::Message( communication::message::Complete&& complete, targets_type targets, Targets task)
               : targets( std::move( targets)), complete( std::move( complete)), task( task)
            {
            }

            Message::Message( communication::message::Complete&& complete, targets_type targets)
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

            void Message::remove( strong::ipc::id ipc)
            {
               algorithm::trim( targets, algorithm::remove( targets, ipc));
            }

            void Message::remove( strong::process::id pid)
            {
               algorithm::trim( targets, algorithm::remove( targets, pid));
            }

            std::ostream& operator << ( std::ostream& out, const Message& value)
            {
               return out << "{ targets: " << range::make( value.targets) << ", complete: " << value.complete << "}";
            }

            namespace policy
            {
               bool consume_unavailable::operator () () const
               {
                  try
                  {
                     throw;
                  }
                  catch( const exception::system::communication::Unavailable&)
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
