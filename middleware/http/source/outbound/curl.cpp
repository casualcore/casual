//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "http/outbound/curl.h"

#include "common/string.h"
#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/code/serialize.h"
#include "common/exception/capture.h"
#include "common/algorithm/compare.h"

namespace casual
{
   using namespace common;

   namespace http::outbound::curl
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

               template< info_code code>
               struct info_value;

               // long
               template< info_code code>
               requires ( common::algorithm::compare::any( code, 
                  CURLINFO_HTTP_CONNECTCODE, CURLINFO_NUM_CONNECTS, CURLINFO_PRIMARY_PORT, CURLINFO_LOCAL_PORT, CURLINFO_RESPONSE_CODE))
               struct info_value< code>
               {
                  using type = long;
               };

               // char*
               template< info_code code>
               requires ( common::algorithm::compare::any( code, 
                  CURLINFO_EFFECTIVE_URL, CURLINFO_PRIMARY_IP, CURLINFO_LOCAL_IP))
               struct info_value< code>
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

            namespace code
            {
               namespace category
               {
                  struct Easy : std::error_category
                  {
                     const char* name() const noexcept override
                     {
                        return "curl-easy";
                     }

                     std::string message( int code) const override
                     {
                        return curl_easy_strerror( static_cast< CURLcode>( code));
                     }
                  };

                  const auto& easy = common::code::serialize::registration< Easy>( 0x54edf8d7545d475fbecd24e15e6828b4_uuid);

                  struct Multi : std::error_category
                  {
                     const char* name() const noexcept override
                     {
                        return "curl-multi";
                     }

                     std::string message( int code) const override
                     {
                        return curl_multi_strerror( static_cast< CURLMcode>( code));
                     }
                  };

                  const auto& multi = common::code::serialize::registration< Multi>( 0x76797ff9a38f49b08ef6592418d2f13c_uuid);

               } // category
            } // code

         } // <unnamed>
      } // local

      namespace type
      {
         std::ostream& operator << ( std::ostream& out, const easy& easy)
         {
            return out << "{ HTTP_CONNECTCODE: " << local::get::option< CURLINFO_HTTP_CONNECTCODE>( easy)
               << ", NUM_CONNECTS: " << local::get::option< CURLINFO_NUM_CONNECTS>( easy)
               << ", EFFECTIVE_URL: " << local::get::option< CURLINFO_EFFECTIVE_URL>( easy)
               << ", RESPONSE_CODE: " << local::get::option< CURLINFO_RESPONSE_CODE>( easy)
               << ", PRIMARY_IP: " << local::get::option< CURLINFO_PRIMARY_IP>( easy) 
               << ", PRIMARY_PORT: " << local::get::option< CURLINFO_PRIMARY_PORT>( easy) 
               << ", LOCAL_IP: " << local::get::option< CURLINFO_LOCAL_IP>( easy)
               << ", LOCAL_PORT: " << local::get::option< CURLINFO_LOCAL_PORT>( easy) 
               << '}';
         }
      } // type

      namespace error
      {
         type::error::buffer& buffer() { return local::global::error;}
      } // error


      namespace response
      {
         long code( const type::easy& easy)
         {
            return local::get::option< CURLINFO_RESPONSE_CODE>( easy);
         }
      } // response

      void check( type::code::multi value)
      {
         using code = type::code::multi;
         switch( value)
         {
            case code::CURLM_OK: break;
            case code::CURLM_CALL_MULTI_PERFORM: break;

            default: 
               common::code::raise::error( common::code::casual::internal_unexpected_value, value);
         }
      }

      void check( type::code::easy value)
      {
         using code = type::code::easy;
         switch( value)
         {
            case code::CURLE_OK: break;

            case CURLE_COULDNT_CONNECT:
               common::code::raise::error( common::code::casual::communication_refused, "failed to connect - code: ", value, ' ', error::buffer().data());

            case CURLE_GOT_NOTHING:
            case CURLE_RECV_ERROR:
               common::code::raise::error( common::code::casual::communication_no_message, "failed to receive - code: ", value, ' ', error::buffer().data());

            default: 
               common::code::raise::error( common::code::casual::internal_unexpected_value, value);
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
            common::exception::sink();
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

         platform::size::type perform( const type::multi& multi)
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

   } // http::outbound::curl
} // casual


std::error_code make_error_code( CURLcode code)
{
   return { code, casual::http::outbound::curl::local::code::category::easy};
}

std::error_code make_error_code( CURLMcode code)
{
   return { code, casual::http::outbound::curl::local::code::category::multi};
}
