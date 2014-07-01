//!
//! service.h
//!
//! Created on: Dec 27, 2012
//!     Author: Lazan
//!

#ifndef SERVICE_H_
#define SERVICE_H_


#include "sf/archive/archive.h"
#include "sf/buffer.h"

#include "xatmi.h"

//
// std
//
#include <memory>
#include <map>



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
            };
         }

         class Interface
         {
         public:

            typedef std::vector< archive::Reader*> readers_type;
            typedef std::vector< archive::Writer*> writers_type;

            virtual ~Interface();


            bool call();

            reply::State finalize();

            void handleException();

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
            virtual bool doCall() = 0;
            virtual reply::State doFinalize() = 0;

            virtual Input& doInput() = 0;
            virtual Output& doOutput() = 0;

            virtual void doHandleException() = 0;

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

            template< typename I, typename M, typename... Args>
            auto call( I& implementation, M memberfunction, Args&&... args) -> decltype( std::mem_fn( memberfunction)( implementation, std::forward< Args>( args)...))
            {
               if( callImplementation())
               {
                  auto function = std::mem_fn( memberfunction);

                  try
                  {
                     return function( implementation, std::forward< Args>( args)...);
                  }
                  catch( ...)
                  {
                     handleException();
                  }
               }

               //
               // Vi return the default value for the return type
               //
               return decltype( std::mem_fn( memberfunction)( implementation, std::forward< Args>( args)...))();
            }

         private:

            bool callImplementation();

            void handleException();


            template< typename T, typename A>
            void serialize( T&& value, A& io)
            {
               for( auto archive : io.readers)
               {
                  *archive >> std::forward< T>( value);
               }

               for( auto archive : io.writers)
               {
                  *archive << std::forward< T>( value);
               }
            }

            interface_type m_interface;
         };

         class Factory
         {
         public:

            typedef std::function< std::unique_ptr< Interface>( TPSVCINFO* serviceInfo)> function_type;




            static Factory& instance();

            std::unique_ptr< Interface> create( TPSVCINFO* serviceInfo) const;


            template< typename T>
            void registrate( sf::buffer::Type&& type)
            {
               m_factories[ std::move( type)] = Creator< T>();
            }


         private:

            template< typename T>
            struct Creator
            {
               std::unique_ptr< Interface> operator()( TPSVCINFO* serviceInfo) const;
            };

            Factory();

            std::map< sf::buffer::Type, function_type> m_factories;
         };

      } // service
   } // sf
} // casual



#endif /* SERVICE_H_ */
