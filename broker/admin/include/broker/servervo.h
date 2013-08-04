#ifndef CASUALBROKERADMINSERVERVO_H
#define CASUALBROKERADMINSERVERVO_H




 

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
    
//## includes protected section begin [300.10]
//## includes protected section end   [300.10]

//## additional declarations protected section begin [300.20]
//## additional declarations protected section end   [300.20]

namespace casual
{
namespace broker
{
namespace admin
{


class ServerVO
{
public:

   //## additional public declarations protected section begin [300.100]
   //## additional public declarations protected section end   [300.100]


   ServerVO();
   ~ServerVO();
   ServerVO( const ServerVO&);
   ServerVO& operator = ( const ServerVO&);
   ServerVO( ServerVO&&);
   ServerVO& operator = (ServerVO&&);


   //## additional public declarations protected section begin [300.110]
   //## additional public declarations protected section end   [300.110]
   //
   // Getters and setters
   //
   
   
   long getPid() const;
   void setPid( long value);


   std::string getPath() const;
   void setPath( std::string value);


   long getQueue() const;
   void setQueue( long value);


   bool getIdle() const;
   void setIdle( bool value);



  
   void serialize( casual::sf::archive::Reader& archive);
  
   void serialize( casual::sf::archive::Writer& archive) const;

private:


   //## additional private declarations protected section begin [300.200]
   //## additional private declarations protected section end   [300.200]

   class Implementation;
   std::unique_ptr< Implementation> pimpl;


};


} // admin
} // broker
} // casual


#endif 
