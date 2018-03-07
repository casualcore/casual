//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#ifndef CASUALSTATISTICSMONITORVOMONITORVO_H
#define CASUALSTATISTICSMONITORVOMONITORVO_H




#include "sf/pimpl.h"
 

// 
// std
//
#include <memory>
    
//
// Forwards
//
namespace casual 
{ 
   namespace sf 
   { 
		namespace archive 
		{
		   class Reader;
		   class Writer;
		}
    }
}
    
//## includes protected section begin [200.10]

#include <sf/platform.h>
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


                  sf::platform::Uuid getCallId() const;
                  void setCallId( sf::platform::Uuid value);


                  sf::platform::time::point::type getStart() const;
                  void setStart( sf::platform::time::point::type value);


                  sf::platform::time::point::type getEnd() const;
                  void setEnd( sf::platform::time::point::type value);




                  void serialize( casual::sf::archive::Reader& archive);

                  void serialize( casual::sf::archive::Writer& archive) const;

               private:


                  //## additional private declarations protected section begin [200.200]
                  //## additional private declarations protected section end   [200.200]

                  struct Implementation;
                  casual::sf::Pimpl< Implementation> pimpl;
               };
            } // vo
         } // service
      } // monitor
   } // traffic
} // casual


#endif 
