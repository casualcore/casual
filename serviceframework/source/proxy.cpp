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

               template< typename C>
               struct basic_protocol
               {
                  using send_type = C;
                  using receive_type = decltype( std::declval< send_type>()());
                  using result_type = decltype( std::declval< receive_type>()());


                  struct Result
                  {
                     template< typename ...Args>
                     Result( Args&&... args) : m_result( std::forward< Args>( args)...)
                     {
                        m_output.readers.push_back( &m_result.policy().archive());
                     }

                     IO::Output& output() { return m_output;}

                  private:
                     result_type m_result;
                     IO::Output m_output;
                  };

                  struct Receive
                  {
                     template< typename ...Args>
                     Receive( Args&&... args) : m_receivce( std::forward< Args>( args)...) {}

                     Result receive()
                     {
                        return Result{ m_receivce()};
                     }

                  private:
                     receive_type m_receivce;
                  };

                  struct Send
                  {
                     template< typename ...Args>
                     Send( Args&&... args) : m_send( std::forward< Args>( args)...)
                     {
                        m_input.writers.push_back( &m_send.policy().archive());
                     }

                     Receive send()
                     {
                        return Receive{ m_send()};
                     }

                     IO::Input& input()
                     {
                        return m_input;
                     }
                  private:

                     send_type m_send;
                     IO::Input m_input;
                  };

               };

               typedef basic_protocol< xatmi::service::binary::Async> Binary;

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
                  //
                  // We could create other protocols later, including mockup.
                  //

                  if( common::log::parameter.good())
                  {
                     return std::unique_ptr< Send::impl_base>{ new send::basic_impl< log::Send< Binary::Send>>{ std::move( service), flags}};
                  }
                  return std::unique_ptr< Send::impl_base>{ new send::basic_impl< Binary::Send>{ std::move( service), flags}};
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


         namespace sync
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



            struct Call::impl_base
            {
               virtual ~impl_base() = default;
               virtual std::auto_ptr< Result::impl_base> call() = 0;
               virtual IO::Input& input() = 0;
            };

            namespace send
            {
               template< typename T>
               struct basic_impl : public Call::impl_base
               {

                  template< typename ...Args>
                  basic_impl( Args&&... args) : m_implemenentation( std::forward< Args>( args)...) {}

                  std::auto_ptr< Result::impl_base> call() override
                  {
                     using receive_type = decltype( m_implemenentation.call());
                     return std::auto_ptr< Result::impl_base>{ new result::basic_impl< receive_type>{ m_implemenentation.call()}};
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

               template< typename C>
               struct basic_protocol
               {
                  using call_type = C;
                  using result_type = decltype( std::declval< call_type>()());


                  struct Result
                  {
                     template< typename ...Args>
                     Result( Args&&... args) : m_result( std::forward< Args>( args)...)
                     {
                        m_output.readers.push_back( &m_result.policy().archive());
                     }

                     IO::Output& output() { return m_output;}

                  private:
                     result_type m_result;
                     IO::Output m_output;
                  };


                  struct Call
                  {
                     template< typename ...Args>
                     Call( Args&&... args) : m_call( std::forward< Args>( args)...)
                     {
                        m_input.writers.push_back( &m_call.policy().archive());
                     }

                     Result send()
                     {
                        return Result{ m_call()};
                     }

                     IO::Input& input()
                     {
                        return m_input;
                     }
                  private:

                     call_type m_call;
                     IO::Input m_input;
                  };

               };
            }


            Result::~Result() = default;
            Result::Result( Result&&) = default;

            Result::Result( std::unique_ptr< impl_base>&& implementation)
            {
            }

            const IO::Output& Result::output() const
            {
               return m_implementation->output();
            }

            Call::Call( std::string service) : Call( std::move( service), 0L) {}

            Call::Call( std::string service, long flags)
            {
            }

            Call::~Call() = default;
            Call::Call( Call&&) = default;

            Result Call::operator ()()
            {
               return Result{ m_implementation->call()};
            }

            const IO::Input& Call::input() const
            {
               return m_implementation->input();
            }

         } // sync
      } // proxy
   } // sf


} // casual


