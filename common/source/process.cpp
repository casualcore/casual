//!
//! process.cpp
//!
//! Created on: May 12, 2013
//!     Author: Lazan
//!

#include "common/process.h"
#include "common/exception.h"
#include "common/error.h"
#include "common/file.h"
#include "common/internal/log.h"
#include "common/internal/trace.h"
#include "common/signal.h"
#include "common/string.h"
#include "common/environment.h"
#include "common/uuid.h"

#include "common/message/server.h"
#include "common/queue.h"

//
// std
//
#include <algorithm>
#include <functional>
#include <fstream>

// TODO: temp
#include <iostream>

#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
//#include <fcntl.h>

#include <spawn.h>


#ifdef __APPLE__
   #include <crt_externs.h>
#else
   #include <unistd.h>
#endif



namespace casual
{
   namespace common
   {
      namespace process
      {
         namespace local
         {
            namespace
            {
               std::string getProcessPath()
               {
                  if( environment::variable::exists( "_"))
                  {
                     return environment::variable::get( "_");
                  }

                  return std::string{};
               }

               std::string& path()
               {
                  static std::string path = getProcessPath();
                  return path;
               }

               char* const * environment()
               {
                  #ifdef __APPLE__
                      return *::_NSGetEnviron();
                  #else
                     return ::environ;
                  #endif
               }


            } // <unnamed>
         } // local

         const std::string& path()
         {
            return local::path();
         }

         void path( const std::string& path)
         {
            local::path() = path;
         }


         platform::pid_type id()
         {
            static const platform::pid_type pid = getpid();
            return pid;
         }

         const Uuid& uuid()
         {
            static const Uuid singleton = Uuid::make();
            return singleton;
         }

         void sleep( std::chrono::microseconds time)
         {
            std::this_thread::sleep_for( time);
         }




         platform::pid_type spawn( const std::string& path, const std::vector< std::string>& arguments)
         {

            //
            // prepare arguments
            //
            std::vector< const char*> c_arguments;

            //
            // think we must add application-name as first argument...
            //
            c_arguments.push_back( path.data());

            std::transform(
                  std::begin( arguments),
                  std::end( arguments),
                  std::back_inserter( c_arguments),
                  std::mem_fn( &std::string::data));

            c_arguments.push_back( nullptr);


            std::vector< const char*> c_environment;
            c_environment.push_back( nullptr);



            posix_spawnattr_t attributes;

            posix_spawnattr_init( &attributes);

            platform::pid_type pid;

            auto status =  posix_spawnp(
                  &pid,
                  path.c_str(),
                  nullptr,
                  &attributes,
                  const_cast< char* const*>( c_arguments.data()),
                  local::environment()// environ //const_cast< char* const*>( c_environment.data())
                  );
            switch( status)
            {
               case 0:
                  break;
               default:
                  throw exception::invalid::Argument( "spawn failed for: " + path + " - " + error::string( status));
            }
            return pid;
         }



         int execute( const std::string& path, const std::vector< std::string>& arguments)
         {
            return wait( spawn( path, arguments));
         }


         namespace local
         {
            bool wait( lifetime::Exit& exit, platform::pid_type pid, int flags = WNOHANG)
            {
               exit.pid = waitpid( pid, &exit.status, flags);

               if( exit.pid == -1)
               {
                  if( errno == error::cNoChildProcesses)
                  {

                  }
                  else
                  {
                     log::error << "failed to check state of pid: " << exit.pid << " - " << error::string() << std::endl;
                     throw exception::NotReallySureWhatToNameThisException( error::string());
                  }
               }
               else if( exit.pid != 0)
               {
                  if( WIFEXITED( exit.status))
                  {
                     exit.reason = lifetime::Exit::Reason::exited;
                     exit.status = WEXITSTATUS( exit.status);
                  }
                  else if( WIFSIGNALED( exit.status))
                  {
                     if( WCOREDUMP( exit.status))
                     {
                        exit.reason = lifetime::Exit::Reason::core;
                        exit.status = 0;
                     }
                     else
                     {
                        exit.reason = lifetime::Exit::Reason::signaled;
                        exit.status = WTERMSIG( exit.status);
                     }
                  }
                  else if( WIFSTOPPED( exit.status))
                  {
                     exit.reason = lifetime::Exit::Reason::stopped;
                     exit.status = WSTOPSIG( exit.status);
                  }
                  return true;
               }
               return false;
            }
         } // local

         int wait( platform::pid_type pid)
         {
            lifetime::Exit exit;

            local::wait( exit, pid, 0);

            return exit.status;
         }




         std::vector< platform::pid_type> terminate( const std::vector< platform::pid_type>& pids)
         {
            std::vector< platform::pid_type> result;
            for( auto pid : pids)
            {
               if( ! terminate( pid))
               {
                  result.push_back( pid);
               }
            }
            return result;
         }



         bool terminate( platform::pid_type pid)
         {
            return signal::send( pid, platform::cSignal_Terminate);
         }

         file::scoped::Path singleton( std::string path)
         {
            std::ifstream file( path);

            if( file)
            {
               try
               {

                  common::signal::alarm::Scoped alarm{ 5};

                  decltype( common::ipc::receive::id()) id;
                  std::string uuid;
                  file >> id;
                  file >> uuid;

                  {
                     common::queue::blocking::Writer send( id);
                     common::message::server::ping::Request request;
                     request.server = common::message::server::Id::current();
                     send( request);
                  }


                  {
                     common::message::server::ping::Reply reply;

                     common::queue::blocking::Reader receive( common::ipc::receive::queue());
                     receive( reply);

                     if( reply.uuid == Uuid( uuid))
                     {

                        //
                        // There are another process for this domain - abort
                        //
                        throw common::exception::invalid::Process( "only a single process of this type is allowed in a domain", __FILE__, __LINE__);
                     }

                     common::log::internal::debug << "process singleton queue file " << path << " is adopted by " << common::process::id() << std::endl;
                  }

               }
               catch( const common::exception::signal::Timeout&)
               {
                  common::log::error << "failed to get ping response from potentially running process - to risky to start - action: terminate" << std::endl;
                  throw common::exception::invalid::Process( "only a single process of this type is allowed in a domain", __FILE__, __LINE__);
               }
               catch( const common::exception::invalid::Argument&)
               {
                  common::log::internal::debug << "process singleton queue file " << path << " is adopted by " << common::process::id() << std::endl;
               }
            }

            file::scoped::Path result( std::move( path));


            std::ofstream output( result);

            if( output)
            {
               output << common::ipc::receive::id() << std::endl;
               output << uuid() << std::endl;
            }
            else
            {
               throw common::exception::invalid::File( "failed to write process singleton queue file: " + path);
            }

            return result;
         }

         namespace lifetime
         {

            std::ostream& operator << ( std::ostream& out, const Exit& terminated)
            {
               out << "{pid: " << terminated.pid << " terminated - reason: ";
               switch( terminated.reason)
               {
                  case Exit::Reason::unknown: out << "unknown"; break;
                  case Exit::Reason::exited: out << "exited"; break;
                  case Exit::Reason::stopped: out << "stopped"; break;
                  case Exit::Reason::signaled: out <<  "signaled"; break;
                  case Exit::Reason::core: out <<  "core"; break;
               }
               return out << '}';
            }

            std::vector< lifetime::Exit> ended()
            {
               std::vector< lifetime::Exit> result;

               Exit exit;

               while( local::wait( exit, -1))
               {
                  result.push_back( exit);
               }

               return result;
            }
         } // lifetime



      } // process
   } // common
} // casual


