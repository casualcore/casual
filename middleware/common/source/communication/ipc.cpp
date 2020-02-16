//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/communication/ipc.h"
#include "common/communication/log.h"
#include "common/communication/select.h"

#include "common/result.h"
#include "common/log.h"
#include "common/signal.h"
#include "common/environment.h"

#include "common/exception/handle.h"


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <utility>


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
               namespace transport
               {
                  std::ostream& operator << ( std::ostream& out, const Header& value)
                  {
                     out << "{ type: " << value.type << ", correlation: ";
                     transcode::hex::encode( out, value.correlation);
                     return stream::write( out, ", offset: ", value.offset, ", count: ", value.count, ", size: ", value.size, '}');
                  }
                  
                  static_assert( max_message_size <= platform::ipc::transport::size, "ipc message is too big'");
               } // transport
               

               std::ostream& operator << ( std::ostream& out, const Transport& value)
               {
                  return stream::write( out, 
                     "{ header: " , value.message.header
                     , ", payload.size: " , value.payload_size()
                     , ", header-size: " , transport::header_size
                     , ", transport-size: " ,  value.size()
                     , ", max-size: " , transport::max_message_size
                     , '}');
               }
            } // message

            namespace local
            {
               namespace
               {
                  namespace fifo
                  {
                     template< typename... Ts>
                     bool check_error( code::system code, Ts&&... ts)
                     {
                        
                        switch( code)
                        {
                           case code::system::broken_pipe:
                           case code::system::no_such_file_or_directory:
                              throw exception::system::communication::unavailable::File{}; 

                           case code::system::resource_unavailable_try_again:
#if EAGAIN != EWOULDBLOCK
                           case code::system::operation_would_block: 
#endif                           
                              // return false
                              break;
                           
                           default: 
                              log::line( log::category::verbose::error, code, " - ", std::forward< Ts>( ts)...);
                              // will allways throw
                              exception::system::throw_from_code( code);   
                        
                        }
                        return false;
                     }

                     auto path( const strong::ipc::id& ipc)
                     {
                        std::ostringstream out;
                        out << environment::ipc::directory() << '/' << ipc.value();
                        return std::move( out).str();
                     }

                     auto make()
                     {
                        strong::ipc::id ipc{ uuid::make()};
                        auto path = fifo::path( ipc);

                        posix::error::result( ::mkfifo( path.c_str(), 0660), "failed to create fifo path: ", path);
                        return ipc;
                     }

                     auto open( const std::string& path, file::Flags flags)
                     {
                        auto result = ::open( path.c_str(), flags.underlaying());
                        if( result == -1)
                           check_error( code::last::system::error(), "failed to open path: ", path, " - flags: ", flags);

                        return strong::file::descriptor::id{ result};
                     }

                     auto open( const strong::ipc::id& ipc, file::Flags flags)
                     {
                        return open( fifo::path( ipc), flags);
                     }

                     void close( strong::file::descriptor::id descriptor)
                     {
                        log::line( verbose::log, "closing descriptor: ", descriptor);
                        posix::log::result( ::close( descriptor.value()), "failed to close filedescriptor: ", descriptor);
                     }



                     namespace native
                     {
                        bool write( strong::file::descriptor::id descriptor, const message::Transport& transport)
                        {
                           Trace trace{ "common::communication::ipc::local::fifo::native::write"};

                           // we rely on the "fact" that we write at most PIPE_BUF bytes and this 
                           // is guranteed to be atomic, hence we will not be interupted mid write with 
                           // signals and other stuff.
                           auto result = ::write(
                              descriptor.value(), 
                              transport.data(), 
                              transport.size());

                           if( result == -1)
                              return local::fifo::check_error( code::last::system::error());

                           if( result > 0)
                           {
                              log::line( verbose::log, "ipc ---> write - count: ", result, ", descriptor: ", descriptor, ", transport: ", transport);
                              return true;
                           }

                           return false;
                        }

                  
                        bool read( strong::file::descriptor::id descriptor, message::Transport& transport)
                        {
                           Trace trace{ "common::communication::ipc::local::fifo::native::read"};

                           // we rely on the "fact" that we read at most PIPE_BUF bytes and this 
                           // is guranteed to be atomic, hence we will not be interupted mid read with 
                           // signals and other stuff.
                           
                           // consume the header
                           {
                              auto result = ::read(
                                 descriptor.value(), 
                                 transport.header_data(), 
                                 message::transport::header_size);

                              if( result == -1)
                                 return local::fifo::check_error( code::last::system::error());

                              assert( result == message::transport::header_size);
                           }

                           // consume the payload. We know that if we've got the header
                           // it's guaranteed that the payload will be there.
                           // TODO semantics: we might need to block signals?
                           {
                              auto result = ::read(
                                 descriptor.value(), 
                                 transport.payload_data(), 
                                 transport.payload_size());

                              assert( result == transport.payload_size());
                           }

                           log::line( verbose::log, "ipc <---- read - descriptor: ", descriptor, ", transport: ", transport);
                           return true;

                        }
                     } // native   
                  } // fifo
                  
               } // <unnamed>
            } // local

            namespace file
            {
               // TODO performance: If we find out/messure that fcntl is "expensive"
               // we could use the highest bit in the file-descriptor to indicate if it
               // is in blocking or non-blocking mode.

               namespace local
               {
                  namespace
                  {
                     auto get_flags = []( auto descriptor)
                     {
                        return posix::result( fcntl( descriptor.value(), F_GETFL));
                     };

                     auto set_flags = []( auto descriptor, auto flags)
                     {
                        posix::result( fcntl( descriptor.value(), F_SETFL, flags));
                     };

                     auto is_non_blocking = []( auto flags)
                     {
                        return ( flags & O_NONBLOCK) == O_NONBLOCK;
                     };
                     
                  } // <unnamed>
               } // local

               Descriptor::Descriptor( const strong::ipc::id& ipc, file::Flags flags)
                  : m_descriptor{ ipc::local::fifo::open( ipc, flags)}
               {
               }

               Descriptor::~Descriptor()
               {
                  exception::guard([&]()
                  {
                     if( m_descriptor)
                        ipc::local::fifo::close( m_descriptor);
                  });
               }

               Descriptor::Descriptor( Descriptor&& other) noexcept
                  : m_descriptor{ std::exchange( other.m_descriptor, {})} {}
               
               Descriptor& Descriptor::operator = ( Descriptor&& other) noexcept
               {
                  m_descriptor = std::exchange( other.m_descriptor, {});
                  return *this;
               }

               strong::file::descriptor::id Descriptor::blocking() const
               {
                  auto flags = local::get_flags( m_descriptor);

                  if( local::is_non_blocking( flags))
                     local::set_flags( m_descriptor,  ~O_NONBLOCK & flags);
                     
                  return m_descriptor;
               }

               strong::file::descriptor::id Descriptor::non_blocking() const
               {
                  auto flags = local::get_flags( m_descriptor);
                  
                  if( ! local::is_non_blocking( flags))
                     local::set_flags( m_descriptor, O_NONBLOCK | flags);
                     
                  return m_descriptor;
               }

               std::ostream& operator << ( std::ostream& out, const Descriptor& value)
               {
                  return out << "{ descriptor: " << value.m_descriptor 
                     << ", flags: " << std::hex << local::get_flags( value.m_descriptor) << std::dec
                     << '}';
               }

  
            } // file

            namespace named
            {
               Pipe::Pipe() : m_ipc{ local::fifo::make()} {}

               Pipe::~Pipe()
               {
                  exception::guard([&]()
                  {
                     if( m_ipc)
                        ipc::remove( m_ipc);
                  });
               }

               Pipe::Pipe( Pipe&& other) noexcept
                  : m_ipc{ std::exchange( other.m_ipc, {})}
               {
               }

               Pipe& Pipe::operator = ( Pipe&& other) noexcept
               {
                  m_ipc = std::exchange( other.m_ipc, {});
                  return *this;
               }

            } // named

            namespace handle
            {
 
               Inbound::Inbound() 
                  : m_descriptor{ m_pipe.ipc(), flag::make( file::Flag::read_only, file::Flag::no_block)},
                  m_dummy_writer{ m_pipe.ipc(), file::Flag::write_only}
               {
               }


               Outbound::Outbound( strong::ipc::id ipc) 
                  : m_descriptor{ ipc, flag::make( file::Flag::write_only, file::Flag::no_block)},
                     m_ipc{ std::move( ipc)}
               {
               }
   
            } // handle

            namespace native
            {
               namespace blocking
               {
                  bool receive( handle::Inbound& handle, message::Transport& transport)
                  {
                     Trace trace{ "common::communication::ipc::native::blocking::receive"};

                     auto descriptor = handle.descriptor().blocking();
                     log::line( verbose::log, "handle: ", handle);

                     return local::fifo::native::read( descriptor, transport);
                  }
               } // blocking
               namespace non
               {
                  namespace blocking
                  {
                     bool receive( handle::Inbound& handle, message::Transport& transport)
                     {
                        Trace trace{ "common::communication::ipc::native::non::blocking::receive"};

                        auto descriptor = handle.descriptor().non_blocking();
                        log::line( verbose::log, "handle: ", handle);

                        return local::fifo::native::read( descriptor, transport);
                     }
                  } // blocking
               } // non
            } // native

            namespace policy
            {
               namespace blocking
               {
                  cache_range_type receive( handle::Inbound& handle, cache_type& cache)
                  {
                     Trace trace{ "common::communication::ipc::policy::blocking::receive"};

                     message::Transport transport;

                     auto descriptor = handle.descriptor().blocking();
                     log::line( verbose::log, "handle: ", handle);

                     // We block and wait - with no signal data races
                     select::block::read( descriptor);

                     if( local::fifo::native::read( descriptor, transport))
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

                  Uuid send( const handle::Outbound& handle, const communication::message::Complete& complete)
                  {
                     Trace trace{ "common::communication::ipc::policy::blocking::send"};

                     message::Transport transport{ complete.type, complete.size()};
                     complete.correlation.copy( transport.correlation());

                     auto part_begin = std::begin( complete.payload);

                     auto descriptor = handle.descriptor().blocking();
                     log::line( verbose::log, "handle: ", handle);

                     do
                     {
                        auto part_end = std::distance( part_begin, std::end( complete.payload)) > message::transport::max_payload_size ?
                           part_begin + message::transport::max_payload_size : std::end( complete.payload);

                        transport.assign( range::make( part_begin, part_end));
                        transport.message.header.offset = std::distance( std::begin( complete.payload), part_begin);

                        // send the physical message
                        if( ! local::fifo::native::write( descriptor, transport))
                           return uuid::empty();

                        part_begin = part_end;
                     }
                     while( part_begin != std::end( complete.payload));

                     return complete.correlation;
                  }

               } // blocking

               namespace non
               {
                  namespace blocking
                  {
                     cache_range_type receive( handle::Inbound& handle, cache_type& cache)
                     {
                        Trace trace{ "common::communication::ipc::policy::non::blocking::receive"};

                        message::Transport transport;

                        auto descriptor = handle.descriptor().non_blocking();
                        log::line( verbose::log, "handle: ", handle);

                        if( local::fifo::native::read( descriptor, transport))
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

                     Uuid send( const handle::Outbound& handle, const communication::message::Complete& complete)
                     {
                        Trace trace{ "common::communication::ipc::policy::non::blocking::send"};

                        message::Transport transport{ complete.type, complete.size()};
                        complete.correlation.copy( transport.correlation());
                        auto part_begin = std::begin( complete.payload);

                        auto descriptor = handle.descriptor().non_blocking();
                        log::line( verbose::log, "handle: ", handle);

                        do
                        {
                           auto part_end = std::distance( part_begin, std::end( complete.payload)) > message::transport::max_payload_size ?
                                 part_begin + message::transport::max_payload_size : std::end( complete.payload);

                           transport.assign( range::make( part_begin, part_end));
                           transport.message.header.offset = std::distance( std::begin( complete.payload), part_begin);

                           // send the physical message
                           if( ! local::fifo::native::write( descriptor, transport))
                              return uuid::empty();

                           part_begin = part_end;
                        }
                        while( part_begin != std::end( complete.payload));

                        return complete.correlation;
                     }
                  } // blocking
               } // non


            } // policy

            namespace inbound
            {
               Device& device()
               {
                  static Device singleton;
                  return singleton;
               }
            } // inbound


            namespace outbound
            {
               Connector::Connector( strong::ipc::id ipc) 
                  : m_handle{ std::move( ipc)}
               {
               }

            } // outbound

            bool exists( strong::ipc::id ipc)
            {
               auto path = local::fifo::path( ipc);
               return ::access( path.c_str(), F_OK) != -1; 
            }

            bool remove( strong::ipc::id ipc)
            {
               auto path = local::fifo::path( ipc);
               if( posix::log::result( ::unlink( path.c_str()), "failed to unlink path: ", path) == -1)  
                  return false;
               log::line( verbose::log, "unlinked ", path);
               return true;
            }

            bool remove( const process::Handle& owner)
            {
               return remove( owner.ipc);
            }


         } // ipc
      } // communication
   } // common
} // casual
