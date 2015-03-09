//!
//! terminal.h
//!
//! Created on: Dec 22, 2014
//!     Author: Lazan
//!

#ifndef COMMON_TERMINAL_H_
#define COMMON_TERMINAL_H_

#include "common/move.h"

#include <string>
#include <ostream>

namespace casual
{
   namespace common
   {
      namespace terminal
      {

         struct color_t
         {
            color_t( std::string color);

            struct proxy_t
            {
               proxy_t( std::ostream& out);
               ~proxy_t();

               proxy_t( proxy_t&&);
               proxy_t& operator = ( proxy_t&&);


               template< typename T>
               std::ostream& operator << ( T&& value)
               {
                  return *m_out << std::forward< T>( value);
               }

            private:
               std::ostream* m_out;
               move::Moved m_moved;
            };

            const std::string& escape() const;
            std::string reset() const;

            friend proxy_t operator << ( std::ostream& out, const color_t& color);

         private:
            std::string m_color;

         };

         namespace color
         {
            extern color_t red;
            extern color_t grey;
            extern color_t red;
            extern color_t green;
            extern color_t yellow;
            extern color_t blue;
            extern color_t magenta;
            extern color_t cyan;
            extern color_t white;

            struct Solid
            {
               Solid( color_t& color) : m_color( color) {}

               template< typename T>
               void operator () ( std::ostream& out, T&& value)
               {
                  out << m_color << value;
               }
            private:
               color_t& m_color;
            };

         } // color
      } // terminal

   } // common
} // casual

#endif // TERMINAL_H_
