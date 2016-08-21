//!
//! casual
//!

#ifndef COMMON_LOG_H
#define COMMON_LOG_H



#include "common/platform.h"


#include <string>
#include <ostream>
#include <mutex>
#include <bitset>

namespace casual
{
   namespace common
   {
      namespace log
      {

         namespace thread
         {
            class Safe
            {
            public:
               Safe( std::ostream& stream) : m_stream( stream), m_lock( m_mutex) {}
               Safe( Safe&&) = default;


               template< typename T>
               Safe& operator << ( T&& value)
               {
                  m_stream << std::forward< T>( value);
                  return *this;
               }


               typedef std::ostream& (&omanip_t)( std::ostream&);

               //!
               //! Overload for manip-functions...
               //! @note Why does not the T&& take these?
               //!
               Safe& operator << ( omanip_t value)
               {
                  m_stream << value;
                  return *this;
               }

            private:
               std::ostream& m_stream;
               std::unique_lock< std::mutex> m_lock;

               static std::mutex m_mutex;
            };


         } // thread


         class Stream : public std::ostream
         {
         public:

            Stream( std::string category);


            template< typename T>
            thread::Safe operator << ( T&& value)
            {
               thread::Safe proxy{ *this};
               proxy << std::forward< T>( value);
               return proxy;
            }
         };


         //!
         //! Log with category 'debug'
         //!
         extern Stream debug;

         //!
         //! Log with category 'trace'
         //!
         extern Stream trace;


         //!
         //! Log with category 'parameter'
         //!
         extern Stream parameter;

         //!
         //! Log with category 'information'
         //!
         extern Stream information;

         //!
         //! Log with category 'warning'
         //!
         //! @note should be used very sparsely. Either it is an error or it's not...
         //!
         extern Stream warning;

         //!
         //! Log with category 'error'
         //!
         //! @note always active
         //!
         extern Stream error;

         namespace stream
         {
            //!
            //! @returns the corresponding stream for the @p category
            //!
            Stream& get( const std::string& category);
         } // stream




         //!
         //! @return true if the log-category is active.
         //!
         bool active( const std::string& category);

         void activate( const std::string& category);

         void deactivate( const std::string& category);

         void write( const std::string& category, const std::string& message);


      } // log

   } // common

} // casual



#endif // COMMON_LOG_H
