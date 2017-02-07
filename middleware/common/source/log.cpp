//!
//! casual
//!

#include "common/log.h"
#include "common/internal/log.h"
#include "common/environment.h"
#include "common/platform.h"
#include "common/process.h"
#include "common/exception.h"
#include "common/chronology.h"
#include "common/server/context.h"
#include "common/transaction/context.h"
#include "common/service/call/context.h"
#include "common/execution.h"
#include "common/algorithm.h"
#include "common/string.h"
#include "common/domain.h"

//
// std
//
#include <fstream>
#include <map>
#include <iostream>

#include <thread>



namespace casual
{
   namespace common
   {
      namespace log
      {


         namespace thread
         {
            std::mutex Safe::m_mutex;

         } // thread

         namespace local
         {
            namespace
            {

               class File
               {
               public:
                  static File& instance()
                  {
                     static File singleton;
                     return singleton;
                  }

                  void log( const std::string& category, const char* message)
                  {
                     static const std::string basename{ file::name::base( process::path())};

                     m_output << std::chrono::duration_cast< std::chrono::microseconds>( platform::clock_type::now().time_since_epoch()).count()
                        << '|' << common::domain::identity().name
                        << '|' << execution::id()
                        << '|' << process::id()
                        << '|' << std::this_thread::get_id()
                        << '|' << basename
                        << '|' << transaction::Context::instance().current().trid
                        << '|' << execution::service::parent::name()
                        << '|' << execution::service::name()
                        << '|' << category
                        << '|' << message
                        << std::endl;
                  }

                  void log( const std::string& category, const std::string& message)
                  {
                     log( category, message.c_str());
                  }

               private:
                  File()
                  {
                     open();
                  }

                  bool open( const std::string& file)
                  {
                     m_output.open( file, std::ios::app | std::ios::out);

                     if( m_output.fail())
                     {
                        //
                        // We don't want to throw... Or do we?
                        //
                        std::cerr << process::path() << " - could not open log-file: " << file << std::endl;
                        return false;
                     }
                     return true;

                  }

                  void open()
                  {
                     if( ! open( common::environment::directory::domain() + "/casual.log"))
                     {
                        std::cerr << "using current directory - ./casual.log" << std::endl;
                        open( "./casual.log");
                     }
                  }

                  std::ofstream m_output;
               };

               namespace user
               {
                  std::regex filter()
                  {
                     return std::regex{ common::environment::variable::get( "CASUAL_LOG", "error")};
                  }



               } // user

               namespace error
               {
                  const std::regex& filter()
                  {
                     static std::regex filter{ "error"};
                     return filter;
                  }

               } // error

               namespace stream
               {

                  struct buffer : public std::streambuf
                  {
                     using base_type = std::streambuf;

                     buffer( std::string category)
                        : m_category( std::move( category))
                     {

                     }

                     int overflow( int value) override
                     {
                        if( value != base_type::traits_type::eof())
                        {
                           if( value == '\n')
                           {
                              log();
                           }
                           else
                           {
                              m_buffer.push_back( base_type::traits_type::to_char_type( value));
                           }

                        }
                        return value;
                     }

                  private:

                     void log()
                     {
                        File::instance().log(  m_category, m_buffer);
                        m_buffer.clear();
                     }

                     typedef std::string buffer_type;
                     buffer_type m_buffer;

                     const std::string m_category;


                  };


                  struct holder
                  {
                     holder( std::string category, log::Stream& stream)
                       : category{ std::move( category)}, stream{ stream} {}

                     std::string category;
                     std::reference_wrapper< log::Stream> stream;
                     std::unique_ptr< stream::buffer> buffer;
                  };

                  std::vector< holder>& streams()
                  {
                     static std::vector< holder> streams;
                     return streams;
                  }


                  stream::buffer* activate( holder& holder)
                  {
                     holder.buffer.reset( new stream::buffer{ holder.category});
                     return holder.buffer.get();
                  }

                  stream::buffer* activate( holder& holder, const std::regex& filter)
                  {
                     if( std::regex_match( holder.category, error::filter()) || std::regex_match( holder.category, filter))
                     {
                        return activate( holder);
                     }
                     else
                     {
                        holder.buffer.reset( nullptr);
                     }

                     return holder.buffer.get();
                  }

                  namespace find
                  {
                     holder* holder( const std::string& category)
                     {
                        auto found = range::find_if( stream::streams(), [&]( const stream::holder& h){ return h.category == category;});

                        if( found)
                        {
                           return &( *found);
                        }
                        return nullptr;
                     }

                  } // find

               } // stream



               namespace registration
               {

                  stream::buffer* stream( log::Stream& stream, std::string category)
                  {
                     if( ! stream::find::holder( category))
                     {
                        stream::streams().emplace_back( std::move( category), stream);

                        return stream::activate( stream::streams().back(), user::filter());
                     }
                     return nullptr;
                  }


               } // registration

            } // unnamed

         } // local




         Stream::Stream( std::string category)
           : std::ostream{ local::registration::stream( *this, std::move( category))}
         {

         }



         namespace stream
         {

            Stream& get( const std::string& category)
            {
               auto holder = local::stream::find::holder( category);

               if( holder)
               {
                  return holder->stream;
               }

               throw exception::invalid::Argument{ "invalid log category", CASUAL_NIP( category)};
            }
         } // stream




         namespace internal
         {

            Stream debug{ "casual.debug"};
            Stream trace{ "casual.trace"};
            Stream transaction{ "casual.transaction"};
            Stream ipc{ "casual.ipc"};
            Stream buffer{ "casual.buffer"};
         } // internal

         Stream debug{ "debug"};
         Stream trace{ "trace"};
         Stream parameter{ "parameter"};
         Stream information{ "information"};
         Stream warning{ "warning"};

         //
         // Always on
         //
         Stream error{ "error"};





         bool active( const std::string& category)
         {
            return static_cast< bool>( stream::get( category));
         }


         void activate( const std::string& category)
         {
            auto holder = local::stream::find::holder( category);

            if( holder && ! holder->stream.get())
            {
               holder->stream.get().rdbuf( local::stream::activate( *holder));
            }
         }

         void deactivate( const std::string& category)
         {
            auto holder = local::stream::find::holder( category);

            if( holder)
            {
               holder->stream.get().rdbuf( nullptr);
               holder->buffer.reset( nullptr);
            }
         }



         void write( const std::string& category, const std::string& message)
         {
            thread::Safe guard{ std::cout};
            local::File::instance().log( category, message);
         }
      } // log

   } // common
} // casual


