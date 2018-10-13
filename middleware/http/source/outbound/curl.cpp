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
                     namespace easy
                     {
                        std::vector< curl::type::native::easy> cache;
                     } // easy

                     namespace cleanup
                     {
                        void easy( curl::type::native::easy handle)
                        {
                           if( global::easy::cache.size() < 100)
                           {
                              curl_easy_reset( handle);
                              global::easy::cache.push_back( handle);
                           }
                           else 
                           {
                              curl_easy_cleanup( handle);
                           }
                        }
                     } // cleanup
                     struct Initializer 
                     {
                        static const Initializer& instance()
                        {
                           static const Initializer singleton;
                           return singleton;
                        }

                        auto multi() const { return common::memory::guard( curl_multi_init(), &curl_multi_cleanup);}
                        
                        auto easy() const 
                        { 
                           if( ! global::easy::cache.empty())
                           {
                              auto handle = common::memory::guard( global::easy::cache.back(), &cleanup::easy);
                              global::easy::cache.pop_back();
                              return handle;
                           }
                           return common::memory::guard( curl_easy_init(), &cleanup::easy);
                        }

                     private:
                        Initializer()
                        {
                           curl::check( curl_global_init( CURL_GLOBAL_DEFAULT));
                           global::easy::cache.reserve( 100);
                        }

                        ~Initializer()
                        {
                           algorithm::for_each( global::easy::cache, []( auto handle){ curl_easy_cleanup( handle);});
                           curl_global_cleanup();
                        }

                     };

                     type::error::buffer error;
                  } // global
               } // <unnamed>
            } // local

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