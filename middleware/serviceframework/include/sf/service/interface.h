//!
//! casual
//!

#ifndef SF_SERVICE_INTERFACE_H_
#define SF_SERVICE_INTERFACE_H_


#include "sf/archive/archive.h"
#include "sf/log.h"
#include "sf/buffer.h"


#include "common/functional.h"

#include "xatmi.h"

//
// std
//
#include <memory>
#include <map>
#include <iosfwd>



namespace casual
{
   namespace sf
   {
      namespace service
      {
         namespace reply
         {
            struct State
            {
               int value = 0;
               long code = 0;
               char* data = nullptr;
               long size = 0;
               long flags = 0;

               friend std::ostream& operator << ( std::ostream& out, const State& state);
            };



         }

         class Interface
         {
         public:

            using readers_type = std::vector< archive::Reader*>;
            using writers_type = std::vector< archive::Writer*>;

            virtual ~Interface();


            bool call();

            reply::State finalize();

            void handle_exception();

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

            Input& input();
            Output& output();


         private:
            virtual bool do_call() = 0;
            virtual reply::State do_finalize() = 0;

            virtual Input& do_input() = 0;
            virtual Output& do_output() = 0;

            virtual void do_handle_exception() = 0;

         };

         class IO
         {
         public:

            typedef std::unique_ptr< service::Interface> interface_type;

            IO( interface_type interface);

            ~IO();

            IO( IO&&) = default;
            IO( const IO&) = delete;


            reply::State finalize();

            template< typename T>
            IO& operator >> ( T&& value)
            {
               serialize( std::forward< T>( value), m_interface->input());

               return *this;
            }

            template< typename T>
            IO& operator << ( T&& value)
            {
               serialize( std::forward< T>( value), m_interface->output());

               return *this;
            }


            template< typename F, typename... Args>
            auto call( F&& function, Args&&... args) -> decltype( common::invoke( std::forward< F>( function), std::forward< Args>( args)...))
            {
               sf::Trace trace{ "sf::service::IO::call"};

               if( call_implementation())
               {
                  try
                  {
                     return common::invoke( std::forward< F>( function), std::forward< Args>( args)...);
                  }
                  catch( ...)
                  {
                     handle_exception();
                  }
               }

               //
               // We return the default value for the return type
               //
               return decltype( common::invoke( std::forward< F>( function), std::forward< Args>( args)...))();
            }




         private:

            bool call_implementation();

            void handle_exception();


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

            interface_type m_interface;
         };


         class Factory
         {
         public:

            using function_type = std::function< std::unique_ptr< Interface>( TPSVCINFO*)>;

            struct Holder
            {
               std::string type;
               function_type create;
            };

            static Factory& instance();

            std::unique_ptr< Interface> create( TPSVCINFO* service_info, const std::string& type) const;
            std::unique_ptr< Interface> create( TPSVCINFO* service_info) const;


            template< typename T>
            void registration()
            {
               Holder holder;
               holder.type = T::type();
               holder.create = Creator< T>();

               m_factories.push_back( std::move( holder));
            }


         private:

            template< typename T>
            struct Creator
            {
               std::unique_ptr< Interface> operator()( TPSVCINFO* serviceInfo) const;
            };

            Factory();

            std::vector< Holder> m_factories;
         };

      } // service
   } // sf
} // casual



#endif /* SERVICE_H_ */
