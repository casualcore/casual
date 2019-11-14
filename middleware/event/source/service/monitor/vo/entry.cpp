//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


      

#include "event/service/monitor/vo/entry.h"
#include "common/serialize/archive.h"

//## includes protected section begin [200.20]


//## includes protected section end   [200.20]

namespace casual
{
   namespace event
   {
      namespace service
      {
         namespace monitor
         {
            namespace vo
            {

               struct Entry::Implementation
               {
                   Implementation() // NOLINT
                  //## initialization list protected section begin [200.40]
                  //## initialization list protected section end   [200.40]
                  {
                     //## ctor protected section begin [200.impl.ctor.10]
                     //## ctor protected section end   [200.impl.ctor.10]
                  }


                  template< typename A>
                  void serialize( A& archive)
                  {
                     //## additional serialization protected section begin [200.impl.serial.10]
                     //## additional serialization protected section end   [200.impl.serial.10]
                     CASUAL_SERIALIZE( parentService);
                     CASUAL_SERIALIZE( service);
                     CASUAL_SERIALIZE( callId);
                     CASUAL_SERIALIZE( start);
                     CASUAL_SERIALIZE( end);
                     //## additional serialization protected section begin [200.impl.serial.20]
                     //## additional serialization protected section end   [200.impl.serial.20]
                  }

                  //## additional attributes protected section begin [200.impl.attr.10]
                  //## additional attributes protected section end   [200.impl.attr.10]
                  std::string parentService;
                  std::string service;
                  common::Uuid callId;
                  platform::time::point::type start;
                  platform::time::point::type end;
                  //## additional attributes protected section begin [200.impl.attr.20]
                  //## additional attributes protected section end   [200.impl.attr.20]

               };




               Entry::Entry() // NOLINT
               {
                  //## base class protected section begin [200.ctor.10]
                  //## base class protected section end   [200.ctor.10]
               }

               Entry::~Entry() = default;
               Entry::Entry( Entry&&  rhs) = default;
               Entry& Entry::operator = (Entry&&) = default;
               Entry::Entry( const Entry& rhs) = default;
               Entry& Entry::operator = ( const Entry& rhs) = default;

               std::string Entry::getParentService() const
               {
                  return pimpl->parentService;
               }
               std::string Entry::getService() const
               {
                  return pimpl->service;
               }
               common::Uuid Entry::getCallId() const
               {
                  return pimpl->callId;
               }
               platform::time::point::type Entry::getStart() const
               {
                  return pimpl->start;
               }
               platform::time::point::type Entry::getEnd() const
               {
                  return pimpl->end;
               }


               void Entry::setParentService( std::string value)
               {
                  pimpl->parentService = value;
               }
               void Entry::setService( std::string value)
               {
                  pimpl->service = value;
               }
               void Entry::setCallId( common::Uuid value)
               {
                  pimpl->callId = value;
               }
               void Entry::setStart( platform::time::point::type value)
               {
                  pimpl->start = value;
               }
               void Entry::setEnd( platform::time::point::type value)
               {
                  pimpl->end = value;
               }


               void Entry::serialize( casual::common::serialize::Reader& archive)
               {
                   pimpl->serialize( archive);
               }

               void Entry::serialize( casual::common::serialize::Writer& archive) const
               {
                   pimpl->serialize( archive);
               }


            } // vo
         } // monitor
      } // service
   } // traffic
} // casual

   
