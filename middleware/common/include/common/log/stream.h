//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/stream.h"


#include <string>
#include <ostream>
#include <mutex>

namespace casual
{
   namespace common
   {
      namespace log
      {
         class Stream : public std::ostream
         {
         public:

            Stream( std::string category);

            //! deleted - use streams only with common::log::line or common::log::write
            template< typename T>
            Stream& operator << ( T&& value) = delete;
         };

         namespace stream
         {
            namespace thread
            {
               class Lock
               {
               public:
                  inline Lock() : m_lock( m_mutex) {}

               private:
                  std::unique_lock< std::mutex> m_lock;
                  static std::mutex m_mutex;
               };
            } // thread

            //! @returns the corresponding stream for the @p category
            Stream& get( const std::string& category);

            //! @return true if the log-category is active.
            bool active( const std::string& category);

            void activate( const std::string& category);
            void deactivate( const std::string& category);
            void write( const std::string& category, const std::string& message);

            //! reopens the logfile. usefull for 'log-rotations'.
            void reopen();

         } // stream


         template< typename... Args>
         void write( std::ostream& stream, Args&&... args)
         {
            if( stream)
            {
               stream::thread::Lock lock;
               common::stream::write( stream, std::forward< Args>( args)...);
            }
         }

         template< typename... Args>
         void line( std::ostream& stream, Args&&... args)
         {
            if( stream)
            {
               stream::thread::Lock lock;
               common::stream::write( stream, std::forward< Args>( args)..., '\n');
            }
         } 

      } // log
   } // common
} // casual



