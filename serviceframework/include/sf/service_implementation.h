//!
//! service_implementation.h
//!
//! Created on: Jan 4, 2013
//!     Author: Lazan
//!

#ifndef SERVICE_IMPLEMENTATION_H_
#define SERVICE_IMPLEMENTATION_H_

#include "sf/service.h"

namespace casual
{
   namespace sf
   {
      namespace service
      {
         namespace implementation
         {
            class Base : public Interface
            {

            private:

               bool doCall() override
               {
                  return true;
               }
               void doFinalize() override
               {

               }

               Interface::Input& doInput() override
               {
                  return m_input;
               }

               Interface::Output& doOutput() override
               {
                  return m_output;
               }


            protected:

               Interface::Input m_input;
               Interface::Output m_output;
            };


            class Binary : public Base
            {


            private:

            };


         } // implementation

      } // service
   } // sf
} // casual


#endif /* SERVICE_IMPLEMENTATION_H_ */
