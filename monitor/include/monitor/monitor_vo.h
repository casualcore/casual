#ifndef CASUALMONITORVOVO_H
#define CASUALMONITORVOVO_H




 

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
    
//## includes protected section begin [200.20]
#include <sf/types.h>
#include <string>
//## includes protected section end   [200.20]

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
	 std::string getService() const;
	 sf::Uuid getCallId() const;
	 sf::time_type getStart() const;
	 sf::time_type getEnd() const;

	 void setParentService( const std::string& value);
	 void setService( const std::string& value);
	 void setCallId( const sf::Uuid& value);
	 void setStart( const sf::time_type& value);
	 void setEnd( const sf::time_type& value);
  
   void serialize( casual::sf::archive::Reader& archive);
  
   void serialize( casual::sf::archive::Writer& archive) const;

private:


   //## additional private declarations protected section begin [200.200]
   //## additional private declarations protected section end   [200.200]

   class Implementation;
   std::unique_ptr< Implementation> pimpl;


};


}
}
}
}

#endif 
