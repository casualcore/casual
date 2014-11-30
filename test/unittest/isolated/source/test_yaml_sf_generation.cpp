//!
//! test_yaml_sf_generation.cpp
//!
//! Created on: Feb 3, 2013
//!     Author: Lazan
//!



#include <gtest/gtest.h>


#include "sf/archive/yaml.h"


#include <sstream>



namespace casual
{


   std::string getDeclaration()
   {
      return R"(

namespace: casual::test

value_object:
  name: SomeValueVO
  language: C++
  namespace: vo
  
  output_headers: include/
  output_source:  source/
  
  attributes:
    - name: someLong
      type: long
      
    - name: someString
      type: std::sring


server:
  name: SomeServer
  language: C++
  
  output_headers: include/
  output_source:  source/
  
  services:
    - &casual_sf_test1
      name: casual_sf_test1
      documentation: |
        Does some stuff 
        
        @return true if some condition is met
        @param values holds some values
        
    
      return: bool
      
      arguments:
        - name: values
          type: const std::vector< vo::Value>&
          
        - name: outputValues
          type: std::vector< vo::Value>&
          
    - &casual_sf_test2
      name: casual_sf_test2
      documentation: |
        Secret stuff...
        
      return: void
      
      arguments:
        - name: someValue
          type: bool
          

proxy:
  name: SomeProxy
  language: C++
  namespace: proxy
  
  output_headers: include/
  output_source:  source/
  
  functions:
    - name: doStuff
      reference: *casual_sf_test1
  
)";


   }


   struct Module
   {

      std::string name;
      std::string documentation;
      std::string language;
      std::string name_space;

      std::string output_headers;
      std::string output_source;


      template< typename A>
      void serialize( A& archive)
      {
         archive & CASUAL_MAKE_NVP( name);
         archive & CASUAL_MAKE_NVP( documentation);
         archive & CASUAL_MAKE_NVP( language);
         archive & sf::makeNameValuePair( "namespace", name_space);
         archive & CASUAL_MAKE_NVP( output_headers);
         archive & CASUAL_MAKE_NVP( output_source);

      }
   };

   struct Argument
   {
      std::string name;
      std::string type;

      template< typename A>
      void serialize( A& archive)
      {
         archive & CASUAL_MAKE_NVP( name);
         archive & CASUAL_MAKE_NVP( type);
      }
   };

   struct Service
   {
      std::string name;
      std::string documentation;
      std::string return_type;

      std::vector< Argument> arguments;


      template< typename A>
      void serialize( A& archive)
      {
         archive & CASUAL_MAKE_NVP( name);
         archive & CASUAL_MAKE_NVP( documentation);
         archive & sf::makeNameValuePair( "return", return_type);
         archive & CASUAL_MAKE_NVP( arguments);
      }

   };

   struct Server : public Module
   {

      std::vector< Service> services;

      template< typename A>
      void serialize( A& archive)
      {
         Module::serialize( archive);

         archive & CASUAL_MAKE_NVP( services);
      }
   };


   struct Declaration
   {

      Server server;

      template< typename A>
      void serialize( A& archive)
      {
         archive & CASUAL_MAKE_NVP( server);
      }


   };

   struct Function
   {

      std::string name;
      Service reference;

      std::vector< Argument> arguments;


      template< typename A>
      void serialize( A& archive)
      {
         archive & CASUAL_MAKE_NVP( name);
         archive & CASUAL_MAKE_NVP( reference);
      }



   };

   struct Proxy : public Module
   {
      std::vector< Function> functions;

      template< typename A>
      void serialize( A& archive)
      {
         Module::serialize( archive);
         archive & CASUAL_MAKE_NVP( functions);
      }

   };


TEST( casual_yaml_sf_generation, desrialize_server)
{
   std::istringstream yaml( getDeclaration());

   sf::archive::yaml::relaxed::Reader relaxed( yaml);

   Declaration declaration;

   declaration.serialize( relaxed);

   ASSERT_TRUE( declaration.server.services.size() == 2);

   EXPECT_TRUE( declaration.server.services.at( 0).name == "casual_sf_test1");
   EXPECT_TRUE( declaration.server.services.at( 0).return_type == "bool");

   EXPECT_TRUE( declaration.server.services.at( 1).name == "casual_sf_test2");
   EXPECT_TRUE( declaration.server.services.at( 1).return_type == "void");


}

TEST( casual_yaml_sf_generation, desrialize_proxy)
{
   std::istringstream yaml( getDeclaration());

   sf::archive::yaml::relaxed::Reader relaxed( yaml);

   Proxy proxy;

   relaxed >> CASUAL_MAKE_NVP( proxy);

   ASSERT_TRUE( proxy.functions.size()  == 1);

   EXPECT_TRUE( proxy.functions.at( 0).reference.name == "casual_sf_test1");


}

}

