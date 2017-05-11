//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_SERVICEFRAMEWORK_INCLUDE_SF_SERVICE_IO_H_
#define CASUAL_MIDDLEWARE_SERVICEFRAMEWORK_INCLUDE_SF_SERVICE_IO_H_


#include "sf/archive/archive.h"

#include "common/service/invoke.h"
#include "common/functional.h"


#include <vector>

namespace casual
{
   namespace sf
   {
      namespace service
      {
         namespace protocol
         {
            using parameter_type = common::service::invoke::Parameter;
            using result_type = common::service::invoke::Result;

            namespace io
            {
               using readers_type = std::vector< archive::Reader*>;
               using writers_type = std::vector< archive::Writer*>;

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
               : Protocol{ std::make_unique< Implementation< P>>( std::move( protocol))}
            {
            }


            ~Protocol();

            Protocol( Protocol&&);
            Protocol& operator = ( Protocol&&);


            bool call() const { return m_implementaion->call();}

            protocol::result_type finalize() { return m_implementaion->finalize();}
            void exception() { m_implementaion->exception();}
            const std::string& type() const { return m_implementaion->type();}

            template< typename T>
            Protocol& operator >> ( T&& value)
            {
               serialize( std::forward< T>( value), m_implementaion->input());
               return *this;
            }

            template< typename T>
            Protocol& operator << ( T&& value)
            {
               serialize( std::forward< T>( value), m_implementaion->output());
               return *this;
            }

            template< typename P, typename... Args>
            static Protocol emplace( Args&&... args)
            {
               return { std::make_unique< Implementation< P>>( std::forward< Args>( args)...)};
            }

         private:

            struct Base
            {
               virtual protocol::io::Input& input() = 0;
               virtual protocol::io::Output& output() = 0;
               virtual bool call() const = 0;
               virtual protocol::result_type finalize() = 0;
               virtual void exception() = 0;

               virtual const std::string& type() const = 0;
            };

            template< typename P>
            Protocol( std::unique_ptr< P>&& implementation) : m_implementaion( std::move( implementation)) {}

            template< typename Protocol>
            struct Implementation : Base
            {
               template< typename... Args>
               Implementation( Args&&... args) : m_protocol{ std::forward< Args>( args)...} {}

               protocol::io::Input& input() override { return m_protocol.input();}
               protocol::io::Output& output() override { return m_protocol.output();}
               bool call() const override { return m_protocol.call();}
               protocol::result_type finalize() override { return m_protocol.finalize();}
               void exception() override { m_protocol.exception();}

               const std::string& type() const override { return m_protocol.type();}

            private:
               Protocol m_protocol;
            };

            template< typename T, typename A>
            void serialize( T&& value, A& io)
            {
               for( auto&& archive : io.readers)
               {
                  *archive >> std::forward< T>( value);
               }

               for( auto&& archive : io.writers)
               {
                  *archive << std::forward< T>( value);
               }
            }

            std::unique_ptr< Base> m_implementaion;

         };


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
            return decltype( function( std::forward< Args>( args)...))();
         }



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

            Protocol deduce( protocol::parameter_type&& parameter);

         } // protocol

      } // service
   } // sf
} // casual

#endif // CASUAL_MIDDLEWARE_SERVICEFRAMEWORK_INCLUDE_SF_SERVICE_IO_H_
