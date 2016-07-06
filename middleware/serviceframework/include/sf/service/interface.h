//!
//! casual
//!

#ifndef SF_SERVICE_INTERFACE_H_
#define SF_SERVICE_INTERFACE_H_


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

            virtual void do_andle_exception() = 0;

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
               if( call_implementation())
               {
                  auto function = std::mem_fn( memberfunction);

                  try
                  {
                     return function( implementation, std::forward< Args>( args)...);
                  }
                  catch( ...)
                  {
                     handle_exception();
                  }
               }

               //
               // We return the default value for the return type
               //
               return decltype( std::mem_fn( memberfunction)( implementation, std::forward< Args>( args)...))();
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

         namespace factory
         {
            namespace buffer
            {
               struct Type
               {
                  using equality_type = std::function< bool( const sf::buffer::Type&, const sf::buffer::Type&)>;

                  Type() = default;
                  inline Type( sf::buffer::Type type) : type{ std::move( type)}, equal{ &default_eqaul} {}
                  inline Type( sf::buffer::Type type, equality_type equal) : type{ std::move( type)}, equal{ std::move( equal)} {}

                  sf::buffer::Type type;
                  equality_type equal;

                  inline friend bool operator == ( const Type& lhs, const Type& rhs) { return lhs.equal( lhs.type, rhs.type);}
                  inline friend bool operator == ( const Type& lhs, const sf::buffer::Type& rhs) { return lhs.equal( lhs.type, rhs);}
                  inline friend bool operator == ( const sf::buffer::Type& lhs, const Type& rhs) { return rhs.equal( lhs, rhs.type);}

                  static bool default_eqaul( const sf::buffer::Type& l, const sf::buffer::Type& r) { return l == r;}
               };

            } // buffer



         } // factory

         class Factory
         {
         public:

            using function_type = std::function< std::unique_ptr< Interface>( TPSVCINFO*)>;

            struct Holder
            {
               factory::buffer::Type type;
               function_type create;
            };

            static Factory& instance();

            std::unique_ptr< Interface> create( TPSVCINFO* service_info, const buffer::Type& type) const;
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
