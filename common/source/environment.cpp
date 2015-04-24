//!
//! casual_utility_environment.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include "common/environment.h"
#include "common/exception.h"
#include "common/file.h"
#include "common/algorithm.h"


#include <memory>

#include <cstdlib>
#include <ctime>
#include <wordexp.h>


namespace casual
{
	namespace common
	{
		namespace environment
		{
			namespace variable
			{
				bool exists( const std::string& name)
				{
					return getenv( name.c_str()) != nullptr;
				}

				std::string get( const std::string& name)
				{
					const char* const result = getenv( name.c_str());

					if( result)
					{
						return result;
					}
					else
					{
						throw exception::EnvironmentVariableNotFound( name);
					}
				}

            std::string get( const std::string& name, const std::string& alternative)
            {
               if( exists( name))
               {
                  return get( name);
               }
               return alternative;
            }

				void set( const std::string& name, const std::string& value)
				{
				   setenv( name.c_str(), value.c_str(), 1);
				}
			}


			namespace directory
			{
			   const std::string& domain()
            {
               static const std::string result = variable::get( "CASUAL_DOMAIN_HOME");
               return result;
            }

            const std::string& temporary()
            {
               static const std::string result = "/tmp";
               return result;
            }

            const std::string& casual()
            {
               static const std::string result = variable::get( "CASUAL_HOME");
               return result;
            }

         }

			namespace file
         {

            std::string brokerQueue()
            {
               return domain::singleton::path() + "/.casual-broker-queue";
            }

            std::string configuration()
            {
               return common::file::find( directory::domain() + "/configuration", std::regex( "domain.(yaml|xml)" ));
            }

            std::string installedConfiguration()
            {
               return common::file::find( directory::casual() + "/configuration", std::regex( "resources.(yaml|xml)" ));
            }

         }


			namespace local
         {
            namespace
            {
               std::string& domainName()
               {
                  static std::string path;
                  return path;
               }
            }
		   }

			namespace domain
         {
            const std::string& name()
            {
               return local::domainName();
            }

            void name( const std::string& value)
            {
               local::domainName() = value;
            }

            namespace singleton
            {
               namespace local
               {
                  namespace
                  {
                     std::string path( std::string path)
                     {
                        common::directory::create( path);
                        return path;
                     }

                  } // <unnamed>
               } // local

               const std::string& path()
               {
                  static const std::string path = local::path( directory::domain() + "/.singleton");
                  return path;
               }

            } // singleton

         } // domain


			namespace local
         {
            namespace
            {
               struct wordexp_deleter
               {
                  void operator()( wordexp_t* holder) const
                  {
                     wordfree( holder);
                  }
               };
            } // <unnamed>
         } // local


         std::string string( const std::string& value)
         {

            wordexp_t holder;
            common::initialize( holder);

            std::unique_ptr< wordexp_t, local::wordexp_deleter> deleter( &holder);


            auto result = wordexp( value.c_str(), &holder, WRDE_UNDEF | WRDE_NOCMD);

            switch( result)
            {
               case 0:
                  break;
               case WRDE_BADCHAR:
               {
                  throw exception::invalid::Argument{ "Illegal occurrence of newline or one of |, &, ;, <, >, (, ), {, }", CASUAL_NIP( value)};
               }
               case WRDE_BADVAL:
               {
                  throw exception::invalid::Argument{ "An undefined shell variable was referenced", CASUAL_NIP( value)};
               }
               case WRDE_CMDSUB:
               {
                  throw exception::invalid::Argument{ "Command substitution occurred", CASUAL_NIP( value)};
               }
               case WRDE_NOSPACE:
               {
                  throw exception::limit::Memory{ "Out of memory", CASUAL_NIP( value)};
               }
               case WRDE_SYNTAX:
               {
                  throw exception::invalid::Argument{ "Shell syntax error, such as unbalanced parentheses or unmatched quotes", CASUAL_NIP( value)};
               }
            }

            switch( holder.we_wordc)
            {
               case 0:
                  return {};
               case 1:
               {
                  auto word = holder.we_wordv;
                  return std::string{ word[ 0]};
               }
               default:
               {
                  throw exception::invalid::Argument{ "string substitution resulted in more than one string", CASUAL_NIP( value)};
               }
            }
         }
		} // environment
	} // common
} // casual


