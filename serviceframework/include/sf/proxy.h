//!
//! proxy.h
//!
//! Created on: Mar 1, 2014
//!     Author: Lazan
//!

#ifndef PROXY_H_
#define PROXY_H_


#include <memory>
#include <vector>

namespace casual
{
   namespace sf
   {
      namespace archive
      {
         class Writer;
         class Reader;
      } // archive

      namespace proxy
      {
         namespace IO
         {
            typedef std::vector< archive::Reader*> readers_type;
            typedef std::vector< archive::Writer*> writers_type;

            struct Input
            {
               writers_type writers;

               template< typename T>
               const Input& operator << ( T&& value) const
               {
                  for( auto&& writer : writers)
                  {
                     *writer << std::forward< T>( value);
                  }
                  return *this;
               }

            };

            struct Output
            {
               readers_type readers;
               writers_type writers;

               template< typename T>
               const Output& operator >> ( T&& value) const
               {
                  for( auto&& reader : readers)
                  {
                     *reader >> std::forward< T>( value);
                  }
                  for( auto&& writer : writers)
                  {
                     *writer << std::forward< T>( value);
                  }
                  return *this;
               }
            };
         } // IO

         class Async
         {
         public:

            Async( const std::string& service);
            Async( const std::string& service, long flags);
            ~Async();

            Async( const Async&) = delete;
            Async& operator = ( const Async&) = delete;

            Async& interface();

            void send();
            void receive();
            void finalize();


            template< typename T>
            const Async& operator << ( T&& value) const
            {
               input() << std::forward< T>( value);
               return *this;
            }

            template< typename T>
            const Async& operator >> ( T&& value) const
            {
               output() >> std::forward< T>( value);
               return *this;
            }

         private:
            const IO::Input& input() const;
            const IO::Output& output() const;

            class Implementation;
            std::unique_ptr< Implementation> m_implementation;
         };


         class Sync
         {
         public:

            Sync( const std::string& service);
            Sync( const std::string& service, long flags);
            ~Sync();

            Sync( const Async&) = delete;
            Sync& operator = ( const Async&) = delete;

            Sync& interface();


            void call();
            void finalize();


            template< typename T>
            const Sync& operator << ( T&& value) const
            {
               input() << std::forward< T>( value);
               return *this;
            }

            template< typename T>
            const Sync& operator >> ( T&& value) const
            {
               output() >> std::forward< T>( value);
               return *this;
            }

         private:
            const IO::Input& input() const;
            const IO::Output& output() const;


            class Implementation;
            std::unique_ptr< Implementation> m_implementation;
         };


      } // proxy
   } // sf


} // casual

#endif // PROXY_H_
