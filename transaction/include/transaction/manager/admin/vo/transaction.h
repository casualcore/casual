#ifndef VOTRANSACTION_H
#define VOTRANSACTION_H




 

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
//## includes protected section end   [200.10]

//## additional declarations protected section begin [200.20]
//## additional declarations protected section end   [200.20]

namespace vo
{


class Transaction
{
public:

   //## additional public declarations protected section begin [200.100]
   //## additional public declarations protected section end   [200.100]


   Transaction();
   ~Transaction();
   Transaction( const Transaction&);
   Transaction& operator = ( const Transaction&);
   Transaction( Transaction&&);
   Transaction& operator = (Transaction&&);


   //## additional public declarations protected section begin [200.110]
   //## additional public declarations protected section end   [200.110]
   //
   // Getters and setters
   //
   
   
   std::string getXid() const;
   void setXid( std::string value);


   //!
   //! State of the transaction
   //!
   //! @{
   long getState() const;
   void setState( long value);
   //! @}



  
   void serialize( casual::sf::archive::Reader& archive);
  
   void serialize( casual::sf::archive::Writer& archive) const;

private:


   //## additional private declarations protected section begin [200.200]
   //## additional private declarations protected section end   [200.200]

   class Implementation;
   std::unique_ptr< Implementation> pimpl;


};


} // vo


#endif 
