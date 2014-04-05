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

#include "sf/xatmi_call.h"

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

         /*
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


            class base_impl;

            template< typename I>
            class basic_impl;

         private:
            const IO::Input& input() const;
            const IO::Output& output() const;

            std::unique_ptr< base_impl> m_implementation;
         };
         */

         namespace async
         {

            class Result
            {
            public:

               ~Result();
               Result( Result&&);

               template< typename T>
               const Result& operator >> ( T&& value) const
               {
                  output() >> std::forward< T>( value);
                  return *this;
               }

               class impl_base;

            private:

               friend class Receive;
               Result( std::unique_ptr< impl_base>&& implementation);


               const IO::Output& output() const;

               std::unique_ptr< impl_base> m_implementation;

            };

            class Receive
            {
            public:

               ~Receive();
               Receive( Receive&&);

               Result operator()();

               class impl_base;

            private:

               friend class Send;
               Receive( std::unique_ptr< impl_base>&& implementation);

               std::unique_ptr< impl_base> m_implementation;
            };

            class Send
            {
            public:
               Send( std::string service);
               Send( std::string service, long flags);

               ~Send();
               Send( Send&&);


               template< typename T>
               const Send& operator << ( T&& value) const
               {
                  input() << std::forward< T>( value);
                  return *this;
               }

               Receive operator() ( );

               class impl_base;
            private:

               const IO::Input& input() const;


               std::unique_ptr< impl_base> m_implementation;
            };
         } // async


         class Sync
         {
         public:

            Sync( const std::string& service);
            Sync( const std::string& service, long flags);
            ~Sync();

            Sync( const Sync&) = delete;
            Sync& operator = ( const Sync&) = delete;

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
