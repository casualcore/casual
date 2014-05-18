#ifndef CASUALTRANSACTIONADMINIMPLEMENTATION_H
#define CASUALTRANSACTIONADMINIMPLEMENTATION_H


//## includes protected section begin [.10]

#include "transaction/manager/admin/vo/transaction.h"

#include <vector>


//## includes protected section end   [.10]

namespace casual
{
namespace transaction
{


//## declarations protected section begin [.20]
//## declarations protected section end   [.20]


class AdminImplementation
{

public:
    
   //
   // Constructor and destructor. 
   // use these to initialize state if needed
   //
   AdminImplementation( int argc, char **argv);
    
   ~AdminImplementation();
    

   //## declarations protected section begin [.200]
   //## declarations protected section end   [.200]
    
    
   //
   // Services
   //
    
   
   //!
   //! List all current transactions 
   //! 
   //! @return list of transacions
   //!
   std::vector< vo::Transaction> casual_listTransactions( );

   //## declarations protected section begin [.300]
   //## declarations protected section end   [.300]

};


//## declarations protected section begin [.40]
//## declarations protected section end   [.40]

} // transaction
} // casual

//## declarations protected section begin [.50]
//## declarations protected section end   [.50]
#endif 
