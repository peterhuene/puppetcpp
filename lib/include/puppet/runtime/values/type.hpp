/**
 * @file
 * Declares the type runtime value.
 */
#pragma once

#include "../values/forward.hpp"
#include "../types/alias.hpp"
#include "../types/any.hpp"
#include "../types/array.hpp"
#include "../types/boolean.hpp"
#include "../types/callable.hpp"
#include "../types/catalog_entry.hpp"
#include "../types/class.hpp"
#include "../types/collection.hpp"
#include "../types/data.hpp"
#include "../types/defaulted.hpp"
#include "../types/enumeration.hpp"
#include "../types/floating.hpp"
#include "../types/hash.hpp"
#include "../types/integer.hpp"
#include "../types/iterable.hpp"
#include "../types/iterator.hpp"
#include "../types/not_undef.hpp"
#include "../types/numeric.hpp"
#include "../types/optional.hpp"
#include "../types/pattern.hpp"
#include "../types/regexp.hpp"
#include "../types/resource.hpp"
#include "../types/runtime.hpp"
#include "../types/scalar.hpp"
#include "../types/string.hpp"
#include "../types/struct.hpp"
#include "../types/tuple.hpp"
#include "../types/type.hpp"
#include "../types/undef.hpp"
#include "../types/variant.hpp"
#include "../../utility/indirect_collection.hpp"
#include "../../cast.hpp"
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <boost/mpl/contains.hpp>
#include <vector>
#include <unordered_set>
#include <exception>

namespace puppet { namespace compiler { namespace evaluation {

    // Forward declare the evaluation context.
    struct context;

}}}  // namespace puppet::compiler::evaluation

namespace puppet { namespace runtime { namespace values {

    /**
     * Exception for type instantiation errors.
     */
    struct instantiation_exception : std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    /**
     * Exception for type conversion errors.
     */
    struct type_conversion_exception : std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    /**
     * Exception for conversion argument errors.
     */
    struct conversion_argument_exception : std::runtime_error
    {
        /**
        * Constructs a new conversion argument exception.
        * @param message The exception message.
        * @param index The index of the argument.
        */
        conversion_argument_exception(std::string const& message, size_t index);

        /**
         * Gets the index of the conversion argument that caused the exception.
         * @return Returns the index of the conversion argument that caused the exception.
         */
        size_t index() const;

     private:
        size_t _index;
    };

    /**
     * The variant representing all possible types.
     * The type value does not derive from this because Boost.Variant does not support variants (like value) containing
     * types derived from boost::variant (ambiguous construction between the type variant and a variant<U...>).
     */
    using type_variant = boost::variant<
        types::any,  // Must be first (default type)
        types::alias,
        types::array,
        types::boolean,
        types::callable,
        types::catalog_entry,
        types::klass,
        types::collection,
        types::data,
        types::defaulted,
        types::enumeration,
        types::floating,
        types::hash,
        types::integer,
        types::iterable,
        types::iterator,
        types::not_undef,
        types::numeric,
        types::optional,
        types::pattern,
        types::regexp,
        types::resource,
        types::runtime,
        types::scalar,
        types::string,
        types::structure,
        types::tuple,
        types::type,
        types::undef,
        types::variant
    >;

    /**
     * Represents the type runtime value.
     */
    struct type
    {
        /**
         * Default constructor for type.
         */
        type() = default;

        /**
         * Constructs a type based on a variant type.
         * @tparam T The variant type to construct with.
         * @param value The value to initialize the variant with.
         */
        template <
            typename T,
            typename = typename std::enable_if<!std::is_same<typename std::remove_reference<T>::type, type>::value>::type
        >
        type(T const& value) :
            _value(value)
        {
        }

        /**
         * Constructs a type based on a variant type.
         * @tparam T The variant type to construct with.
         * @param value The value to initialize the variant with.
         */
        template <
            typename T,
            typename = typename std::enable_if<!std::is_same<typename std::remove_reference<T>::type, type>::value>::type
        >
        type(T&& value) :
            _value(rvalue_cast(value))
        {
        }

        /**
         * Copy assignment operator for type.
         * @tparam T The variant type to assign with.
         * @param value The value to assign.
         * @return Returns this type.
         */
        template <
            typename T,
            typename = typename std::enable_if<!std::is_same<typename std::remove_reference<T>::type, type>::value>::type
        >
        type& operator=(T const& value)
        {
            _value = value;
            return *this;
        }

        /**
         * Move assignment operator for type.
         * @tparam T The variant type to assign with.
         * @param value The value to assign.
         * @return Returns this type.
         */
        template <
            typename T,
            typename = typename std::enable_if<!std::is_same<typename std::remove_reference<T>::type, type>::value>::type
        >
        type& operator=(T&& value)
        {
            _value = rvalue_cast(value);
            return *this;
        }

        /**
         * Applies a visitor to the type variant.
         * @tparam Visitor The visitor type.
         * @param visitor The visitor to apply.
         * @return Returns the result of the visitor.
         */
        template <typename Visitor>
        typename Visitor::result_type apply_visitor(Visitor& visitor)
        {
            return _value.apply_visitor(visitor);
        }

        /**
         * Applies a visitor to the type variant.
         * @tparam Visitor The visitor type.
         * @param visitor The visitor to apply.
         * @return Returns the result of the visitor.
         */
        template <typename Visitor>
        typename Visitor::result_type apply_visitor(Visitor& visitor) const
        {
            return _value.apply_visitor(visitor);
        }

        /**
         * Gets the type variant for the type.
         * @return Returns the type variant for the type.
         */
        type_variant& get();

        /**
         * Gets the type variant for the type.
         * @return Returns the type variant for the type.
         */
        type_variant const& get() const;

        /**
         * Determines if this type is an alias.
         * @return Returns true if the type is an alias or false if not.
         */
        bool is_alias() const;

        /**
         * Deferences the type if an alias or returns this type if not.
         * @return Returns the dereferenced alias or this type if not an alias.
         */
        type const& dereference() const;

        /**
         * Creates a generalized version of the type.
         * A new type is created that does not have type constraints.
         * @return Returns the generalized type.
         */
        type generalize() const;

        /**
         * Determines if the value is an instance of this type.
         * @param value The value to check if being an instance of this type.
         * @param guard The recursion guard to use for aliases.
         * @return Returns true if the value is an instance of this type or false if not.
         */
        bool is_instance(values::value const& value, types::recursion_guard& guard) const;

        /**
         * Determines if the given type is assignable to this type.
         * @param other The other type to check for assignability.
         * @param guard The recursion guard to use for aliases.
         * @return Returns true if the given type is assignable to this type or false if the given type is not assignable to this type.
         */
        bool is_assignable(values::type const& other, types::recursion_guard& guard) const;

        /**
         * Determines if the type is real (i.e. actual type vs. an alias/variant that never resolves to an actual type).
         * @param guard The recursion guard to use for aliases.
         * @return Returns true if the type is real or false if it never resolves to an actual type.
         */
        bool is_real(types::recursion_guard& guard) const;

        /**
         * Checks to see if this type refers to the given type.
         * For aliases, this checks to see if the resolved type refers to the given type.
         * For variants, this checks to see if any member types refer to the given type.
         * For all other types, checks to see if this type is the same instance as the other type.
         * @param other The other type to check.
         * @param guard The recursion guard to use for aliases.
         * @return Returns true if the type refers to the other type or false if it does not.
         */
        bool references(values::type const& other, types::recursion_guard& guard) const;

        /**
         * Writes a representation of the type to the given stream.
         * @param stream The stream to write to.
         * @param expand True to specify that type aliases should be expanded or false if not.
         */
        void write(std::ostream& stream, bool expand = true) const;

        /**
         * Instantiates a new value for the type, performing any required type conversions or formatting.
         * Note: not all types support instantiation.
         * @param from The value to convert from.
         * @param arguments The array of conversion arguments.
         * @param offset The offset in the array where the arguments start; defaults to 0.
         * @return Returns the intantiated value.
         */
        value instantiate(value from, values::array const& arguments = values::array{}, size_t offset = 0) const;

        /**
         * Finds a type in the Puppet type system by normalized name.
         * @name The normalized name of the Puppet type (e.g. 'hash').
         * @return Returns the Puppet type or nullptr if the given name is not a type from the Puppet type system.
         */
        static values::type const* find(std::string const& name);

        /**
         * Creates a type from a postfix expression.
         * @param expression The expression to create the type from.
         * @param context The evaluation context to use to evaluate the expression; if nullptr, an empty evaluation context will be used.
         * @return Returns the type or boost::none if the expression is not a valid type expression.
         */
        static boost::optional<type> create(compiler::ast::postfix_expression const& expression, compiler::evaluation::context* context = nullptr);

        /**
         * Parses a type from a Puppet type expression.
         * @param expression The expression to parse for the type.
         * @param context The evaluation context to use to evaluate the expression; if nullptr, an empty evaluation context will be used.
         * @return Returns the type if the parse was successful or boost::none if the string is not a valid type expression.
         */
        static boost::optional<type> parse(std::string const& expression, compiler::evaluation::context* context = nullptr);

        /**
         * Creates a type from a Puppet type expression.
         * @tparam T The expected Puppet type.
         * @param expression The expression to parse for the type.
         * @return Returns the type if the parse was successful or boost::none if the string is not a valid type expression.
         */
        template <
            typename T,
            typename = typename boost::mpl::contains<type_variant::types, T>::type
        >
        static boost::optional<T> parse_as(std::string const& expression)
        {
            auto result = parse(expression);
            if (!result) {
                return boost::none;
            }
            auto ptr = boost::get<T>(&result->get());
            if (!ptr) {
                return boost::none;
            }
            return rvalue_cast(*ptr);
        }

     private:
        type_variant _value;
    };

    /**
     * Stream insertion operator for runtime type.
     * @param os The output stream to write the runtime type to.
     * @param type The runtime type to write.
     * @return Returns the given output stream.
     */
    std::ostream& operator<<(std::ostream& os, values::type const& type);

    /**
     * Equality operator for type.
     * @param left The left type to compare.
     * @param right The right type to compare.
     * @return Returns true if types are equal or false if not.
     */
    bool operator==(type const& left, type const& right);

    /**
     * Inequality operator for type.
     * @param left The left type to compare.
     * @param right The right type to compare.
     * @return Returns true if the two types are not equal or false if they are equal.
     */
    bool operator!=(type const& left, type const& right);

    /**
     * Hashes the type value.
     * @param type The type value to hash.
     * @return Returns the hash value for the type.
     */
    size_t hash_value(values::type const& type);

    /**
     * Utility class for printing a set of types.
     */
    struct type_set
    {
        /**
         * Adds a type to the set.
         * The types will appear in the order they were added.
         * Duplicate types will be ignored.
         * Note: a pointer to the given type is stored internally; the type is expected to outlive the set.
         * @param type The type to add to the set.
         */
        void add(values::type const& type);

        /**
         * Clears the set.
         */
        void clear();

        /**
         * Determines if the set is empty.
         * @return Returns true if the set is empty or false if not.
         */
        bool empty() const;

        /**
         * Gets the size of the set.
         * @return Returns the size of the set.
         */
        size_t size() const;

        /**
         * Gets the type at the given index.
         * @param index The index to get the type at.
         * @return Returns the type at the given index.
         */
        values::type const& operator[](size_t index) const;

     private:
        std::vector<type const*> _types;
        utility::indirect_set<type> _set;
    };

    /**
     * Stream insertion operator for type set.
     * @param os The output stream to write the set to.
     * @param set The type set to write.
     * @return Returns the given output stream.
     */
    std::ostream& operator<<(std::ostream& os, type_set const& set);

}}}  // puppet::runtime::values

namespace boost {

    /**
     * Gets a value from a type.
     * @tparam T The type to get.
     * @param type The type to get a value from.
     * @return Returns the type or throws boost::bad_get if the type is not of the requested value.
     */
    template <typename T>
    inline T const& get(puppet::runtime::values::type const& type)
    {
        return boost::get<T>(type.dereference().get());
    }

    /**
     * Gets a value from a type.
     * @tparam T The type to get.
     * @param type The type to get a value from.
     * @return Returns the type or nullptr if the type is not of the requested value.
     */
    template <typename T>
    inline T const* get(puppet::runtime::values::type const* type)
    {
        return boost::get<T>(&type->dereference().get());
    }

}  // namespace boost
