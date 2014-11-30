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

         namespace sync
         {
            class Service;
         }
         namespace async
         {
            class Receive;
         } // async

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

            friend class sync::Service;
            friend class async::Receive;
            Result( std::unique_ptr< impl_base>&& implementation);

            const IO::Output& output() const;
            std::unique_ptr< impl_base> m_implementation;
         };

         namespace async
         {

            class Receive
            {
            public:

               ~Receive();
               Receive( Receive&&);

               Result operator()();

               class impl_base;

            private:

               friend class Service;
               Receive( std::unique_ptr< impl_base>&& implementation);

               std::unique_ptr< impl_base> m_implementation;
            };

            class Service
            {
            public:
               Service( std::string service);
               Service( std::string service, long flags);

               ~Service();
               Service( Service&&);


               template< typename T>
               const Service& operator << ( T&& value) const
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


         namespace sync
         {

            class Service
            {
            public:
               Service( std::string service);
               Service( std::string service, long flags);

               ~Service();
               Service( Service&&);


               template< typename T>
               const Service& operator << ( T&& value) const
               {
                  input() << std::forward< T>( value);
                  return *this;
               }

               Result operator() ();

               class impl_base;

            private:
               const IO::Input& input() const;
               std::unique_ptr< impl_base> m_implementation;
            };


         } // sync

      } // proxy
   } // sf


} // casual

#endif // PROXY_H_
