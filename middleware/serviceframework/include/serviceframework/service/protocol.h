//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/serialize/archive.h"

#include "common/service/invoke.h"
#include "common/functional.h"


#include <vector>

namespace casual
{
   namespace serviceframework
   {
      namespace service
      {
         namespace protocol
         {
            using parameter_type = common::service::invoke::Parameter;
            using result_type = common::service::invoke::Result;

            namespace io
            {
               using readers_type = std::vector< common::serialize::Reader*>;
               using writers_type = std::vector< common::serialize::Writer*>;

               struct Input
               {
                  readers_type readers;
                  writers_type writers;
               };

               struct Output
               {
                  readers_type readers;
                  writers_type writers;
               };

            } // io

         } // protocol


         class Protocol
         {
         public:

            template< typename P>
            Protocol( P&& protocol)
               : Protocol{ std::make_unique< model< P>>( std::move( protocol))}
            {}

            ~Protocol();

            Protocol( Protocol&&);
            Protocol& operator = ( Protocol&&);


            bool call() const { return m_concept->call();}

            protocol::result_type finalize() { return m_concept->finalize();}
            void exception() { m_concept->exception();}
            const std::string& type() const { return m_concept->type();}
            
            //! extract type `R` with `name`
            template< typename R, typename N> 
            auto extract( N&& name)
            {
               R result{};
               *this >> CASUAL_NAMED_VALUE_NAME( result, name);
               return result;
            }

            //! extract type `R`
            template< typename R> 
            auto extract()
            {
               R result;
               *this >> result;
               return result;
            }

            template< typename T>
            Protocol& operator >> ( T&& value)
            {
               serialize( std::forward< T>( value), m_concept->input());
               return *this;
            }

            template< typename T>
            Protocol& operator << ( T&& value)
            {
               // check if user uses const lvalues for _result_. Don't really see a
               // use case when this would be natural, based on the intent of the 
               // `Protocol` type. But apparently there are... 
               if constexpr ( std::is_const_v< std::remove_reference_t< T>>)
               {
                  auto& io = m_concept->output();

                  if( io.readers.empty())
                  {
                     for( auto& archive : io.writers)
                        *archive << value;
                  }
                  else
                  {
                     // need to copy to enable protocols that mutates the value (describe is one).
                     auto non_const = value;
                     serialize( non_const, io);
                  }
               }
               else // use the effective, normal branch
                  serialize( std::forward< T>( value), m_concept->output());
               return *this;
            }

            template< typename P, typename... Args>
            static Protocol emplace( Args&&... args)
            {
               return { std::make_unique< model< P>>( std::forward< Args>( args)...)};
            }

            friend std::ostream& operator << ( std::ostream& out, const Protocol& value);

            //! @attention internal use only
            //! @{
            auto& input() { return m_concept->input();}
            auto& output() { return m_concept->output();}
            //! @}

         private:

            struct concept
            {
               virtual ~concept() = default;
               virtual protocol::io::Input& input() = 0;
               virtual protocol::io::Output& output() = 0;
               virtual bool call() = 0;
               virtual protocol::result_type finalize() = 0;
               virtual void exception() = 0;

               virtual const std::string& type() const = 0;
            };

            template< typename P>
            Protocol( std::unique_ptr< P>&& concept) : m_concept( std::move( concept)) {}

            template< typename Protocol>
            struct model : concept
            {
               template< typename... Args>
               model( Args&&... args) : m_protocol{ std::forward< Args>( args)...} {}

               protocol::io::Input& input() override { return m_protocol.input();}
               protocol::io::Output& output() override { return m_protocol.output();}
               bool call() override { return m_protocol.call();}
               protocol::result_type finalize() override { return m_protocol.finalize();}
               void exception() override { m_protocol.exception();}

               const std::string& type() const override { return m_protocol.type();}

            private:
               Protocol m_protocol;
            };

            template< typename T, typename A>
            void serialize( T&& value, A& io)
            {
               for( auto& archive : io.readers)
                  *archive >> value;
               for( auto& archive : io.writers)
                  *archive << value;
            }

            std::unique_ptr< concept> m_concept;
         };

         namespace protocol
         {
            class Factory
            {
            public:
               static Factory& instance();

               using creator_type = std::function< Protocol( protocol::parameter_type&&)>;

               Protocol create( protocol::parameter_type&& parameter);

               template< typename Protocol>
               const std::string& registration( const std::string& type)
               {
                  m_creators[ type] = Creator< Protocol>{};
                  return type;
               }

               template< typename Protocol>
               const std::string& registration()
               {
                  m_creators[ Protocol::type()] = Creator< Protocol>{};
                  return Protocol::type();
               }

            private:

               template< typename P>
               struct Creator
               {
                  using protocol_type = P;

                  service::Protocol operator()( protocol::parameter_type&& parameter) const
                  {
                     return service::Protocol::emplace< protocol_type>( std::move( parameter));
                  }

               };

               Factory();

               using mapping_type = std::map< std::string, creator_type>;

               mapping_type m_creators;
            };

            //! @returns a protocol deduced from `parameter`
            Protocol deduce( protocol::parameter_type&& parameter);

         } // protocol


         template< typename F, typename... Args>
         auto user( Protocol& protocol, F&& function, Args&&... args)
         {
            if( protocol.call())
            {
               try
               {
                  return common::invoke( function, std::forward< Args>( args)...);
               }
               catch( ...)
               {
                  protocol.exception();
               }
            }
            return decltype( common::invoke( function, std::forward< Args>( args)...))();
         }

         namespace detail
         {
            template< typename R> 
            struct user 
            {
               template< typename F, typename... Args>
               static auto call( Protocol&& protocol, F&& function, Args&&... args)
               {
                  auto result = service::user( protocol, std::forward< F>( function), std::forward< Args>( args)...);
                  protocol << CASUAL_NAMED_VALUE( result);
                  return protocol.finalize();
               }
            };

            template<> 
            struct user< void>
            {
               template< typename F, typename... Args>
               static auto call( Protocol&& protocol, F&& function, Args&&... args)
               {
                  service::user( protocol, std::forward< F>( function), std::forward< Args>( args)...);
                  return protocol.finalize();
               }
            };

         } // detail
         
         //! takes ownership of protocol and serializes the result (if not void) and return 
         //! common::service::invoke::Result
         template< typename F, typename... Args>
         auto user( Protocol&& protocol, F&& function, Args&&... args)
         {
            using implementation = detail::user< decltype( common::invoke( function, std::forward< Args>( args)...))>;
            return implementation::call( std::move( protocol), std::forward< F>( function), std::forward< Args>( args)...);
         }

         //! takes ownership of `parameter` and deduces protocol and serializes the result (if not void) and return 
         //! common::service::invoke::Result
         template< typename... Ts>
         auto user( common::service::invoke::Parameter&& parameter, Ts&&... ts)
         {
            return service::user( protocol::deduce( std::move( parameter)), std::forward< Ts>( ts)...);
         }

      } // service
   } // serviceframework
} // casual


