//!
//! casual
//!

#include "common/environment.h"
#include "common/exception/system.h"
#include "common/file.h"
#include "common/algorithm.h"
#include "common/log.h"
#include "common/string.h"

#include <memory>

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
                        static const Variable singleton;
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

                        //
                        // We need to return by value and copy the variable while
                        // we have the lock
                        //
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

            namespace process
            {
               common::process::Handle get( const char* variable)
               {
                  auto value = common::environment::variable::get( variable);

                  common::process::Handle result;
                  {
                     auto split = range::divide(value, '|');

                     auto& pid = std::get < 0 > ( split);
                     if( !pid.empty())
                     {
                        result.pid = std::stoi( std::string( std::begin(pid), std::end(pid)));
                     }

                     auto queue = std::get < 1 > ( split);
                     if( ! queue.empty())
                     {
                        ++queue;
                        result.queue = communication::ipc::Handle{ 
                           common::from_string< decltype( result.queue.native())>( std::string( std::begin(queue), std::end(queue)))};
                     }
                  }

                  return result;
               }

               void set( const char* variable, const common::process::Handle& process)
               {
                  variable::set( variable, string::compose( process.pid, '|', process.queue));
               }

            } // process


         } // variable

         namespace directory
         {
            const std::string& domain()
            {
               static const std::string result = variable::get( variable::name::domain::home());
               return result;
            }

            const std::string& temporary()
            {
               static const std::string result = "/tmp";
               return result;
            }

            const std::string& casual()
            {
               static const std::string result = variable::get( variable::name::home());
               return result;
            }

         }


         namespace domain
         {
            namespace singleton
            {
               namespace local
               {
                  namespace
                  {
                     std::string path( std::string path)
                     {
                        common::directory::create(path);
                        return path;
                     }

                  } // <unnamed>
               } // local

               const std::string& path()
               {
                  static const std::string path = local::path(directory::domain() + "/.singleton");
                  return path;
               }

               const std::string& file()
               {
                  static const std::string file = path() + "/.domain-singleton";
                  return file;

               }

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
               auto split( R&& range, F&& first, L&& last) -> std::vector< Token< decltype( range::make( range))>>
               {
                  using token_type = Token< decltype( range::make( range))>;
                  std::vector< token_type> result;

                  auto splitted = range::divide_first(range, first);

                  if( std::get < 0 > ( splitted))
                  {
                     result.emplace_back(std::get < 0 > ( splitted), Type::text);
                  }

                  auto token = std::get < 1 > ( splitted);

                  if( token)
                  {
                     //
                     // We got a split. Make sure we consume 'first-token'
                     //
                     token.advance(range::make(first).size());

                     splitted = range::divide_first(token, last);

                     if( !std::get < 1 > ( splitted))
                     {
                        //
                        // We did not find the 'last-delimiter'
                        //
                        throw exception::system::invalid::Argument{ 
                           string::compose( "syntax error, such as unbalanced parentheses: ", std::string{ std::begin(range), std::end(range)})};
                     }

                     result.emplace_back(std::get < 0 > ( splitted), Type::token);

                     auto next = std::get < 1 > ( splitted);
                     next.advance(range::make(last).size());

                     for( auto& value : split(next, first, last))
                     {
                        result.push_back(std::move(value));
                     }
                  }

                  return result;
               }

            } // <unnamed>
         } // local

         std::string string( const std::string& value)
         {
            std::string result;
            result.reserve(value.size());

            for( auto& token : local::split(value, std::string { "${"}, std::string { "}"}))
            {
               switch( token.type)
               {
               case local::Type::text:
               {
                  result.append(std::begin(token.value), std::end(token.value));
                  break;
               }
               case local::Type::token:
               {
                  result += variable::get(std::string { std::begin(token.value), std::end(token.value)});
                  break;
               }
               }
            }

            return result;
         }
      } // environment
   } // common
} // casual

