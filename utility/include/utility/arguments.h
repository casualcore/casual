//!
//! arguments.h
//!
//! Created on: Jan 5, 2013
//!     Author: Lazan
//!

#ifndef ARGUMENTS_H_
#define ARGUMENTS_H_


#include <vector>
#include <memory>
#include <limits>
#include <algorithm>
#include <type_traits>

namespace casual
{
   namespace utility
   {
      namespace argument
      {
         namespace internal
         {
            template< std::size_t min, std::size_t max>
            struct basic_cardinality
            {
               static_assert( min <= max, "not a valid cardinality");
               enum
               {
                  min_value = min,
                  max_value = max
               };
            };


         }

         namespace cardinality
         {
            template< std::size_t min, std::size_t max>
            using Range = internal::basic_cardinality< min, max>;

            template< std::size_t size>
            using Fixed = Range< size, size>;

            template< std::size_t size>
            using Min = Range< size, std::numeric_limits< std::size_t>::max()>;

            template< std::size_t size>
            using Max = Range< 0, size>;

            using One = Fixed< 1>;

            using Zero = Max< 0>;

            using Any = Min< 0>;

            using OneMany = Min< 1>;

         }

         class Base
         {
         public:
            virtual ~Base() {}

            virtual bool option( const std::string& option) const = 0;

            virtual bool consumed() const = 0;

         };

         namespace internal
         {
            template< typename T>
            struct dispatch
            {

            };

         }

         class Directive : public Base
         {
         public:
            Directive( const std::vector< std::string>& options, const std::string& description)
               : m_options( options), m_description( description) {}

            bool option( const std::string& option) const
            {
               return std::find( std::begin( m_options), std::end( m_options), option) != std::end( m_options);
            }

            bool consumed() const
            {
               return false;
            }

         private:
            const std::vector< std::string> m_options;
            const std::string m_description;
         };


         class Group : public Base
         {
         public:

            typedef std::vector< std::unique_ptr< Base>> groups_type;


            Group() = default;

            template< typename... Args>
            Group( Args&&... args)
            {
               add( std::forward< Args>( args)...);
            }

            template< typename T, typename... Args>
            void add( T&& directive, Args&&... args)
            {
               groups_type::value_type value{ new typename std::decay< T>::type{ std::forward< T>( directive)}};
               m_groups.emplace_back( std::move( value));
               add( args...);
            }

            bool option( const std::string& option) const
            {
               return std::find_if(
                     std::begin( m_groups),
                     std::end( m_groups),
                     std::bind( &Base::option, std::placeholders::_1, option)) != std::end( m_groups);
            }

            bool consumed() const
            {
               return std::all_of(
                     std::begin( m_groups),
                     std::end( m_groups),
                     std::bind( &Base::consumed, std::placeholders::_1));
            }


         protected:

            void add() {}

            std::vector< std::unique_ptr< Base>> m_groups;
         };

      } // argument

      class Arguments : private argument::Group
      {
      public:

         template< typename... Args>
         void add( Args&&... args)
         {
            argument::Group::add( std::forward< Args>( args)...);
         }


         bool parse( int argc, const char** argv)
         {
            std::vector< std::string> arguments{ argv, argv + argc};
            return parse( arguments);

         }

         bool parse( const std::vector< std::string>& arguments)
         {
            auto current = std::begin( arguments);

            while( current != std::end( arguments))
            {
               auto handler = std::find_if(
                     std::begin( m_groups),
                     std::end( m_groups),
                     std::bind( &Base::option, std::placeholders::_1, *current));

               if( handler != std::end( m_groups))
               {


               }
            }

            return true;
         }



      };



   } // utility
} // casual



#endif /* ARGUMENTS_H_ */
