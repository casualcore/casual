#include "common/communication/select.h"
#include "common/communication/log.h"

#include "common/signal.h"
#include "common/result.h"

namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace select
         {
            void block( const std::vector< Action>& actions)
            {
               Trace trace{ "common::communication::select::block"};

               platform::file::descriptor::native::type max = 0;

               fd_set read;
               FD_ZERO( &read);
               fd_set write;
               FD_ZERO( &write);

               for( const Action& action : actions)
               {
                  max = std::max( action.descriptor.value(), max);
                  switch( action.direction)
                  {
                     case Action::Direction::read:
                     {
                        FD_SET( action.descriptor.value(), &read);
                        break;
                     }
                     case Action::Direction::write:
                     {
                        FD_SET( action.descriptor.value(), &write);
                        break;
                     }
                  }
               };

               // block all signals
               signal::thread::scope::Block block;
               
               // check pending signals
               signal::handle();

               // will set previous signal mask atomically
               posix::result( 
                  ::pselect( max + 1, &read, &write, nullptr, nullptr, &block.previous().set));
               
               // check which file descriptor can do progress
               for( const Action& action : actions)
               {
                  switch( action.direction)
                  {
                     case Action::Direction::read:
                     {
                        if( FD_ISSET( action.descriptor.value(), &read))
                        {
                           action.callback( action.descriptor);
                           // todo: do we want to return?
                        }
                        break;
                     }
                     case Action::Direction::write:
                     {
                        if( FD_ISSET( action.descriptor.value(), &write))
                        {
                           action.callback( action.descriptor);
                           // todo: do we want to return?
                        }
                        break;
                     }
                  }
               };

            }

         } // select
      } // communication
   } // common
} // casual