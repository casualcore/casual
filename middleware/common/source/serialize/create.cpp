//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/serialize/create.h"
#include "common/serialize/log.h"

#include "common/algorithm.h"

namespace casual
{
   namespace common
   {
      namespace serialize
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
        
               serialize::Writer Dispatch::create( const std::string& key, std::ostream& stream) 
               {
                  Trace trace{ "serialize::create::writer"};

                  return create::detail::create( key, m_creators, stream);

               }

               serialize::Writer Dispatch::create( const std::string& key, platform::binary::type& data)
               {
                  Trace trace{ "serialize::create::writer"};

                  return create::detail::create( key, m_creators, data);
               }

               std::vector< std::string> Dispatch::keys() const
               {
                  return algorithm::transform( m_creators, []( auto& pair){ return pair.first;});
               }


            } // writer
         } // create
      } // serialize
   } // common
} // casual