//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/exception/handle.h"
#include "common/argument.h"
#include "common/file.h"
#include "common/log.h"

#include <iostream>
#include <fstream>
#include <regex>

namespace casual
{
   namespace tools
   {
      namespace file
      {
         namespace
         {
            struct Settings
            {
               std::string headerfile;
               std::vector< std::string> files;
               std::string predicate = R"(^((//)|(/\*)|([ ]\*)).*)";
            };


            struct Directive
            {
               std::string header;
               std::regex predicate;
            };

            constexpr auto default_header() 
            {
               return R"(//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

)";
            }

            Directive directive( Settings settings)
            {
               Directive result;
               result.predicate.assign( settings.predicate);

               if( ! settings.headerfile.empty())
               {
                  std::ifstream file{ settings.headerfile};
                  std::string line;
                  while( std::getline( file, line))
                  {
                     result.header += line + '\n';
                  }
               }
               else 
               {
                  result.header = default_header();
               }
               return result;
            }

            void header( const Directive& directive, const std::string& file)
            {
               std::ifstream input{ file};
               std::string line;

               // consume while predicate is true
               while( std::getline( input, line) && std::regex_match( line, directive.predicate))
                  ; // std::cout << line << '\n'; // no-op

               // create temporary file
               common::file::scoped::Path guard{ common::file::name::unique( file)};
               std::ofstream output{ guard};

               output << directive.header;
               
               // make sure we put back the first line where predicate was false
               if( ! std::regex_match( line, directive.predicate))
                  output << line << '\n';

               // write the rest of the file.
               output << input.rdbuf();

               common::file::move( guard, file);
               guard.release();
            }

            void header( Settings settings)
            {
               const auto files = std::move( settings.files);
               const auto directive = file::directive( std::move( settings));

               for( auto& file : files)
                  header( directive, file);
            }


            int main( int argc, char* argv[])
            {
               try
               {
                  Settings settings;
                  {
                     common::argument::Parse parse{ "add a header to every file passed by --files",
                        common::argument::Option( std::tie( settings.headerfile), { "-h", "--header-file"}, "the header to use"),
                        common::argument::Option( std::tie(  settings.files), { "-f", "--files"}, "files to replace header")
                     };
                     parse( argc, argv);
                  }
                  header( std::move( settings));
               }
               catch( ...)
               {
                  common::log::line( std::cerr, common::exception::capture());
               }
               return 0;
            }
            
         } // <unnamed>
      } // file
   } // tools
} // casual


int main( int argc, char* argv[])
{
   return casual::tools::file::main( argc, argv);
} 
