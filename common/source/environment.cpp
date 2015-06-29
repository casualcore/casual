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
#include "common/internal/log.h"
#include "common/trace.h"


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
						throw exception::invalid::environment::Variable( "failed to get variable", CASUAL_NIP( name));
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


			//
			// wordexp is way to slow, 10-30ms which quickly adds up...
			// we roll our own until we find something better
			//
			/*
         std::string string( const std::string& value)
         {
            wordexp_t holder;
            common::initialize( holder);

            scope::Execute deleter{ [&](){ wordfree( &holder);}};


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

            log::internal::debug << "environment::string - we_wordc: " << holder.we_wordc << " we_offs: " << holder.we_offs << '\n';

            //
            // We join with ' '. Not sure if this is what we always want
            //
            return string::join( range::make( holder.we_wordv, holder.we_wordv + holder.we_wordc), " ");
         }
         */

			namespace local
         {
            namespace
            {
               enum class Type
               {
                  text,
                  token
               };


               template< typename T>
               struct Token
               {

                  Token( T value, Type type) : value( std::move( value)), type( type) {}

                  T value;
                  Type type = Type::text;
               };

               template< typename R, typename F, typename L>
               auto split( R&& range, F&& first, L&& last) -> std::vector< Token< decltype( range::make( range))>>
               {
                  using token_type = Token< decltype( range::make( range))>;
                  std::vector< token_type> result;

                  auto splitted = range::divide_first( range, first);

                  if( std::get< 0>( splitted))
                  {
                     result.emplace_back( std::get< 0>( splitted), Type::text);
                  }


                  auto token = std::get< 1>( splitted);

                  if( token)
                  {
                     //
                     // We got a split. Make sure we consume 'first-token'
                     //
                     token.first += range::make( first).size();


                     splitted = range::divide_first( token, last);

                     if( ! std::get< 1>( splitted))
                     {
                        //
                        // We did not find the 'last-delimiter'
                        //
                        throw exception::invalid::Argument{ "syntax error, such as unbalanced parentheses",
                           exception::make_nip( "value", std::string{ std::begin( range), std::end( range)})};
                     }


                     result.emplace_back( std::get< 0>( splitted), Type::token);

                     auto next = std::get< 1>( splitted);
                     next.first += range::make( last).size();

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

            for( auto& token : local::split( value, std::string{ "${"}, std::string{ "}"}))
            {
               switch( token.type)
               {
                  case local::Type::text:
                  {
                     result.append( token.value.first, token.value.last);
                     break;
                  }
                  case local::Type::token:
                  {
                     result += variable::get( std::string{ token.value.first, token.value.last});
                     break;
                  }
               }
            }


            return result;
         }
		} // environment
	} // common
} // casual


