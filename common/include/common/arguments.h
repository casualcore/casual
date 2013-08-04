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
   namespace common
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

            virtual void assign( const std::string& option, std::vector< std::string>&& values) = 0;

            virtual bool consumed() const = 0;

            virtual void dispatch() const = 0;

         };

         namespace internal
         {
            struct base_dispatch
            {
               virtual ~base_dispatch() = default;
               virtual void operator () ( const std::vector< std::string>& values) const = 0;

            };

            template< typename C, typename F>
            struct dispatch;

            template< typename F>
            struct dispatch< cardinality::Zero, F> : public base_dispatch
            {
               dispatch( F caller) : m_caller( caller) {}

               void operator () ( const std::vector< std::string>& values) const
               {
                  m_caller();
               }

            private:
               F m_caller;

            };

            template< typename C>
            auto find( const std::string& option, C& container) -> decltype( container.begin())
            {
               return std::find_if(
                  std::begin( container),
                  std::end( container),
                  std::bind( &Base::option, std::placeholders::_1, option));
            }

            template< typename Iter>
            Iter find( Iter start, Iter end, Base* base)
            {
               return std::find_if(
                  start,
                  end,
                  std::bind( &Base::option, base, std::placeholders::_1));
            }


         } // internal

         class Directive : public Base
         {
         public:
            Directive( const std::vector< std::string>& options, const std::string& description)
               : m_options( options), m_description( description) {}



            bool option( const std::string& option) const override
            {
               std::cout << "option: " << option << " description: " << m_description << std::endl;
               return std::find( std::begin( m_options), std::end( m_options), option) != std::end( m_options);
            }

            void assign( const std::string& option, std::vector< std::string>&& values) override
            {
               m_values = std::move( values);
               m_assigned = true;
            }

            bool consumed() const override
            {
               return m_assigned;
            }

            void dispatch() const override
            {
               // TODO
            }

         private:
            const std::vector< std::string> m_options;
            const std::string m_description;
            std::vector< std::string> m_values;
            bool m_assigned = false;
         };


         //template< typename C>
         class Group : public Base
         {
         public:

            //typedef C correlation_type;
            typedef std::vector< std::shared_ptr< Base>> groups_type;


            Group() = default;
            Group( Group&&) = default;
            Group( const Group&) = default;

            /*
            template< typename... Args>
            Group( Args&&... args)
            {
               add( std::forward< Args>( args)...);
            }
            */

            template< typename T, typename... Args>
            void add( T&& directive, Args&&... args)
            {
               m_groups.emplace_back( std::make_shared< typename std::decay< T>::type>( std::forward< T>( directive)));
               add( std::forward< Args>(args)...);
            }

            bool option( const std::string& option) const override
            {
               return internal::find( option, m_groups) != std::end( m_groups);
            }

            void assign( const std::string& option, std::vector< std::string>&& values) override
            {
               auto findIter = internal::find( option, m_groups);

               if( findIter != std::end( m_groups))
               {
                  (*findIter)->assign( option, std::move( values));
               }
            }

            bool consumed() const override
            {
               return std::all_of(
                     std::begin( m_groups),
                     std::end( m_groups),
                     std::bind( &Base::consumed, std::placeholders::_1));
            }

            void dispatch() const override
            {
               for( auto& base : m_groups)
               {
                  base->dispatch();
               }
            }


         protected:

            void add() {}

            groups_type m_groups;
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
            //
            // Find first argument that has a handler
            //
            auto current = argument::internal::find(
                  std::begin( arguments),
                  std::end( arguments),
                  this);

            //auto start = current;

            while( current != std::end( arguments))
            {

               std::cout << "current " << *current << std::endl;

               //
               // Try to find a handler for this argument
               //
               auto handler = argument::internal::find( *current, m_groups);

               if( handler != std::end( m_groups))
               {
                  //
                  // Find the end of values assosiated with this option
                  //
                  auto currentEnd = argument::internal::find(
                     current + 1,
                     std::end( arguments),
                     this);

                  (*handler)->assign( *current, { current + 1, currentEnd});

                  current = currentEnd;
               }
               else
               {
                  // temp
                  ++current;
               }
            }

            dispatch();

            return true;
         }



      };



   } // utility
} // casual



#endif /* ARGUMENTS_H_ */
