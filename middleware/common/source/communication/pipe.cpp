//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/communication/pipe.h"
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
         namespace pipe
         {
            
            namespace message
            {
               static_assert( transport::max_message_size() <= PIPE_BUF, "pipe message size has to be less or equal to 'PIPE_BUF'");
            } // message

            namespace local
            {
               namespace
               {
                  namespace open
                  {
                     strong::pipe::id write( const std::string& name)
                     {
                        Trace trace{ "common::communication::pipe::local::open::write"};

                        return strong::pipe::id{ posix::result( ::open( name.c_str(), O_WRONLY))};
                     }

                     strong::pipe::id read( const std::string& name)
                     {
                        Trace trace{ "common::communication::pipe::local::open::read"};

                        return strong::pipe::id{ posix::result( ::open( name.c_str(), O_RDONLY | O_NONBLOCK))};
                     }
                     
                  } // open

                  strong::pipe::id create( const Address& address)
                  {
                     Trace trace{ "common::communication::pipe::local::create"};

                     auto name = pipe::file( address);
                     log::line( communication::log, "pipe file name: ", name);

                     // make sure directories exists
                     directory::create( directory::name::base( name));

                     posix::result( ::mkfifo( name.c_str(), 0660));
                     return local::open::read( name);
                  }

                  namespace transport
                  {
                     bool send( strong::pipe::id id, const message::Transport& transport)
                     {
                        common::log::line( log, "---> [", id,  "] send transport: ", transport);

                        posix::result( ::write( 
                           id.underlaying(), 
                           transport.data(),
                           transport.size()));

                        return true;
                     }

                     bool receive( strong::pipe::id id, message::Transport& transport)
                     {
                        // read header
                        posix::result( ::read( 
                           id.underlaying(), 
                           transport.header_data(),
                           message::transport::header_size()));

                        // read payload
                        posix::result( ::read( 
                           id.underlaying(), 
                           transport.payload_data(),
                           transport.payload_size()));

                        common::log::line( log, "<--- [", id, "] receive transport: ", transport);

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

            Handle::Handle( strong::pipe::id id) : m_id( id) {}

            Handle::Handle( Handle&& other) : m_id( std::exchange( other.m_id, {}))
            {

            }

            Handle& Handle::operator = ( Handle&& other)
            {
               std::swap( m_id, other.m_id);
               return *this;
            }

            Handle::~Handle()
            {
               if( m_id)
               {
                  ::close( m_id.underlaying());
               }
            }


            namespace native
            {
               Handle create( const std::string& name)
               {
                  Trace trace{ "common::communication::pipe::native::create"};

                  posix::result( ::mkfifo( name.c_str(), 0660));
                  return Handle{ local::open::read( name)};
               }

               Handle open( const std::string& name)
               {
                  Trace trace{ "common::communication::pipe::native::open"};

                  return Handle{ local::open::write( name)};
               }

               namespace non
               {
                  namespace blocking
                  {
                     bool send( strong::pipe::id id, const message::Transport& transport)
                     {
                        // check pending signals
                        signal::handle();
                        return local::transport::send( id, transport);
                     }

                     bool receive( strong::pipe::id id, message::Transport& transport)
                     {
                        // check pending signals
                        signal::handle();
                        return local::transport::receive( id, transport);
                     }
                  } // blocking
               } // non

               namespace blocking
               {
                  bool send( strong::pipe::id id, const message::Transport& transport)
                  {
                     fd_set write_descriptors;
                     FD_ZERO( &write_descriptors);
                     FD_SET( id.underlaying(), &write_descriptors);

                     // block all signals
                     signal::thread::scope::Block block;
                     
                     // check pending signals
                     signal::handle();

                     posix::result( 
                        ::pselect( id.underlaying() + 1, nullptr, &write_descriptors, nullptr, nullptr, &block.previous().set));

                     return local::transport::send( id, transport);
                  }
                  bool receive( strong::pipe::id id, message::Transport& transport)
                  {
                     fd_set read_descriptors;
                     FD_ZERO( &read_descriptors);
                     FD_SET( id.underlaying(), &read_descriptors);

                     // block all signals
                     signal::thread::scope::Block block;
                     
                     // check pending signals
                     signal::handle();
                     
                     posix::result( 
                        ::pselect( id.underlaying() + 1, &read_descriptors, nullptr, nullptr, nullptr, &block.previous().set));

                     return local::transport::receive( id, transport);
                  }
               } // blocking
               
            } // native
            
            Address Address::create()
            {
               Address result;
               result.m_uuid = uuid::make();
               return result;
            }

            std::string file( const Address& address)
            {
               return environment::transient::directory() + '/' + uuid::string( address.uuid()) + ".fifo";
            }

            namespace policy
            {
               template< typename C>
               cache_range_type receive( C&& callable, strong::pipe::id id, cache_type& cache)
               {
                  message::Transport transport;

                  if( callable( id, transport))
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
               Uuid send( C&& callable, strong::pipe::id id, const communication::message::Complete& complete)
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
                     if( ! callable(  id, transport))
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
                  cache_range_type receive( strong::pipe::id id, cache_type& cache)
                  {
                     return policy::receive( &native::blocking::receive, id, cache);
                  }

                  Uuid send( strong::pipe::id id, const communication::message::Complete& complete)
                  {
                     return policy::send( &native::blocking::send, id, complete);
                  }
               } // blocking

               namespace non
               {
                  namespace blocking
                  {
                     cache_range_type receive( strong::pipe::id id, cache_type& cache);
                     Uuid send( strong::pipe::id id, const communication::message::Complete& complete);
                  } // blocking
               } // non
            } // policy

            namespace inbound
            {

               Connector::Connector() : m_address{ Address::create()}, m_id( local::create( m_address))
               {

               }
               Connector::~Connector()
               {
                  try
                  {
                     posix::log::result( ::close( m_id.underlaying()));
                     const auto name = file( m_address);
                     common::file::remove( name);

                  }
                  catch( ...)
                  {
                     exception::handle();
                  }
               }


               //Connector::Connector( Connector&& rhs) noexcept : m_address( ), m_id( std::exchange( rhs.m_id, {}))
               Connector::Connector( Connector&& rhs) noexcept = default;
               Connector& Connector::operator = ( Connector&& rhs) noexcept = default;

               
               void swap( Connector& lhs, Connector& rhs);
               std::ostream& operator << ( std::ostream& out, const Connector& rhs)
               {
                  return out << "{ id: " << rhs.m_id
                     << ", address" << rhs.m_address
                     << '}';
               }
               
            } // inbound


            namespace outbound
            {
               Connector::Connector( const pipe::Address& address) : m_id( local::open::write( file( address)))
               {
               }

               Connector::~Connector()
               {
                  posix::log::result( ::close( m_id.underlaying()));
               }


               std::ostream& operator << ( std::ostream& out, const Connector& rhs)
               {
                  return out << "{ id: " << rhs.m_id
                     << '}';
               }

            } // outbound
         } // pipe
      } // communication
   } // common
} // casual