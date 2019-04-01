//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "http/outbound/curl.h"

#include "common/exception/system.h"
#include "common/exception/handle.h"
#include "common/string.h"

namespace casual
{
   using namespace common;

   namespace http
   {
      namespace outbound
      {
         namespace curl
         {
            namespace local
            {
               namespace
               {
                  namespace global
                  {
                     struct Initializer 
                     {
                        static const Initializer& instance()
                        {
                           static const Initializer singleton;
                           return singleton;
                        }

                        auto multi() const
                        {
                           return curl::type::multi{ curl_multi_init()};
                        }
                        
                        auto easy() const 
                        { 
                           return curl::type::easy( curl_easy_init());
                        }

                     private:
                        Initializer()
                        {
                           curl::check( curl_global_init( CURL_GLOBAL_DEFAULT));

                           auto version = curl_version_info( CURLVERSION_NOW);
                           common::log::line( http::log, "curl version: ", version->version);
                        }

                        ~Initializer()
                        {
                           curl_global_cleanup();
                        }

                     };

                     type::error::buffer error;
                  } // global

                  namespace get
                  {
                     using info_code = decltype( CURLINFO_EFFECTIVE_URL);

                     template< info_code code, typename Enable = void>
                     struct info_value;

                     // long
                     template< info_code code>
                     struct info_value< code, std::enable_if_t< 
                        code == CURLINFO_HTTP_CONNECTCODE || 
                        code == CURLINFO_NUM_CONNECTS ||
                        code == CURLINFO_PRIMARY_PORT ||
                        code == CURLINFO_LOCAL_PORT>> 
                     {
                        using type = long;
                     };

                     // char*
                     template< info_code code>
                     struct info_value< code, std::enable_if_t< 
                        code == CURLINFO_EFFECTIVE_URL ||
                        code == CURLINFO_PRIMARY_IP ||
                        code == CURLINFO_LOCAL_IP>> 
                     {
                        using type = char*;
                     };

                     template< info_code code>
                     using info_value_t = typename info_value< code>::type;

                     template< info_code code>
                     auto option( const type::easy& handle)
                     {
                        info_value_t< code> value{};
                        curl_easy_getinfo( handle.get(), code, &value);
                        return value;
                     } 
                      
                  } // get
               } // <unnamed>
            } // local

            namespace type
            {
               std::ostream& operator << ( std::ostream& out, const easy& value)
               {
                  return out << "{ HTTP_CONNECTCODE: " << local::get::option< CURLINFO_HTTP_CONNECTCODE>( value)
                     << ", NUM_CONNECTS: " << local::get::option< CURLINFO_NUM_CONNECTS>( value)
                     << ", EFFECTIVE_URL: " << local::get::option< CURLINFO_EFFECTIVE_URL>( value) 
                     << ", PRIMARY_IP: " << local::get::option< CURLINFO_PRIMARY_IP>( value) 
                     << ", PRIMARY_PORT: " << local::get::option< CURLINFO_PRIMARY_PORT>( value) 
                     << ", LOCAL_IP: " << local::get::option< CURLINFO_LOCAL_IP>( value) 
                     << ", LOCAL_PORT: " << local::get::option< CURLINFO_LOCAL_PORT>( value) 
                     << '}';
               }
            } // type



            namespace error
            {
               type::error::buffer& buffer() { return local::global::error;}
            } // error

            void check( type::code::multi value)
            {
               using code = type::code::multi;
               switch( value)
               {
                  case code::CURLM_OK: break;
                  case code::CURLM_CALL_MULTI_PERFORM: break;

                  default: 
                  {  
                     throw exception::system::invalid::Argument{ string::compose( "code: ", value, " - ", curl_multi_strerror( value))};
                  }
               }
            }

            void check( type::code::easy value)
            {
               using code = type::code::easy;
               switch( value)
               {
                  case code::CURLE_OK: break;

                  case CURLE_COULDNT_CONNECT:
                  {
                     throw common::exception::system::communication::unavailable::no::Connect{ common::string::compose(
                        "failed to connect - code: ", value, " - message: ", error::buffer().data())};
                  }
                  case CURLE_GOT_NOTHING:
                  case CURLE_RECV_ERROR:
                  {
                     throw common::exception::system::communication::no::message::Absent{ common::string::compose(
                        "failed to receive - code: ", value, " - message: ", error::buffer().data())};
                  }
                  default: 
                  {  
                     throw exception::system::invalid::Argument{ string::compose( "code: ", value, " - ", curl_easy_strerror( value))};
                  }
               }
            }

            void log( type::code::multi code)
            {
               try
               {
                  check( code);
               }
               catch( ...)
               {
                  exception::handle();
               }
            }

            namespace multi
            {
               void add( const type::multi& multi, const type::easy& easy)
               {
                  curl::check( curl_multi_add_handle( multi.get(), easy.get()));
               }

               void remove( const type::multi& multi, const type::easy& easy)
               {
                  curl::log( curl_multi_remove_handle( multi.get(), easy.get()));
               }

               common::platform::size::type perform( const type::multi& multi)
               {
                  int count = 0;
                  curl::check( curl_multi_perform( multi.get(), &count));
                  return count;
               }

               type::multi create()
               {
                  return local::global::Initializer::instance().multi();
               }
            } // multi

            namespace easy
            {
               type::easy create()
               {
                  return local::global::Initializer::instance().easy();
               }
            } // easy

         } // curl
      } // outbound
   } // http
} // casual
