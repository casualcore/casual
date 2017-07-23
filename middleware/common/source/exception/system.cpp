#include "common/exception/system.h"
#include "common/log/category.h"


#include <cerrno>

namespace casual
{
   namespace common
   {
      namespace exception 
      {
         namespace system 
         {
            namespace local
            {

               namespace
               {
                  template< typename... Args>
                  void throw_from_code( error::code::system code, Args&&... args)
                  {
                     switch( error::code::last::system::error())
                     {
                        using sys = error::code::system;

                        case sys::invalid_argument: throw system::invalid::Argument( std::forward< Args>( args)...);
                        case sys::no_such_file_or_directory: throw system::invalid::File( std::forward< Args>( args)...);
                        case sys::no_such_process: throw system::invalid::Process( std::forward< Args>( args)...);

                        // communication
                        case sys::identifier_removed: throw system::communication::unavailable::Removed( std::forward< Args>( args)...);
                        case sys::connection_reset: throw system::communication::unavailable::Reset( std::forward< Args>( args)...);
                        case sys::broken_pipe: throw system::communication::unavailable::Pipe( std::forward< Args>( args)...);
                        case sys::not_connected: throw system::communication::unavailable::no::Connect( std::forward< Args>( args)...);
                        case sys::connection_refused: throw system::communication::Refused( std::forward< Args>( args)...);
                        case sys::protocol_error: throw system::communication::Protocol( std::forward< Args>( args)...);

                        // no message
                        case sys::no_message: throw system::communication::no::message::Absent( std::forward< Args>( args)...);
#if EAGAIN != EWOULDBLOCK
                        case system::operation_would_block: 
#endif
                        case sys::resource_unavailable_try_again: throw system::communication::no::message::Resource( std::forward< Args>( args)...);

                        default:
                        {
                           common::log::category::error << code <<  " - throwing a generic exception\n";
                           throw system::exception( code, std::forward< Args>( args)...);
                        }
                     }
                  }

                  template< typename... Args>
                  void throw_from_errno( Args&&... args)
                  {
                     throw_from_code( error::code::last::system::error(), std::forward< Args>( args)...);
                  }
               } // <unnamed>
            } // local
            void throw_from_errno()
            {
               local::throw_from_errno();
            }

            void throw_from_errno( const std::string& context)
            {
               local::throw_from_errno( context);
            }

            void throw_from_errno( const char* context)
            {
               local::throw_from_errno( context);
            }

            void throw_from_code( int code)
            {
               local::throw_from_code( static_cast< error::code::system>( code));
            }
            
         } // system 
      } // exception 
   } // common
} // casual