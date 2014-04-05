//!
//! proxy.cpp
//!
//! Created on: Mar 1, 2014
//!     Author: Lazan
//!

#include "sf/proxy.h"
#include "sf/archive.h"
#include "sf/archive_binary.h"
#include "sf/xatmi_call.h"
#include "sf/log.h"


namespace casual
{
   namespace sf
   {
      namespace proxy
      {

         namespace local
         {
            namespace
            {
               struct base_implementation
               {
                  base_implementation( const std::string& service, long flags)
                     : service( service), flags( flags)
                  {
                  }

                  IO::Input input;
                  IO::Output output;

                  std::string service;
                  long flags = 0;
               };

               template< typename I>
               struct ParameterLog : public I
               {
                  template< typename ...Args>
                  ParameterLog( Args... args) : I( std::forward< Args>( args)...), writer{ sf::log::parameter}
                  {
                     this->input.writers.push_back( &writer);
                     this->output.writers.push_back( &writer);
                  }

                  archive::log::Writer writer;
               };

            } // <unnamed>
         } // local


         /*
         struct Async::base_impl
         {
            virtual ~base_impl() = default;

            virtual void send() = 0;
            virtual void receive() = 0;
         };

         template< typename I>
         struct Async::basic_impl : public Async::base_impl
         {
            template< typename ...Args>
            basic_impl( Args... args) : m_implementation( std::forward< Args>( args)...) {}

            void send() override { m_implementation.send();}
            void receive() override { m_implementation.receive();}


            I m_implementation;
         };


         namespace local
         {
            namespace
            {
               namespace async
               {

                  struct Binary : local::base_implementation
                  {
                     typedef local::base_implementation base_type;

                     Binary( const std::string& service, long flags)
                      : base_type( service, flags), writer{ buffer}, reader{ buffer}
                     {

                     }

                     void send()
                     {
                        callDescriptor = xatmi::service::send( service, buffer, flags);

                     }

                     void receive()
                     {
                        if( ! xatmi::service::receive( callDescriptor, buffer, flags))
                        {
                           // TODO: somehow there's a noblock flag...
                        }
                     }

                     sf::xatmi::service::call_descriptor_type callDescriptor = 0;

                     buffer::Binary buffer;

                     archive::binary::Writer writer;
                     archive::binary::Reader reader;
                  };

                  std::unique_ptr< Async::base_impl> create( const std::string& service, long flags)
                  {
                     //
                     // We could create mockups and such...
                     //


                     if( log::parameter.good())
                     {
                        typedef ParameterLog< Async::basic_impl< Binary>> Implementation;
                        return std::unique_ptr< Async::base_impl>( new Implementation{ service, flags});
                     }

                     return std::unique_ptr< Async::base_impl>{ new Async::basic_impl< Binary>{ service, flags}};
                  }

               } // async
            } // <unnamed>
         } // local

         */

         /*
         Async::Async( const std::string& service) : Async( service, 0) {}

         Async::Async( const std::string& service, long flags) : m_implementation( local::async::create( service, flags))
         {

         }

         Async::~Async() {}

         Async& Async::interface()
         {
            return *this;
         }

         void Async::send()
         {
            m_implementation->send();
         }

         void Async::receive()
         {
            m_implementation->receive();
         }

         void Async::finalize()
         {

         }

         const IO::Input& Async::input() const
         {
            return m_implementation->input;
         }

         const IO::Output& Async::output() const
         {
            return m_implementation->output;
         }

         */



         namespace async
         {

            struct Result::impl_base
            {
               virtual ~impl_base() = default;
               virtual IO::Output& output() = 0;
            };

            namespace result
            {
               template< typename T>
               struct basic_impl : public Result::impl_base
               {
                  template< typename ...Args>
                  basic_impl( Args&&... args) : m_implemenentation( std::forward< Args>( args)...) {}

                  IO::Output& output() override
                  {
                     return m_implemenentation.output();
                  }
               private:
                  T m_implemenentation;
               };
            } // result


            struct Receive::impl_base
            {
               virtual ~impl_base() = default;
               virtual std::auto_ptr< Result::impl_base> receive() = 0;
            };

            namespace receive
            {
               template< typename T>
               struct basic_impl : public Receive::impl_base
               {
                  template< typename ...Args>
                  basic_impl( Args&&... args) : m_implemenentation( std::forward< Args>( args)...) {}

                  std::auto_ptr< Result::impl_base> receive() override
                  {
                     using result_type = decltype( m_implemenentation.receive());
                     return std::auto_ptr< Result::impl_base>{ new result::basic_impl< result_type>{ m_implemenentation.receive()}};
                  }
                  T m_implemenentation;
               };

            } // receive


            struct Send::impl_base
            {
               virtual ~impl_base() = default;
               virtual std::auto_ptr< Receive::impl_base> send() = 0;
               virtual IO::Input& input() = 0;
            };

            namespace send
            {
               template< typename T>
               struct basic_impl : public Send::impl_base
               {

                  template< typename ...Args>
                  basic_impl( Args&&... args) : m_implemenentation( std::forward< Args>( args)...) {}

                  std::auto_ptr< Receive::impl_base> send() override
                  {
                     using receive_type = decltype( m_implemenentation.send());
                     return std::auto_ptr< Receive::impl_base>{ new receive::basic_impl< receive_type>{ m_implemenentation.send()}};
                  }

                  IO::Input& input() override
                  {
                     return m_implemenentation.input();
                  }
               private:
                  T m_implemenentation;
               };
            } // send

            namespace protocol
            {
               namespace binary
               {
                  struct Result
                  {
                     Result( xatmi::service::call_descriptor_type cd, long flags)
                        : m_reader{ m_buffer}
                     {
                        m_output.readers.push_back( &m_reader);
                        xatmi::service::receive( cd, m_buffer, flags);
                     }

                     IO::Output& output() { return m_output;}

                  private:
                     buffer::Binary m_buffer;
                     archive::binary::Reader m_reader;
                     IO::Output m_output;
                  };

                  struct Receive
                  {
                     Receive( xatmi::service::call_descriptor_type cd, long flags)
                        : m_cd( cd), m_flags( flags) {}

                     Result receive()
                     {
                        return Result{ m_cd, m_flags};
                     }

                  private:
                     xatmi::service::call_descriptor_type m_cd;
                     long m_flags;
                  };

                  struct Send
                  {
                     Send( std::string service, long flags)
                        : m_service{ std::move( service)}, m_flags{ flags}, m_writer{ m_buffer}
                     {
                        m_input.writers.push_back( &m_writer);
                     }

                     Receive send()
                     {
                        buffer::Binary sendBuffer;
                        std::swap( m_buffer, sendBuffer);

                        return Receive{ xatmi::service::send( m_service, sendBuffer, m_flags), m_flags};
                     }

                     IO::Input& input()
                     {
                        return m_input;
                     }
                  private:

                     const std::string m_service;
                     const long m_flags;

                     buffer::Binary m_buffer;
                     archive::binary::Writer m_writer;
                     IO::Input m_input;
                  };
               } // binary

               namespace log
               {
                  template< typename P>
                  struct Result
                  {
                     template< typename ...Args>
                     Result( Args&&... args) : m_protocol( std::forward< Args>( args)...), m_writer{ common::log::parameter}
                     {
                        m_protocol.output().writers.push_back( &m_writer);
                     }

                     IO::Output& output()
                     {
                        return m_protocol.output();
                     }

                  private:
                     P m_protocol;
                     archive::log::Writer m_writer;
                  };

                  template< typename P>
                  struct Receive
                  {
                     using result_type = decltype( std::declval< P>().receive());

                     template< typename ...Args>
                     Receive( Args&&... args) : m_protocol( std::forward< Args>( args)...) {}

                     log::Result< result_type> receive()
                     {
                        return log::Result< result_type>( m_protocol.receive());
                     }

                  private:
                     P m_protocol;
                  };
                  template< typename P>
                  struct Send
                  {
                     using receive_type = decltype( std::declval< P>().send());

                     template< typename ...Args>
                     Send( Args&&... args) : m_protocol( std::forward< Args>( args)...), m_writer{ common::log::parameter}
                     {
                        m_protocol.input().writers.push_back( &m_writer);
                     }

                     Receive< receive_type> send()
                     {
                        return Receive< receive_type>{ m_protocol.send()};
                     }

                     IO::Input& input() { return m_protocol.input();}

                  private:
                     P m_protocol;
                     archive::log::Writer m_writer;
                  };
               } // log


               std::unique_ptr< Send::impl_base> create( std::string service, long flags)
               {
                  if( common::log::parameter.good())
                  {
                     return std::unique_ptr< Send::impl_base>{ new send::basic_impl< log::Send< binary::Send>>{ std::move( service), flags}};
                  }
                  return std::unique_ptr< Send::impl_base>{ new send::basic_impl< binary::Send>{ std::move( service), flags}};
               }
            } // protocol


            Send::Send( std::string service) : Send( std::move( service), 0) {}
            Send::Send( std::string service, long flags)
               : m_implementation( protocol::create( std::move( service), flags))
            {}

            Send::~Send() = default;
            Send::Send( Send&&) = default;

            Receive Send::operator() ( )
            {
               return Receive{ m_implementation->send()};
            }

            const IO::Input& Send::input() const
            {
               return m_implementation->input();
            }



            Result::~Result() = default;
            Result::Result( Result&&) = default;


            Result::Result( std::unique_ptr< impl_base>&& implementation)
               : m_implementation( std::move( implementation)) {}


            const IO::Output& Result::output() const
            {
               return m_implementation->output();
            }


            Receive::Receive(  std::unique_ptr< impl_base>&& implementation)
             : m_implementation( std::move( implementation)) {}

            Receive::~Receive() = default;
            Receive::Receive( Receive&&) = default;


            Result Receive::operator()()
            {
               return Result{ m_implementation->receive()};
            }

         } // async



         struct Sync::Implementation : public local::base_implementation
         {
            using local::base_implementation::base_implementation;

            void call()
            {
               xatmi::service::call( service, buffer, buffer, flags);
            }


            buffer::Binary buffer;

            archive::binary::Writer writer = archive::binary::Writer{ buffer};
            archive::binary::Reader reader = archive::binary::Reader{ buffer};

         };


         Sync::Sync( const std::string& service) : Sync( service, 0) {}

         Sync::Sync( const std::string& service, long flags) : m_implementation( new Implementation( service, flags))
         {

         }

         Sync::~Sync() {}

         Sync& Sync::interface()
         {
            return *this;
         }

         void Sync::call()
         {
            m_implementation->call();
         }

         void Sync::finalize()
         {

         }

         const IO::Input& Sync::input() const
         {
            return m_implementation->input;
         }

         const IO::Output& Sync::output() const
         {
            return m_implementation->output;
         }

      } // proxy
   } // sf


} // casual

