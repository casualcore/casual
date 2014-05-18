


//## includes protected section begin [.10]
#include "transaction/manager/admin/adminimplementation.h"

//## includes protected section end   [.10]

namespace casual
{
namespace transaction
{


//## declarations protected section begin [.20]
//## declarations protected section end   [.20]

AdminImplementation::AdminImplementation( int argc, char **argv)
{
   //## constructor protected section begin [ctor.20]
   //## constructor protected section end   [ctor.20]
 }



AdminImplementation::~AdminImplementation()
{
   //## destructor protected section begin [dtor.20]
   //## destructor protected section end   [dtor.20]
}


//
// Services definitions
//


std::vector< vo::Transaction> AdminImplementation::casual_listTransactions( )
{
   //## service implementation protected section begin [300]

   std::vector< vo::Transaction> result;



   return result;

   //## service implementation protected section end   [300]
}

//## declarations protected section begin [.40]
//## declarations protected section end   [.40]

} // transaction
} // casual

//## declarations protected section begin [.50]
//## declarations protected section end   [.50]
