//!
//! casual
//!

#include "sf/proxy.h"
#include "sf/archive/archive.h"
#include "sf/archive/binary.h"
#include "sf/xatmi_call.h"
#include "sf/log.h"


namespace casual
{
   namespace sf
   {
      namespace proxy
      {



         //!
         //! Used by both sync and asynk
         //!
         //! @{
         class Result::impl_base
         {
         public:
            virtual ~impl_base() = default;
            virtual IO::Output& output() = 0;
         };



         Result::~Result() = default;
         Result::Result( Result&&) = default;

         Result::Result( std::unique_ptr< impl_base>&& implementation)
            : m_implementation{ std::move( implementation)}
         {
         }

         const IO::Output& Result::output() const
         {
            return m_implementation->output();
         }


         namespace result
         {
            template< typename T>
            struct basic_impl : public Result::impl_base
            {
               template< typename ...Args>
               basic_impl( Args&&... args) : m_implemenentation( std::forward< Args>( args)...)
               {
                  m_output.readers.push_back( &m_implemenentation.policy().archive());
               }

               IO::Output& output() override
               {
                  return m_output;
               }
            private:
               T m_implemenentation;
               IO::Output m_output;
            };


            template< typename T>
            struct basic_log : public result::basic_impl< T>
            {
               using base_type = result::basic_impl< T>;
               template< typename ...Args>
               basic_log( Args&&... args) : base_type( std::forward< Args>( args)...), m_writer{ common::log::parameter}
               {
                  this->output().writers.push_back( &m_writer);
               }

            private:
               archive::log::Writer m_writer;
            };
         } // result
         //!
         //! @}

         namespace async
         {

            class Receive::impl_base
            {
            public:
               virtual ~impl_base() = default;
               virtual std::unique_ptr< Result::impl_base> receive() = 0;
            };

            namespace receive
            {
               template< typename T>
               struct basic_impl : public Receive::impl_base
               {
                  template< typename ...Args>
                  basic_impl( Args&&... args) : m_implemenentation( std::forward< Args>( args)...) {}

                  std::unique_ptr< Result::impl_base> receive() override
                  {
                     using result_type = decltype( m_implemenentation());
                     return std::make_unique< result::basic_impl< result_type>>( m_implemenentation());
                  }
                  T m_implemenentation;
               };

            } // receive


            class Service::impl_base
            {
            public:
               virtual ~impl_base() = default;
               virtual std::unique_ptr< Receive::impl_base> call() = 0;
               virtual IO::Input& input() = 0;
            };

            namespace service
            {
               template< typename T>
               struct basic_impl : public Service::impl_base
               {

                  template< typename ...Args>
                  basic_impl( Args&&... args) : m_implemenentation( std::forward< Args>( args)...)
                  {
                     m_input.writers.push_back( &m_implemenentation.policy().archive());
                  }

                  std::unique_ptr< Receive::impl_base> call() override
                  {
                     using receive_type = decltype( m_implemenentation());
                     return std::make_unique< receive::basic_impl< receive_type>>( m_implemenentation());
                  }

                  IO::Input& input() override
                  {
                     return m_input;
                  }
               private:
                  T m_implemenentation;
                  IO::Input m_input;
               };
            } // service

            namespace protocol
            {

               typedef service::basic_impl< xatmi::service::binary::Async> Binary;

               /*
               template< typename P>
               struct basic_log
               {
                  using protocol_type = P;
                  using receive_type = decltype( std::declval< protocol_type>()());
                  using result_type = decltype( std::declval< receive_type>()());


                  using Result = result::basic_log< result_type>;


                  struct Receive : public receive::basic_impl< receive_type>
                  {
                     using base_type = receive::basic_impl< receive_type>;

                     using base_type::base_type;

                     std::unique_ptr< Result::impl_base> receive() override
                     {

                     }

                  private:
                     P m_protocol;
                  };
                  template< typename P>
                  struct Service
                  {
                     using receive_type = decltype( std::declval< P>().call());

                     template< typename ...Args>
                     Service( Args&&... args) : m_protocol( std::forward< Args>( args)...), m_writer{ common::log::parameter}
                     {
                        m_protocol.input().writers.push_back( &m_writer);
                     }

                     Receive< receive_type> call()
                     {
                        return Receive< receive_type>{ m_protocol.call()};
                     }

                     IO::Input& input() { return m_protocol.input();}

                  private:
                     P m_protocol;
                     archive::log::Writer m_writer;
                  };
               }; // basic_log
               */


               std::unique_ptr< Service::impl_base> create( std::string service, long flags)
               {
                  //
                  // We could create other protocols later, including mockup.
                  //
                  return std::make_unique< Binary>( std::move( service), flags);
               }
            } // protocol


            Service::Service( std::string service) : Service( std::move( service), 0) {}

            Service::Service( std::string service, long flags)
               : m_implementation( protocol::create( std::move( service), flags))
            {}

            Service::~Service() = default;
            Service::Service( Service&&) = default;

            Receive Service::operator() ( )
            {
               return Receive{ m_implementation->call()};
            }

            const IO::Input& Service::input() const
            {
               return m_implementation->input();
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

            class Service::impl_base
            {
            public:
               virtual ~impl_base() = default;
               virtual std::unique_ptr< Result::impl_base> call() = 0;
               virtual IO::Input& input() = 0;
            };

            namespace service
            {
               template< typename T>
               struct basic_impl : public Service::impl_base
               {

                  template< typename ...Args>
                  basic_impl( Args&&... args) : m_implemenentation( std::forward< Args>( args)...)
                  {
                     m_input.writers.push_back( &m_implemenentation.policy().archive());
                  }

                  std::unique_ptr< Result::impl_base> call() override
                  {
                     using receive_type = decltype( m_implemenentation());
                     return std::make_unique< result::basic_impl< receive_type>>( m_implemenentation());
                  }

                  IO::Input& input() override
                  {
                     return m_input;
                  }
               private:
                  T m_implemenentation;
                  IO::Input m_input;
               };
            } // service


            namespace protocol
            {
               typedef service::basic_impl< xatmi::service::binary::Sync> Binary;

               std::unique_ptr< Service::impl_base> create( std::string service, long flags)
               {
                  //
                  // We could create other protocols later, including mockup.
                  //
                  return std::make_unique< Binary>( std::move( service), flags);
               }
            }




            Service::Service( std::string service) : Service( std::move( service), 0L) {}

            Service::Service( std::string service, long flags)
               : m_implementation{ protocol::create( std::move( service), flags)}
            {
            }

            Service::~Service() = default;
            Service::Service( Service&&) = default;

            Result Service::operator ()()
            {
               return Result{ m_implementation->call()};
            }

            const IO::Input& Service::input() const
            {
               return m_implementation->input();
            }

         } // sync
      } // proxy
   } // sf


} // casual


