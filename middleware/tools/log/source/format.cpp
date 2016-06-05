//!
//! casual 
//!


#include <functional>
#include <algorithm>
#include <string>
#include <iterator>
#include <array>
#include <iostream>

namespace casual
{
   namespace log
   {
      namespace format
      {
         struct Settings
         {

         };




         namespace string
         {
            struct view
            {
               using iter = std::string::iterator;

               view() = default;
               view( iter first, iter last) : first( first), last( last) {}

               iter first;
               iter last;


               auto size() const -> typename std::iterator_traits< iter>::difference_type
               {
                  return std::distance( first, last);
               }

               friend std::ostream& operator << ( std::ostream& out, const view& value)
               {
                  out.write( &( *value.first), value.size());
                  return out;
               }
            };

            struct formatter : view
            {
               using formatter_type = std::function< void( std::ostream&, const view&)>;
               using view::view;

               formatter_type formatter;

               friend std::ostream& operator << ( std::ostream& out, const string::formatter& value)
               {
                  value.formatter( out, value);
                  return out;
               }
            };

         } // string

         namespace color
         {
            constexpr auto end() -> decltype( "\033[0m")
            {
               return "\033[0m";
            }


            struct basic_color
            {
               //basic_color( std::array< char, 11> value) : value{ std::move( value)} {}
               basic_color( const char* const value) : value{ value} {}

               void operator() ( std::ostream& out, const string::view& view)
               {
                  //out << value.data() << view << end();
                  out << value << view << end();
               }

               //std::array< char, 11> value;
               const char* const value;
            };

            basic_color white{ "\033[0;37m"};
            basic_color yellow{ "\033[0;33m"};

         } // color

         namespace format
         {
            void raw( std::ostream& out, const string::view& view)
            {
               out << view << '|';
            }

            void timestamp( std::ostream& out, const string::view& view)
            {
               // 2016-04-23T11:12:57.733
               if( view.size() > 19)
               {
                  out << string::view{ view.first, view.first + 11};
                  color::white( out, { view.first + 11, view.last});
               }
               else
               {
                  out << view;
               }
               out << '|';
            }

            void execution( std::ostream& out, const string::view& view)
            {
               //b0ece06ba1f94d1d8354a26fbc316057|
               //b0ece06ba1f94|
               if( view.size() > 13) { color::yellow( out, { view.first, view.first + 13});}
               else { out << view;}
               out << '|';
            }

            void transaction( std::ostream& out, const string::view& view)
            {
               // 7d47a6a7b043407299bd467b9ff144c8:05bff118d76949268e5d873cb74be3df:42:21971:360841220
               //                     467b9ff144c8:05bff11
               if( view.size() > 40) { color::yellow( out, { view.first + 20, view.first + 40});}
               else { out << view;}
               out << '|';

            }

            void information( std::ostream& out, const string::view& view)
            {
               out << view;
            }

         } // formatters

         namespace splitted
         {
            enum Index
            {
               timestamp = 0,
               domain,
               execution,
               transaction,
               pid,
               thread,
               executable,
               parent,
               service,
               category,
               information,
            };
            using type = std::array< string::formatter, 11>;

            type crate( const Settings& settings)
            {
               type result;

               result[ Index::timestamp].formatter = &format::timestamp;
               result[ Index::domain].formatter = &format::raw;
               result[ Index::execution].formatter = &format::execution;
               result[ Index::transaction].formatter = &format::transaction;
               result[ Index::pid].formatter = &format::raw;
               result[ Index::thread].formatter = &format::raw;
               result[ Index::executable].formatter = &format::raw;
               result[ Index::parent].formatter = &format::raw;
               result[ Index::service].formatter = &format::raw;
               result[ Index::category].formatter = &format::raw;
               result[ Index::information].formatter = &format::information;

               return result;
            }

         } // splitted

         bool split( std::string& line, splitted::type& splitted)
         {
            auto current = std::begin( line);
            const auto last = std::end( line);
            auto split_current = std::begin( splitted);


            for( ; split_current != std::end( splitted) - 1; ++split_current)
            {
               split_current->first = current;
               split_current->last = std::find( current, last, '|');

               if( split_current->last == last)
               {
                  return false;
               }
               current = split_current->last + 1;
            }

            split_current->first = current;
            split_current->last = last;

            return true;
         }



         void transform( const Settings& settings, std::istream& in, std::ostream& out)
         {

            auto splitted = splitted::crate( settings);
            std::string line;
            line.reserve( 1024);

            while( std::getline( in, line))
            {
               if( split( line, splitted))
               {
                  for( auto& view : splitted)
                  {
                     out << view;
                  }
                  out << '\n';
               }
               else
               {
                  out << line << '\n';
               }
            }
         }


         int main( int argc, char **argv)
         {
            Settings settings;
            transform( settings, std::cin, std::cout);

            return 0;
         }

      } // format

   } // log

} // casual


int main( int argc, char **argv)
{
   return casual::log::format::main( argc, argv);
}
