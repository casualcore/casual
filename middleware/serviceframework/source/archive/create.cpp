//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "serviceframework/archive/create.h"

#include "serviceframework/log.h"

#include "common/algorithm.h"

namespace casual
{
   namespace serviceframework
   {
      namespace archive
      {
         namespace create
         {
            namespace writer
            {

               Dispatch& Dispatch::instance()
               {
                  static Dispatch singleton;
                  return singleton;
               }

               Dispatch::Dispatch() = default;
        
               archive::Writer Dispatch::create( const std::string& key, std::ostream& stream) 
               {
                  Trace trace{ "serviceframework::archive::create::writer"};

                  return create::detail::create( key, m_creators, stream);

               }

               archive::Writer Dispatch::create( const std::string& key, platform::binary::type& data)
               {
                  Trace trace{ "serviceframework::archive::create::writer"};

                  return create::detail::create( key, m_creators, data);
               }


            } // writer
         } // create
      } // archive
   } // serviceframework
} // casual