//!
//! casual
//!

#ifndef COMMON_LOG_STREAM_H
#define COMMON_LOG_STREAM_H



#include "common/platform.h"
#include "common/traits.h"
#include "common/range.h"


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
         class Stream;

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

               class Safe : public Lock
               {
               public:
                  inline Safe( std::ostream& stream) : m_stream( stream) {}
                  Safe( Safe&&) = default;


                  template< typename T>
                  Safe& operator << ( T&& value)
                  {
                     //if( m_stream)
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
                     //if( m_stream)
                        m_stream << value;

                     return *this;
                  }

               private:
                  std::ostream& m_stream;
               };


            } // thread

            //!
            //! @returns the corresponding stream for the @p category
            //!
            Stream& get( const std::string& category);


            //!
            //! @return true if the log-category is active.
            //!
            bool active( const std::string& category);

            void activate( const std::string& category);

            void deactivate( const std::string& category);

            void write( const std::string& category, const std::string& message);

         } // stream

         class Stream : public std::ostream
         {
         public:

            Stream( std::string category);


            template< typename T>
            stream::thread::Safe operator << ( T&& value)
            {
               stream::thread::Safe proxy{ *this};
               proxy << std::forward< T>( value);
               return proxy;
            }
         };

         template< typename T, typename = void>
         struct has_formatter : std::false_type{};

         //!
         //! Specialization for containers, to log ranges
         //!
         template< typename C> 
         struct has_formatter< C> : 
            std::conditional_t< traits::container::is_sequence< C>::value && ! traits::container::is_string< C>::value,
               std::true_type, std::false_type>
         {
            struct formatter
            {
               template< typename R>
               void operator () ( std::ostream& out, R&& range) const
               { 
                  out << range::make( range);
               }
            };
         };

         namespace detail
         {
            template< typename S>
            void part( S& stream)
            {
            }

            template< typename S, typename T>
            auto part( S& stream, T&& value) -> std::enable_if_t< has_formatter< std::decay_t< T>>::value>
            {
               typename has_formatter< std::decay_t< T>>::formatter{}( stream, std::forward< T>( value));
            }

            template< typename S, typename T>
            auto part( S& stream, T&& value) -> std::enable_if_t< ! has_formatter< std::decay_t< T>>::value>
            {
               stream << std::forward< T>( value);
            }

            template< typename S, typename Arg, typename... Args>
            void part( S& stream, Arg&& arg, Args&&... args)
            {
               detail::part( stream, std::forward< Arg>( arg));
               detail::part( stream, std::forward< Args>( args)...);
            }
         } // detail

         template< typename... Args>
         void write( std::ostream& stream, Args&&... args)
         {
            if( stream)
            {
               stream::thread::Lock lock;
               detail::part( stream, std::forward< Args>( args)...);
            }
         }

         template< typename... Args>
         void line( std::ostream& stream, Args&&... args)
         {
            write( stream, std::forward< Args>( args)..., '\n');
         } 
      } // log
   } // common
} // casual


#endif // COMMON_LOG_H
