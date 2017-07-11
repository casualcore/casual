//!
//! casual 
//!

#include "sf/log.h"


namespace casual
{

   namespace sf
   {
      namespace log
      {
         common::log::Stream sf{ "casual.sf"};

      } // log

      namespace trace
      {
         namespace detail
         {
               Scope::~Scope() = default;

               Scope::Scope( const char* information, std::ostream& log)
                  : common::log::trace::basic::Scope( information, log) {}

         } // detail

      } // trace


   } // sf

} // casual
