//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/environment.h"
#include "common/exception/system.h"
#include "common/file.h"
#include "common/algorithm.h"
#include "common/log.h"
#include "common/string.h"
#include "common/view/string.h"


#include <memory>
#include <iomanip>
#include <cstdlib>

namespace casual
{
   namespace common
   {
      namespace environment
      {
         namespace local
         {
            namespace
            {
               namespace native
               {
                  struct Variable
                  {
                     using lock_type = std::lock_guard< std::mutex>;

                     static const Variable& instance()
                     {
                        static const Variable singleton{};
                        return singleton;
                     }

                     bool exists( const char* name) const
                     {
                        lock_type lock { m_mutex};

                        return getenv( name) != nullptr;
                     }

                     std::string get( const char* name) const
                     {
                        lock_type lock { m_mutex};

                        auto result = getenv( name);

                        // We need to return by value and copy the variable while
                        // we have the lock
                        if( result)
                        {
                           return result;
                        }

                        return {};
                     }

                     void set( const char* name, const std::string& value) const
                     {
                        lock_type lock { m_mutex};

                        if( setenv( name, value.c_str(), 1) == -1)
                        {
                           exception::system::throw_from_errno();
                        }
                     }

                     void unset( const char* name) const
                     {
                        lock_type lock { m_mutex};

                        if( ::unsetenv( name) == -1)
                        {
                           exception::system::throw_from_errno();
                        }
                     }

                     std::mutex& mutex() const
                     {
                        return m_mutex;
                     }

                  private:

                     mutable std::mutex m_mutex;
                  };

               } // native

            } // <unnamed>
         } // local

         namespace variable
         {
            std::mutex& mutex()
            {
               return local::native::Variable::instance().mutex();
            }

            bool exists( const char* name)
            {
               return local::native::Variable::instance().exists( name);
            }

            namespace detail
            {
               std::string get( const char* name)
               {
                  if( ! exists( name))
                  {
                     throw exception::system::invalid::Argument( string::compose( "failed to get variable: ",name));
                  }
                  return local::native::Variable::instance().get( name);
               }


               std::string get( const char* name, std::string alternative)
               {
                  if( exists( name))
                  {
                     return get( name);
                  }
                  return alternative;
               }

               void set( const char* name, const std::string& value)
               {
                  local::native::Variable::instance().set( name, value);
               }

               void unset( const char* name)
               {
                  local::native::Variable::instance().unset( name);
               }

            } // detail



            namespace process
            {
               common::process::Handle get( const char* variable)
               {
                  auto value = common::environment::variable::get( variable);

                  common::process::Handle result;
                  {
                     auto split = algorithm::split(value, '|');

                     auto pid = std::get < 0 > ( split);
                     if( common::string::integer( pid))
                     {  
                        result.pid = strong::process::id{ std::stoi( std::string( std::begin( pid), std::end( pid)))};
                     }

                     auto ipc = std::get< 1>( split);
                     if( ! ipc.empty())
                     {
                        result.ipc = strong::ipc::id{ Uuid( ipc)};
                     }
                  }

                  return result;
               }

               void set( const char* variable, const common::process::Handle& process)
               {
                  variable::set( variable, string::compose( process.pid, '|', process.ipc));
               }

            } // process


         } // variable

         namespace local
         {
            namespace
            {
               //! holds "all" paths based on enviornment.
               //! main purpose is to be able to reset when running
               //! unittests
               struct Paths 
               {
                  std::string domain = create_path( get_domain());
                  std::string detail = create_path( get_domain() + "/.casual");
                  std::string tmp = "/tmp";
                  std::string casual = variable::get( variable::name::home());

                  std::string log = []() -> std::string
                  {
                     auto file = []( ) -> std::string
                     {
                        if( variable::exists( variable::name::log::path()))
                           return variable::get( variable::name::log::path());

                        if( variable::exists( variable::name::domain::home()))
                           return variable::get( variable::name::domain::home()) + "/casual.log";

                        return "./casual.log";
                     }();

                     create_path( common::directory::name::base( file));
                     return file;
                  }();

                  std::string ipc = []() -> std::string 
                  {
                     auto get = []()
                     {
                        if( variable::exists( variable::name::ipc::directory()))
                           return variable::get( variable::name::ipc::directory());

                        return environment::directory::temporary() + "/casual/ipc";
                     };
                     return create_path( get());
                  }();

                  std::string singleton = detail + "/singleton";

                  friend std::ostream& operator << ( std::ostream& out, const Paths& value)
                  {
                     return out << "{ domain: " << std::quoted( value.domain)
                        << ", tmp: " << std::quoted( value.tmp)
                        << ", casual: " << std::quoted( value.casual)
                        << ", log: " << std::quoted( value.log)
                        << ", ipc: " << std::quoted( value.ipc)
                        << ", singleton: " << std::quoted( value.singleton)
                        << '}';
                  }

               private:
                  static std::string get_domain()
                  {
                     return variable::get( variable::name::domain::home());
                  }

                  static std::string create_path( std::string path)
                  {
                     if( ! common::directory::exists( path))
                        common::directory::create( path);

                     return path;
                  }
               };

               Paths& paths() 
               {
                  static Paths singleton;
                  return singleton;
               }
            } // <unnamed>
         } // local


         namespace directory
         {
            const std::string& domain()
            {
               return local::paths().domain;
            }

            const std::string& temporary()
            {
               static std::string singleton{ platform::directory::temporary};
               return singleton;
            }

            const std::string& casual()
            {
               return local::paths().casual;
            }
         } // directory

         namespace log
         {
            const std::string& path()
            {
               return local::paths().log;
            }
         } // log

         namespace ipc
         {
            const std::string& directory()
            {
                return local::paths().ipc;
            }
         } // transient

         namespace domain
         {
            namespace singleton
            {
               const std::string& file() { return local::paths().singleton;}
            } // singleton
         } // domain


         namespace local
         {
            namespace
            {
               enum class Type
               {
                  text, token
               };

               template< typename T>
               struct Token
               {

                  Token( T value, Type type) :
                     value(std::move(value)), type(type)
                  {
                  }

                  T value;
                  Type type = Type::text;
               };

               template< typename R, typename F, typename L>
               auto split( R&& range, F&& first, L&& last) -> std::vector< Token< view::String>>
               {
                  using token_type = Token< view::String>;
                  std::vector< token_type> result;

                  auto splitted = algorithm::divide_first( range, first);

                  if( std::get< 0>( splitted))
                  {
                     result.emplace_back( std::get< 0>( splitted), Type::text);
                  }

                  auto token = std::get< 1>( splitted);

                  if( token)
                  {

                     // We got a split. Make sure we consume 'first-token'
                     token.advance( view::String( first).size());

                     splitted = algorithm::divide_first( token, last);

                     if( ! std::get< 1>( splitted))
                     {
                        // We did not find the 'last-delimiter'
                        throw exception::system::invalid::Argument{ 
                           string::compose( "syntax error, such as unbalanced parentheses: ", std::string{ std::begin(range), std::end(range)})};
                     }

                     result.emplace_back( std::get< 0>( splitted), Type::token);

                     auto next = std::get< 1>( splitted);
                     next.advance( view::String( last).size());

                     for( auto& value : split( next, first, last))
                     {
                        result.push_back( std::move( value));
                     }
                  }

                  return result;
               }

            } // <unnamed>
         } // local

         std::string string( const std::string& value)
         {
            std::string result;
            result.reserve( value.size());

            for( auto& token : local::split( value, "${", "}"))
            {
               switch( token.type)
               {
               case local::Type::text:
               {
                  result.append( std::begin( token.value), std::end( token.value));
                  break;
               }
               case local::Type::token:
               {
                  result += variable::get( std::string{ std::begin( token.value), std::end( token.value)});
                  break;
               }
               }
            }

            return result;
         }

         void reset()
         {
            local::paths() = local::Paths{};

            common::log::line( common::verbose::log, "paths: ", local::paths());
         }
      } // environment
   } // common
} // casual

