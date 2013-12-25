#ifndef CASUALSTATISTICSMONITORVOMONITORVO_H
#define CASUALSTATISTICSMONITORVOMONITORVO_H




 

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
namespace statistics
{
namespace monitor
{
namespace vo
{


class MonitorVO
{
public:

   //## additional public declarations protected section begin [200.100]
   //## additional public declarations protected section end   [200.100]


   MonitorVO();
   ~MonitorVO();
   MonitorVO( const MonitorVO&);
   MonitorVO& operator = ( const MonitorVO&);
   MonitorVO( MonitorVO&&);
   MonitorVO& operator = (MonitorVO&&);


   //## additional public declarations protected section begin [200.110]
   //## additional public declarations protected section end   [200.110]
   //
   // Getters and setters
   //
   
   
   std::string getParentService() const;
   void setParentService( std::string value);


   std::string getSrv() const;
   void setSrv( std::string value);


   sf::platform::Uuid getCallId() const;
   void setCallId( sf::platform::Uuid value);


   sf::platform::time_type getStart() const;
   void setStart( sf::platform::time_type value);


   sf::platform::time_type getEnd() const;
   void setEnd( sf::platform::time_type value);



  
   void serialize( casual::sf::archive::Reader& archive);
  
   void serialize( casual::sf::archive::Writer& archive) const;

private:


   //## additional private declarations protected section begin [200.200]
   //## additional private declarations protected section end   [200.200]

   class Implementation;
   std::unique_ptr< Implementation> pimpl;


};


} // vo
} // monitor
} // statistics
} // casual


#endif 
