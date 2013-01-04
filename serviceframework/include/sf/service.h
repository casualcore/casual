//!
//! service.h
//!
//! Created on: Dec 27, 2012
//!     Author: Lazan
//!

#ifndef SERVICE_H_
#define SERVICE_H_


#include "sf/archive.h"

namespace casual
{
   namespace sf
   {
      namespace service
      {
         class Interface
         {
         public:

            typedef std::vector< archive::Reader*> readers_type;
            typedef std::vector< archive::Writer*> writers_type;

            virtual ~Interface();

            bool call();
            void finalize();

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
            virtual void doFinalize() = 0;

            virtual Input& doInput() = 0;
            virtual Output& doOutput() = 0;

         };

         class IO
         {
         public:

            IO( Interface& interface) : m_interface( interface) {}

            template< typename T>
            IO& operator >> ( T&& value)
            {
               serialize( std::forward< T>( value), m_interface.input());

               return *this;
            }

            template< typename T>
            IO& operator << ( T&& value)
            {
               serialize( std::forward< T>( value), m_interface.output());

               return *this;
            }

            void finalize()
            {
               m_interface.finalize();
            }

         private:

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


            Interface& m_interface;
         };

      } // service
   } // sf
} // casual



#endif /* SERVICE_H_ */
