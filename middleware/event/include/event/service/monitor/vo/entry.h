//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/pimpl.h"
 

// std
#include <memory>
    
// Forwards
namespace casual 
{ 
   namespace common 
   { 
      namespace serialize 
      {
         class Reader;
         class Writer;
      }
    }
}
    
//## includes protected section begin [200.10]

#include "common/uuid.h"

#include <string>
//## includes protected section end   [200.10]

//## additional declarations protected section begin [200.20]
//## additional declarations protected section end   [200.20]

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
               class Entry
               {
               public:

                  //## additional public declarations protected section begin [200.100]
                  //## additional public declarations protected section end   [200.100]


                  Entry();
                  ~Entry();
                  Entry( const Entry&);
                  Entry& operator = ( const Entry&);
                  Entry( Entry&&);
                  Entry& operator = (Entry&&);


                  //## additional public declarations protected section begin [200.110]
                  //## additional public declarations protected section end   [200.110]
                  //
                  // Getters and setters
                  //


                  std::string getParentService() const;
                  void setParentService( std::string value);


                  std::string getService() const;
                  void setService( std::string value);


                  common::Uuid getCallId() const;
                  void setCallId( common::Uuid value);


                  platform::time::point::type getStart() const;
                  void setStart( platform::time::point::type value);


                  platform::time::point::type getEnd() const;
                  void setEnd( platform::time::point::type value);

                  void serialize( casual::common::serialize::Reader& archive);
                  void serialize( casual::common::serialize::Writer& archive) const;

               private:


                  //## additional private declarations protected section begin [200.200]
                  //## additional private declarations protected section end   [200.200]

                  struct Implementation;
                  common::basic_pimpl< Implementation> pimpl;
               };
            } // vo
         } // service
      } // monitor
   } // traffic
} // casual


