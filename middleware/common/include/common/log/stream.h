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
   namespace common::log
   {

      struct Stream : std::ostream
      {
         Stream( std::string category);
         ~Stream();

         //! deleted - use streams only with common::log::line or common::log::write
         template< typename T>
         Stream& operator << ( T&& value) = delete;

         //! creates an 'inactive' stream. Only for internal use.
         Stream();
      };

      namespace stream
      {
         namespace thread
         {
            struct Lock : traits::unrelocatable
            {
               inline Lock() : m_lock( m_mutex) {}

            private:
               std::unique_lock< std::mutex> m_lock;
               static std::mutex m_mutex;
            };
         } // thread

         //! @returns the corresponding stream for the @p category
         Stream& get( std::string_view category);

         //! @return true if the log-category is active.
         bool active( std::string_view category) noexcept;

         void activate( std::string_view expression) noexcept;
         void deactivate( std::string_view expression) noexcept;
         void write( std::string_view category, const std::string& message);

         //! reopens the logfile on the next write. useful for 'log-rotations'.
         void reopen();

         const std::filesystem::path& path() noexcept;

         struct Configure
         {
            std::optional< std::filesystem::path> path;

            struct
            {
               std::optional< std::string> inclusive;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( inclusive);
               )
            } expression;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( path);
               CASUAL_SERIALIZE( expression);
            );
         };

         void configure( const Configure& configure);

      } // stream


      template< typename... Args>
      void write( std::ostream& out, Args&&... args)
      {
         if( out)
         {
            stream::thread::Lock lock;
            common::stream::write( out, std::forward< Args>( args)...);
         }
      }

   } // common::log
} // casual
