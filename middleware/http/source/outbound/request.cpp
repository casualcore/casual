//!
//! casual
//!

#include "http/outbound/request.h"
#include "http/common.h"

#include "common/exception/system.h"
#include "common/string.h"
#include "common/algorithm.h"

#include <curl/curl.h>

namespace casual
{
   namespace http
   {
      namespace request
      {

         namespace local
         {
            namespace
            {

               namespace global
               {
                  namespace error
                  {
                     std::array< char, CURL_ERROR_SIZE> buffer = {};
                  } // error


               } // global


               namespace curl
               {
                  namespace set
                  {
                     //using directive_type = decltype( CURLOPT_ERRORBUFFER);

                     template< typename H, typename Directive, typename Data>
                     void option( H&& handle, Directive directive, Data&& data)
                     {
                        http::verbose::log << "directive: " << directive << " - data: " << data << '\n';

                        auto code = curl_easy_setopt( handle.get(), directive, std::forward< Data>( data));


                        if( code != CURLE_OK)
                        {
                           throw common::exception::system::invalid::Argument{ common::string::compose(
                              "failed to set curl option - directive: ", directive, ", code: ", code, ", message: ", global::error::buffer.data())};
                        }
                     }

                  } // set


                  namespace callback
                  {
                     namespace reply
                     {
                        size_t payload( char* data, size_t size, size_t nmemb, std::vector< char>* destination)
                        {
                          if( destination == nullptr)
                          {
                            return 0;
                          }

                          auto first = data;
                          auto last = data + ( size * nmemb);

                          destination->insert( std::end( *destination), first, last);

                          return size * nmemb;
                        }
                     } // reply


                     namespace request
                     {
                        struct Payload
                        {
                           Payload( const common::platform::binary::type& data) : data( data) {}

                           std::size_t offset = 0;
                           const common::platform::binary::type& data;

                           auto begin() { return std::begin( data) + offset;}
                           auto end() { return std::end( data);}

                           auto read( char* buffer, std::size_t size)
                           {
                              auto left = data.size() - offset;
                              if( left < size)
                              {
                                 std::copy( begin(), end(), buffer);
                                 offset += left;
                                 return left;
                              }

                              std::copy( begin(), begin() + size, buffer);
                              offset += size;
                              return size;
                           }
                        };

                        size_t payload( char* buffer, size_t size, size_t nitems, Payload* input)
                        {
                           return input->read( buffer, size * nitems);
                        }

                     } // request


                     std::size_t header( char* buffer, size_t size, size_t nitems, std::vector< std::string>* destination)
                     {
                        if( destination == nullptr)
                        {
                          return 0;
                        }
                        auto first = buffer;
                        auto last = std::find_if( first, first + ( size * nitems), []( char c){
                           return c == '\n' || c == '\r';
                        });

                        //
                        // Tror detta är en "tom header" som vi blir invokerad med som "sista header"
                        // skulla kunna användas att detektera när anropet resulterar
                        // i flera headers (https handskakning och whatnot..)
                        //
                        if( first == last)
                        {
                           return size * nitems;
                        }

                        destination->emplace_back( first, last);

                        return size * nitems;
                     }

                  } // write

                  using handle_type = std::unique_ptr< CURL, std::function< void(CURL*)>>;
               } // curl

               struct Curl
               {
                  static const Curl& instance()
                  {
                     static Curl singleton;
                     return singleton;
                  }


                  curl::handle_type handle() const
                  {
                     return { curl_easy_init(), &curl_easy_cleanup};
                  }



               private:
                  Curl()
                  {
                     if( curl_global_init( CURL_GLOBAL_DEFAULT) != CURLE_OK)
                     {
                        throw std::runtime_error{ "failed to initialize curl"};
                     }
                  }
               };

               struct Connection
               {
                  using handle_type = curl::handle_type;
                  using header_type = std::unique_ptr< curl_slist, std::function< void(curl_slist*)>>;


                  Connection( const std::string& url, const std::vector< std::string>& header)
                    : m_handle( Curl::instance().handle())
                  {
                     Trace trace{ "http::request::local::Connection::Connection"};

                     if( ! m_handle)
                     {
                        throw std::runtime_error{ "failed to initialize curl connection"};
                     }

                     http::verbose::log << "handle @" << m_handle.get() << '\n';

                     curl::set::option( m_handle, CURLOPT_ERRORBUFFER, global::error::buffer.data());
                     curl::set::option( m_handle, CURLOPT_URL, url.data());
                     curl::set::option( m_handle, CURLOPT_FOLLOWLOCATION, 1L);
                     curl::set::option( m_handle, CURLOPT_WRITEFUNCTION, &curl::callback::reply::payload);
                     curl::set::option( m_handle, CURLOPT_FAILONERROR, 1L);


                     http::verbose::log << "header: " << common::range::make( header) << '\n';

                     for( auto& field : header)
                     {
                        m_header.reset( curl_slist_append( m_header.release(), field.c_str()));
                     }

                  }


                  request::Reply get() const
                  {
                     Trace trace{ "http::request::local::Connection::get"};

                     request::Reply reply;
                     curl::set::option( m_handle, CURLOPT_WRITEDATA, &reply.payload);

                     if( m_header)
                     {
                        curl::set::option( m_handle, CURLOPT_HTTPHEADER, m_header.get());
                     }

                     //
                     // Set reply-headers-stuff
                     //
                     {
                        curl::set::option( m_handle, CURLOPT_HEADERFUNCTION, &curl::callback::header);
                        curl::set::option( m_handle, CURLOPT_HEADERDATA, &reply.header);
                     }



                     auto code = curl_easy_perform( m_handle.get());
                     if( code != CURLE_OK)
                     {
                        throw common::exception::system::invalid::Argument{ common::string::compose(
                           "failed to connect - code: ", code, " - message: ", global::error::buffer.data())};
                     }

                     return reply;
                  }


                  request::Reply post( const common::platform::binary::type& payload) const
                  {
                     Trace trace{ "http::request::local::Connection::post"};

                     curl::set::option( m_handle, CURLOPT_POST, 1L);

                     curl::callback::request::Payload holder( payload);
                     //
                     // set read-payload-stuff
                     //
                     {
                        curl::set::option( m_handle, CURLOPT_POSTFIELDSIZE_LARGE , payload.size());
                        curl::set::option( m_handle, CURLOPT_READFUNCTION, &curl::callback::request::payload);
                        curl::set::option( m_handle, CURLOPT_READDATA , &holder);
                     }

                     return get();
                  }

               private:
                  handle_type m_handle;
                  header_type m_header = header_type{ nullptr, &curl_slist_free_all};
               };
            } // <unnamed>
         } // local

         Reply post( const std::string& url, const common::platform::binary::type& payload, const std::vector< std::string>& header)
         {
            Trace trace{ "http::request::post"};

            local::Connection connection( url, header);

            return connection.post( payload);
         }


      } // request
   } // http
} // casual


