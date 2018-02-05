//!
//! casual
//!

#include "http/outbound/request.h"
#include "http/common.h"

#include "common/exception/system.h"
#include "common/string.h"
#include "common/algorithm.h"
#include "common/transcode.h"
#include "common/buffer/type.h"

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

                  struct Reply
                  {
                     http::Header header;
                     std::string payload;
                  };


                  namespace callback
                  {
                     using payload_const_view = common::Range< const char*>;

                     namespace reply
                     {
                        size_t payload( char* data, size_t size, size_t nmemb, curl::Reply* destination)
                        {
                           Trace trace{ "http::request::local::curl::callback::reply::payload"};

                           if( destination == nullptr)
                           {
                              return 0;
                           }

                           auto first = data;
                           auto last = data + ( size * nmemb);

                           destination->payload.insert( std::end( destination->payload), first, last);

                           return size * nmemb;
                        }
                     } // reply


                     namespace request
                     {
                        size_t read( char* buffer, common::platform::size::type size, payload_const_view& input)
                        {

                           if( input.size() < size)
                           {
                              common::algorithm::copy( input, buffer);
                              auto count = input.size();
                              input.advance( count);
                              return count;
                           }
                           else
                           {
                              std::copy( std::begin( input), std::begin( input) + size, buffer);
                              input.advance( size);
                              return size;
                           }
                        }

                        size_t payload( char* buffer, size_t size, size_t nitems, payload_const_view* input)
                        {
                           Trace trace{ "http::request::local::curl::callback::request::payload"};

                           return read( buffer, size * nitems, *input);
                        }

                     } // request

                     namespace local
                     {
                        namespace 
                        {
                           template< typename R>
                           auto to_string( R&& range)
                           {
                              range = common::string::trim( range);
                              return std::string( std::begin( range), std::end( range));
                           };
                        }
                     }

                     std::size_t header( char* buffer, size_t size, size_t nitems, http::Header* destination)
                     {
                        Trace trace{ "http::request::local::curl::header"};

                        if( destination == nullptr)
                        {
                          return 0;
                        }

                        auto range = common::range::make( buffer, buffer + ( size * nitems));

                        range = std::get< 0>( common::algorithm::divide_if( range, []( char c){
                           return c == '\n' || c == '\r';
                        }));


                        if( range)
                        {
                           auto split = common::algorithm::split( range, ':');

                           /* bug in gcc 5.4 can't use this lambda
                           auto to_string = []( auto&& range){
                              range = common::string::trim( range);
                              return std::string( std::begin( range), std::end( range));
                           };
                           */

                           destination->emplace_back(
                              local::to_string( std::get< 0>( split)),
                              local::to_string( std::get< 1>( split))
                           );
                        }
                        // else:
                        // Think this is an "empty header" that we'll be invoked as the "last header"
                        //  

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
                  using payload_const_view = local::curl::callback::payload_const_view;


                  Connection( const std::string& url, const http::Header& header)
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
                     curl::set::option( m_handle, CURLOPT_FAILONERROR, 1L);


                     http::verbose::log << "header: " << common::range::make( header) << '\n';

                     for( auto& field : header)
                     {
                        auto http = field.http();

                        m_header.reset( curl_slist_append( m_header.release(), http.c_str()));
                     }
                  }


                  request::Reply get() const
                  {
                     Trace trace{ "http::request::local::Connection::get"};

                     local::curl::Reply reply;
                     curl::set::option( m_handle, CURLOPT_WRITEFUNCTION, &curl::callback::reply::payload);
                     curl::set::option( m_handle, CURLOPT_WRITEDATA, &reply);

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

                     switch( code)
                     {
                        case CURLE_OK:
                           break;
                        case CURLE_COULDNT_CONNECT:
                        {
                           throw common::exception::system::communication::unavailable::no::Connect{ common::string::compose(
                              "failed to connect - code: ", code, " - message: ", global::error::buffer.data())};
                        }
                        case CURLE_GOT_NOTHING:
                        case CURLE_RECV_ERROR:
                        {
                           throw common::exception::system::communication::no::message::Absent{ common::string::compose(
                              "failed to receive - code: ", code, " - message: ", global::error::buffer.data())};
                        }
                        default:
                        {
                           throw common::exception::system::invalid::Argument{ common::string::compose(
                              "failed to connect - code: ", code, " - message: ", global::error::buffer.data())};
                        }
                     }

                     return transform( std::move( reply));
                  }


                  request::Reply post( const payload::Request& payload)
                  {
                     Trace trace{ "http::request::local::Connection::post"};

                     curl::set::option( m_handle, CURLOPT_POST, 1L);

                     auto view = transcode( payload);

                     //
                     // set read-payload-stuff
                     //
                     {
                        curl::set::option( m_handle, CURLOPT_POSTFIELDSIZE_LARGE , view.size());
                        curl::set::option( m_handle, CURLOPT_READFUNCTION, &curl::callback::request::payload);
                        curl::set::option( m_handle, CURLOPT_READDATA , &view);
                     }

                     return get();
                  }
               private:

                  request::Reply transform( local::curl::Reply&& reply) const
                  {
                     Trace trace{ "http::request::local::Connection::transform"};

                     auto transcode_base64 = [&]( local::curl::Reply&& reply, const std::string& type){

                        request::Reply result;
                        result.payload.type = std::move( type);
                        result.header = std::move( reply.header);
                        result.payload.memory = common::transcode::base64::decode( reply.payload);

                        return result;
                     };

                     auto transcode_none = [&]( local::curl::Reply&& reply, const std::string& type){

                        request::Reply result;
                        result.payload.type = type;
                        result.header = std::move( reply.header);
                        common::algorithm::copy( reply.payload, result.payload.memory);

                        return result;
                     };


                     const auto mapping = std::map< std::string, std::function< request::Reply( local::curl::Reply&&, const std::string&)>>{
                        {
                           "CFIELD/",
                           transcode_base64
                        },
                        {
                           common::buffer::type::binary(),
                           transcode_base64
                        },
                        {
                           common::buffer::type::x_octet(),
                           transcode_base64
                        },
                        {
                           common::buffer::type::json(),
                           transcode_none
                        },
                        {
                           common::buffer::type::xml(),
                           transcode_none
                        }
                     };

                     auto content = reply.header.at( "content-type", "");

                     verbose::log << "content: " << content << '\n';

                     auto type = protocol::convert::to::buffer( content);

                     auto found = common::algorithm::find( mapping, type);

                     if( found)
                     {
                        return found->second( std::move( reply), type);
                     }
                     else
                     {
                        common::log::category::error << "failed to find a transcoder for buffertype: " << type << '\n';
                        verbose::log << "header: " << common::range::make( reply.header) << '\n';
                        return transcode_none( std::move( reply), common::buffer::type::x_octet());
                     }
                  }


                  payload_const_view transcode( const payload::Request& payload)
                  {
                     Trace trace{ "http::request::local::Connection::transcode"};

                     auto add_content_header = [&]( const std::string& buffertype){
                        auto content = protocol::convert::from::buffer( buffertype);

                        verbose::log << "content: " << content << '\n';

                        if( ! content.empty())
                        {
                           auto header = "content-type: " + content;
                           verbose::log << "header: " << header << '\n';
                           m_header.reset( curl_slist_append( m_header.release(), header.c_str()));
                        }
                     };


                     auto transcode_base64 = [&]( const payload::Request& payload){

                        Trace trace{ "http::request::local::Connection::transcode transcode_base64"};

                        add_content_header( payload.payload().type);

                        m_transcoded_payload = common::transcode::base64::encode( payload.payload().memory);
                        return payload_const_view( m_transcoded_payload.data(), m_transcoded_payload.size());
                     };

                     auto transcode_none = [&]( const payload::Request& payload){

                        Trace trace{ "http::request::local::Connection::transcode transcode_none"};

                        add_content_header( payload.payload().type);
                        return payload_const_view( payload.payload().memory.data(), payload.payload().memory.size());
                     };

                     const auto mapping = std::map< std::string, std::function<payload_const_view(const payload::Request&)>>{
                        {
                           "CFIELD/",
                           transcode_base64
                        },
                        {
                           common::buffer::type::binary(),
                           transcode_base64
                        },
                        {
                           common::buffer::type::x_octet(),
                           transcode_base64
                        },
                        {
                           common::buffer::type::json(),
                           transcode_none
                        },
                        {
                           common::buffer::type::xml(),
                           transcode_none
                        }
                     };

                     auto found = common::algorithm::find( mapping, payload.payload().type);

                     if( found)
                     {
                        verbose::log << "found transcoder for: " << found->first << '\n';
                        return found->second( payload);
                     }
                     else
                     {
                        common::log::category::warning << "failed to find a transcoder for buffertype: " << payload.payload().type << '\n';
                        verbose::log << "payload: " << payload.payload() << '\n';
                        return transcode_none( payload);
                     }
                  }

                  handle_type m_handle;
                  header_type m_header = header_type{ nullptr, &curl_slist_free_all};
                  std::string m_transcoded_payload;
               };
            } // <unnamed>
         } // local

         Reply post( const std::string& url, const payload::Request& payload)
         {
            return post( url, payload, {});
         }

         Reply post( const std::string& url, const payload::Request& payload, const http::Header& header)
         {
            Trace trace{ "http::request::post"};

            local::Connection connection( url, header);

            return connection.post( payload);
         }


      } // request
   } // http
} // casual


