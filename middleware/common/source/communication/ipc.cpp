//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/communication/ipc.h"
#include "common/communication/log.h"

#include "common/result.h"
#include "common/log.h"
#include "common/signal.h"
#include "common/environment.h"

#include "common/exception/handle.h"


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace ipc
         {
            
            namespace message
            {
               static_assert( transport::max_message_size() <= PIPE_BUF, "ipc message size has to be less or equal to 'PIPE_BUF'");
            } // message

            namespace local
            {
               namespace
               {
                  namespace signal
                  {
                     void handle() 
                     {
                        try
                        {
                           common::signal::handle();
                        }
                        catch( const exception::signal::Pipe&)
                        {
                           throw exception::system::communication::unavailable::Pipe{};
                        }
                     }
                  } // signal

                  namespace open
                  {
                     strong::file::descriptor::id write( const std::string& name)
                     {
                        Trace trace{ "common::communication::ipc::local::open::write"};

                        log::line( communication::verbose::log, "name: ", name);

                        return strong::file::descriptor::id{ posix::result( ::open( name.c_str(), O_WRONLY | O_NONBLOCK))};
                        //return strong::file::descriptor::id{ posix::result( ::open( name.c_str(), O_WRONLY))};
                     }

                     strong::file::descriptor::id read( const std::string& name)
                     {
                        Trace trace{ "common::communication::ipc::local::open::read"};

                        log::line( communication::verbose::log, "name: ", name);

                        auto file_descriptor = posix::result( ::open( name.c_str(), O_RDONLY | O_NONBLOCK));
                        //auto file_descriptor = posix::result( ::open( name.c_str(), O_RDWR));
                        //auto flags = ::fcntl( file_descriptor, F_GETFL);
                        //::fcntl( file_descriptor, F_SETFL, flags & ~O_NONBLOCK);

                        return strong::file::descriptor::id{ file_descriptor};
                     }
                  } // open

                  strong::file::descriptor::id create( strong::ipc::id ipc)
                  {
                     Trace trace{ "common::communication::ipc::local::create"};

                     auto name = file( ipc);

                     // make sure directories exists
                     directory::create( directory::name::base( name));

                     posix::result( ::mkfifo( name.c_str(), 0660));
                     
                     log::line( communication::verbose::log, "created fifo: ", name);

                     return local::open::read( name);
                  }

                  namespace transport
                  {
                     bool send( const Handle& handle, const message::Transport& transport)
                     {
                        Trace trace{ "common::communication::ipc::local::transport::send"};

                        common::log::line( log, "---> [", handle.ipc(),  "] send transport: ", transport);

                        posix::result( ::write( 
                           handle.id().value(), 
                           transport.data(),
                           transport.size()));

                        return true;
                     }

                     bool receive( const Handle& handle, message::Transport& transport)
                     {
                        Trace trace{ "common::communication::ipc::local::transport::receive"};
                        
                        // read header
                        auto read = ::read( 
                           handle.id().value(), 
                           transport.header_data(),
                           message::transport::header_size());

                        if( read == 0)
                           return false;
                        
                        if( read == -1)
                        {
                           switch( code::last::system::error())
                           {
                              case code::system::resource_unavailable_try_again:
                                 return false;
                              case code::system::interrupted:
                                 local::signal::handle();
                                 return false;
                              default: 
                                 exception::system::throw_from_errno();
                           }  
                        }
                        assert( read == message::transport::header_size());

                        // read payload
                        read = posix::result( ::read( 
                           handle.id().value(), 
                           transport.payload_data(),
                           transport.payload_size()));

                        assert( read == transport.payload_size());

                        common::log::line( log, "<--- [", handle.ipc(), "] receive transport: ", transport);

                        return true;
                     }
                  } // transport


               } // <unnamed>
            } // local

            namespace message
            {
               std::ostream& operator << ( std::ostream& out, const Transport& value)
               {
                  return out << "{ type: " << value.type()
                     << ", correlation: " << uuid::string( value.correlation())
                     << ", offset: " << value.payload_offset()
                     << ", payload.size: " << value.payload_size()
                     << ", complete_size: " << value.complete_size()
                     << ", header-size: " << transport::header_size()
                     << ", transport-size: " <<  value.size()
                     << ", max-size: " << transport::max_message_size() << "}";
               }
            } // message

            Handle::Handle( strong::file::descriptor::id id, strong::ipc::id ipc) : m_id( id), m_ipc( ipc)
            {
               log::line( communication::verbose::log, "created handle: ", *this);
            }

            Handle::Handle( Handle&& other) noexcept 
               : m_id( std::exchange( other.m_id, {})), 
                 m_ipc( std::exchange( other.m_ipc, {}))
            {

            }

            Handle& Handle::operator = ( Handle&& other) noexcept
            {
               std::swap( m_id, other.m_id);
               std::swap( m_ipc, other.m_ipc);
               return *this;
            }

            void Handle::blocking() const
            {
               auto flags = ::fcntl( m_id.value(), F_GETFL);
               ::fcntl( m_id.value(), F_SETFL, flags & ~O_NONBLOCK);
            }

            void Handle::non_blocking() const
            {
               auto flags = ::fcntl( m_id.value(), F_GETFL);
               ::fcntl( m_id.value(), F_SETFL, flags | O_NONBLOCK);
            }

            Handle::~Handle()
            {
               if( m_id)
               {
                  log::line( communication::verbose::log, "closing: ", *this);
                  posix::log::result( ::close( m_id.value()));
               }
            }

            std::ostream& operator << ( std::ostream& out, const Handle& rhs)
            {
               return out << "{ ipc: " << rhs.m_ipc << ", descriptor: " << rhs.m_id << '}';
            }


            namespace native
            {
               Handle create( strong::ipc::id ipc)
               {
                  Trace trace{ "common::communication::ipc::native::create"};

                  return Handle{ local::create( ipc), ipc};                  
               }

               namespace open
               {
                  Handle read( strong::ipc::id ipc)
                  {
                     Trace trace{ "common::communication::ipc::native::open::read"};

                     return Handle{ local::open::read( file( ipc)), ipc};
                  }
                  
                  Handle write( strong::ipc::id ipc)
                  {
                     Trace trace{ "common::communication::ipc::native::open::write"};

                     return Handle{ local::open::write( file( ipc)), ipc};
                  }
               } // open


               namespace non
               {
                  namespace blocking
                  {
                     bool send( const Handle& handle, const message::Transport& transport)
                     {
                        Trace trace{ "common::communication::ipc::native::non::blocking::send"};

                        log::line( communication::verbose::log, "handle: ", handle);

                        handle.non_blocking();
                        
                        // check pending signals
                        local::signal::handle();
                        return local::transport::send( handle, transport);
                     }

                     bool receive( const Handle& handle, message::Transport& transport)
                     {
                        Trace trace{ "common::communication::ipc::native::non::blocking::receive"};

                        log::line( communication::verbose::log, "handle: ", handle);
                        
                        handle.non_blocking();

                        // check pending signals
                        local::signal::handle();
                        return local::transport::receive( handle, transport);
                     }
                  } // blocking
               } // non

               namespace blocking
               {
                  bool send( const Handle& handle, const message::Transport& transport)
                  {
                     Trace trace{ "common::communication::ipc::native::blocking::send"};

                     log::line( communication::verbose::log, "write: ", handle);

                     handle.blocking();

                     //fd_set write_descriptors;
                     //FD_ZERO( &write_descriptors);
                     //FD_SET( handle.id().value(), &write_descriptors);

                     // block all signals
                     ////signal::thread::scope::Block block;
                     
                     // check pending signals
                     //signal::handle( block.previous());
                     local::signal::handle();

                     //posix::result( 
                     //   ::pselect( handle.id().underlaying() + 1, nullptr, &write_descriptors, nullptr, nullptr, &block.previous().set));

                     return local::transport::send( handle, transport);
                  }
                  bool receive( Handle& handle, message::Transport& transport)
                  {
                     Trace trace{ "common::communication::ipc::native::blocking::receive"};

                     log::line( communication::verbose::log, "read: ", handle);

                     handle.blocking();

                     //fd_set read_descriptors;
                     //FD_ZERO( &read_descriptors);
                     //FD_SET( handle.id().value(), &read_descriptors);

                     // block all signals
                     //signal::thread::scope::Block block;
                     
                     // check pending signals
                     //signal::handle( block.previous());
                     local::signal::handle();
                     
                     //posix::result( 
                        //::pselect( handle.id().value() + 1, &read_descriptors, nullptr, nullptr, nullptr, &block.previous().set));

                     return local::transport::receive( handle, transport);
                     //while( ! local::transport::receive( handle, transport))
                     //   ; // no-op

                     //return true;
                  }
               } // blocking
               
            } // native

            std::string file( strong::ipc::id ipc)
            {
               return environment::transient::directory() + '/' + uuid::string( ipc.underlaying()) + ".fifo";
            }

            namespace policy
            {
               template< typename C, typename H>
               cache_range_type receive( C&& callable, H& handle, cache_type& cache)
               {
                  message::Transport transport;

                  if( callable( handle, transport))
                  {
                     auto found = algorithm::find_if( cache,
                           [&]( const auto& m)
                           {
                              return ! m.complete() 
                                 && transport.message.header.correlation == m.correlation;
                           });

                     if( found)
                     {
                        found->add( transport);
                        return found;
                     }
                     else
                     {
                        cache.emplace_back(
                              transport.type(),
                              transport.correlation(),
                              transport.complete_size(),
                              transport);

                        return cache_range_type{ std::end( cache) - 1, std::end( cache)};
                     }
                  }

                  return cache_range_type{};
               }

               template< typename C>
               Uuid send( C&& callable, const Handle& handle, const communication::message::Complete& complete)
               {
                  message::Transport transport{ complete.type, complete.size()};

                  complete.correlation.copy( transport.correlation());

                  auto part_begin = std::begin( complete.payload);

                  do
                  {
                     auto part_end = std::distance( part_begin, std::end( complete.payload)) > message::transport::max_payload_size() ?
                           part_begin + message::transport::max_payload_size() : std::end( complete.payload);

                     transport.assign( range::make( part_begin, part_end));
                     transport.message.header.offset = std::distance( std::begin( complete.payload), part_begin);

                     //
                     // send the physical message
                     //
                     if( ! callable( handle, transport))
                     {
                        return uuid::empty();
                     }

                     part_begin = part_end;
                  }
                  while( part_begin != std::end( complete.payload));

                  return complete.correlation;

               }



               namespace blocking
               {
                  cache_range_type receive( Handle& handle, cache_type& cache)
                  {
                     return policy::receive( &native::blocking::receive, handle, cache);
                  }

                  Uuid send( const Handle& handle, const communication::message::Complete& complete)
                  {
                     return policy::send( &native::blocking::send, handle, complete);
                  }
               } // blocking

               namespace non
               {
                  namespace blocking
                  {
                     cache_range_type receive( const Handle& handle, cache_type& cache)
                     {
                        return policy::receive( &native::non::blocking::receive, handle, cache);
                     }
                     Uuid send( const Handle& handle, const communication::message::Complete& complete)
                     {
                        return policy::send( &native::non::blocking::send, handle, complete);
                     }
                  } // blocking
               } // non
            } // policy

            namespace inbound
            {

               Connector::Connector() 
                  : m_id( native::create( strong::ipc::id{ uuid::make()}))
                  ,m_dummy_writer( native::open::write( m_id.ipc()))
               {
                  m_dummy_writer.blocking();
               }

               Connector::~Connector()
               {
                  try
                  {
                     if( m_id)
                     {
                        const auto name = file( m_id.ipc());
                        common::file::remove( name);
                     }
                  }
                  catch( ...)
                  {
                     exception::handle();
                  }
               }

               std::ostream& operator << ( std::ostream& out, const Connector& rhs)
               {
                  return out << "{ id: " << rhs.m_id
                     << '}';
               }

               Device& device()
               {
                  static Device singleton;
                  return singleton;
               }
            } // inbound


            namespace outbound
            {
               Connector::Connector( strong::ipc::id ipc) : m_id( native::open::write( ipc))
               {
               }

               std::ostream& operator << ( std::ostream& out, const Connector& rhs)
               {
                  return out << "{ id: " << rhs.m_id
                     << '}';
               }

            } // outbound




            bool exists( strong::ipc::id id)
            {
               const auto name = ipc::file( id);
               return ::access( name.c_str(), F_OK) != -1; 
            }
/*
            bool remove( strong::ipc::id id);
            bool remove( const process::Handle& owner);
            */

         } // ipc
      } // communication
   } // common
} // casual