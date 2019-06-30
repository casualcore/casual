//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!




#include <gtest/gtest.h>


#include "serviceframework/archive/yaml.h"


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
         CASUAL_SERIALIZE( name);
         CASUAL_SERIALIZE( documentation);
         CASUAL_SERIALIZE( language);
         CASUAL_SERIALIZE( serviceframework::name::value::pair::make( "namespace", name_space));
         CASUAL_SERIALIZE( output_headers);
         CASUAL_SERIALIZE( output_source);

      }
   };

   struct Argument
   {
      std::string name;
      std::string type;

      template< typename A>
      void serialize( A& archive)
      {
         CASUAL_SERIALIZE( name);
         CASUAL_SERIALIZE( type);
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
         CASUAL_SERIALIZE( name);
         CASUAL_SERIALIZE( documentation);
         CASUAL_SERIALIZE( serviceframework::name::value::pair::make( "return", return_type));
         CASUAL_SERIALIZE( arguments);
      }

   };

   struct Server : public Module
   {

      std::vector< Service> services;

      template< typename A>
      void serialize( A& archive)
      {
         Module::serialize( archive);

         CASUAL_SERIALIZE( services);
      }
   };


   struct Declaration
   {

      Server server;

      template< typename A>
      void serialize( A& archive)
      {
         CASUAL_SERIALIZE( server);
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
         CASUAL_SERIALIZE( name);
         CASUAL_SERIALIZE( reference);
      }



   };

   struct Proxy : public Module
   {
      std::vector< Function> functions;

      template< typename A>
      void serialize( A& archive)
      {
         Module::serialize( archive);
         CASUAL_SERIALIZE( functions);
      }

   };


TEST( casual_yaml_sf_generation, desrialize_server)
{
   auto relaxed = serviceframework::archive::yaml::relaxed::reader( getDeclaration()); 

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
   auto relaxed = serviceframework::archive::yaml::relaxed::reader( getDeclaration()); 

   Proxy proxy;

   relaxed >> CASUAL_NAMED_VALUE( proxy);

   ASSERT_TRUE( proxy.functions.size()  == 1);

   EXPECT_TRUE( proxy.functions.at( 0).reference.name == "casual_sf_test1");


}

}

