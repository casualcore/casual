#ifndef CASUALBROKERADMINSERVICEVO_H
#define CASUALBROKERADMINSERVICEVO_H




 

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
    
//## includes protected section begin [400.10]
//## includes protected section end   [400.10]

//## additional declarations protected section begin [400.20]
//## additional declarations protected section end   [400.20]

namespace casual
{
namespace broker
{
namespace admin
{


class ServiceVO
{
public:

   //## additional public declarations protected section begin [400.100]
   //## additional public declarations protected section end   [400.100]


   ServiceVO();
   ~ServiceVO();
   ServiceVO( const ServiceVO&);
   ServiceVO& operator = ( const ServiceVO&);
   ServiceVO( ServiceVO&&);
   ServiceVO& operator = (ServiceVO&&);


   //## additional public declarations protected section begin [400.110]
   //## additional public declarations protected section end   [400.110]
   //
   // Getters and setters
   //
   
   
   std::string getNameF() const;
   void setNameF( std::string value);


   long getTimeoutF() const;
   void setTimeoutF( long value);


   std::vector< long> getPids() const;
   void setPids( std::vector< long> value);



  
   void serialize( casual::sf::archive::Reader& archive);
  
   void serialize( casual::sf::archive::Writer& archive) const;

private:


   //## additional private declarations protected section begin [400.200]
   //## additional private declarations protected section end   [400.200]

   class Implementation;
   std::unique_ptr< Implementation> pimpl;


};


} // admin
} // broker
} // casual


#endif 
