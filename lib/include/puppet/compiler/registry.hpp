/**
 * @file
 * Declares the compiler registry.
 */
#pragma once

#include "resource.hpp"
#include "ast/ast.hpp"
#include "../runtime/values/value.hpp"
#include "evaluation/functions/descriptor.hpp"
#include "evaluation/operators/binary/descriptor.hpp"
#include "evaluation/operators/unary/descriptor.hpp"
#include <type.grpc.pb.h>
#include <function.grpc.pb.h>
#include <boost/optional.hpp>
#include <memory>
#include <vector>
#include <unordered_map>

namespace puppet { namespace compiler {

    namespace evaluation {

        // Forward declaration of context
        struct context;

    }  // namespace puppet::compiler::evaluation

    // Forward declaration of node.
    struct node;

    /**
     * Represents a defined class.
     */
    struct klass
    {
        /**
         * Constructs a class.
         * @param name The fully-qualified name of the class.
         * @param statement The class statement.
         */
        klass(std::string name, ast::class_statement const& statement);

        /**
         * Gets the fully-qualified name of the class.
         * @return Returns the fully-qualified name of the class.
         */
        std::string const& name() const;

        /**
         * Gets the statement that defines the class.
         * @return Returns the statement that defines the class.
         */
        ast::class_statement const& statement() const;

     private:
        std::string _name;
        std::shared_ptr<ast::syntax_tree> _tree;
        ast::class_statement const& _statement;
    };

    /**
     * Represents a defined type.
     */
    struct defined_type
    {
        /**
         * Constructs a defined type.
         * @param name The fully-qualified name of the defined type.
         * @param statement The defined type statement.
         */
        defined_type(std::string name, ast::defined_type_statement const& statement);

        /**
         * Gets the fully-qualified name of the defined type.
         * @return Returns the fully-qualified name of the defined type.
         */
        std::string const& name() const;

        /**
         * Gets the statement that defines the defined type.
         * @return Returns the statement that defines the defined type.
         */
        ast::defined_type_statement const& statement() const;

     private:
        std::string _name;
        std::shared_ptr<ast::syntax_tree> _tree;
        ast::defined_type_statement const& _statement;
    };

    /**
     * Represents a node definition.
     */
    struct node_definition
    {
        /**
         * Constructs a node definition.
         * @param statement The node statement.
         */
        explicit node_definition(ast::node_statement const& statement);

        /**
         * Gets the statement that defines the node.
         * @return Returns the statement that defines the node.
         */
        ast::node_statement const& statement() const;

     private:
        std::shared_ptr<ast::syntax_tree> _tree;
        ast::node_statement const& _statement;
    };

    /**
     * Represents a type alias.
     */
    struct type_alias
    {
        /**
         * Constructs a type alias.
         * @param statement The type alias statement.
         */
        explicit type_alias(ast::type_alias_statement const& statement);

        /**
         * Gets the statement for the type alias.
         * @return Returns the statement for the type alias.
         */
        ast::type_alias_statement const& statement() const;

     private:
        std::shared_ptr<ast::syntax_tree> _tree;
        ast::type_alias_statement const& _statement;
    };

    /**
     * Represents a registered resource type.
     * Not to be confused with the Resource type from the Puppet type system.
     * This is used to represent a registered resource type (such as File, Service, Package, etc).
     */
    struct resource_type
    {
        /**
         * Represents a resource type parameter (or property).
         */
        struct parameter
        {
            /**
             * Constructs a parameter.
             * @param name The parameter name.
             * @param namevar True if the parameter is a namevar or false if it is not.
             */
            explicit parameter(std::string name, bool namevar = false);

            /**
             * Constructs a parameter from a ruby host representation.
             * @param parameter The ruby host representation of the parameter.
             */
            explicit parameter(PuppetRubyHost::Protocols::DescribeTypeResponse::Type::Parameter const& parameter);

            /**
             * Gets the parameter name.
             * @return Returns the parameter name.
             */
            std::string const& name() const;

            /**
             * Gets the parameter's supported string values.
             * @return Returns the parameter's supported string values.
             */
            std::vector<std::string> const& values() const;

            /**
             * Gets the parameter's supported regexes.
             * @return Returns the parameter's supported regexes.
             */
            std::vector<runtime::values::regex> const& regexes() const;

            /**
             * Gets whether or not the parameter is a namevar.
             * @return Returns true if the parameter is a namevar or false if not.
             */
            bool namevar() const;

            /**
             * Adds a string value to the parameter.
             * @param value The string value to add.
             */
            void add_value(std::string value);

            /**
             * Adds a regex to the parameter.
             * @param regex The regex to add.
             */
            void add_regex(runtime::values::regex regex);

         private:
            std::string _name;
            std::vector<std::string> _values;
            std::vector<runtime::values::regex> _regexes;
            bool _namevar = false;
        };

        /**
         * Constructs a resource type.
         * @param name The name of the resource type (e.g. 'Foo').
         * @param file The path of the file declaring the resource type (e.g. '.../lib/puppet/type/foo.rb')
         * @param line The line where the resource was declared.
         */
        explicit resource_type(std::string name, std::string file = {}, size_t line = 0);

        /**
         * Constructs a resource type from a ruby host representation.
         * @param type The ruby host respresentation of the resource type.
         */
        explicit resource_type(PuppetRubyHost::Protocols::DescribeTypeResponse::Type const& type);

        /**
         * Gets the name of the resource type (e.g. 'Foo').
         * @return Returns the name of the resource type.
         */
        std::string const& name() const;

        /**
         * Gets the path of the file declaring the resource type.
         * This may be empty for built-in types.
         * @return Returns the path of the file declaring the resource type.
         */
        std::string const& file() const;

        /**
         * Gets the line where the resource type was declared.
         * This may be 0 for built-in types.
         * @return Returns the line where the resource type was declared.
         */
        size_t line() const;

        /**
         * Gets the properties of the resource type.
         * @return Returns the properties of the resource type.
         */
        std::vector<parameter> const& properties() const;

        /**
         * Gets the parameters of the resource type.
         * @return Returns the parameters of the resource type.
         */
        std::vector<parameter> const& parameters() const;

        /**
         * Adds a property to the resource type.
         * @param p The property to add to the resource type.
         */
        void add_property(parameter p);

        /**
         * Adds a parameter to the resource type.
         * @param p The parameter to add to the resource type.
         */
        void add_parameter(parameter p);

     private:
        std::string _name;
        std::string _file;
        size_t _line = 0;
        std::vector<parameter> _properties;
        std::vector<parameter> _parameters;
    };

    /**
     * Represents the compiler registry.
     * Important: the registry is not thread safe; it is expected that the containing environment handles thread safety.
     */
    struct registry
    {
        /**
         * Default constructor for compiler registry.
         * @param channel The Ruby host RPC channel to use for the registry.
         */
        explicit registry(std::shared_ptr<grpc::ChannelInterface> const& channel);

        /**
         * Default move constructor for registry.
         */
        registry(registry&&) = default;

        /**
         * Default move assignment operator for registry.
         * @return Returns this registry.
         */
        registry& operator=(registry&&) = default;

        /**
         * Registers the built-in Puppet resource types, functions, and operators.
         */
        void register_builtins();

        /**
         * Finds a class given the normalized name.
         * @param name The normalized name of the class to find (e.g. foo::bar).
         * @return Returns a pointer to the class if found or nullptr if the class does not exist.
         */
        klass const* find_class(std::string const& name) const;

        /**
         * Registers a class.
         * @param klass The class to register.
         */
        void register_class(compiler::klass klass);

        /**
         * Finds a defined type given the normalized name.
         * @param name The normalized name of the defined type (e.g. foo::bar).
         * @return Returns a pointer to the defined type or nullptr if the defined type does not exist.
         */
        defined_type const* find_defined_type(std::string const& name) const;

        /**
         * Registers a defined type.
         * @param type The defined type to register.
         */
        void register_defined_type(defined_type type);

        /**
         * Finds a matching node definition and node resource name for the given node.
         * @param node The node to find the node definition for.
         * @return Returns the pair of node definition and resource name if found or a nullptr if one does not exist.
         */
        std::pair<node_definition const*, std::string> find_node(compiler::node const& node) const;

        /**
         * Finds a matching node definition for the given node statement.
         * @param statement The node statement to find a matching node definition for.
         * @return Returns a pointer to the node definition if found or nullptr if one does not exist.
         */
        node_definition const* find_node(ast::node_statement const& statement) const;

        /**
         * Registers a node definition.
         * @param node The node to register.
         * @return Returns nullptr if the node was successfully registered or the pointer to the previous definition.
         */
        node_definition const* register_node(node_definition node);

        /**
         * Determines if the registry has a node definition.
         * @return Returns true if the registry contains any node definitions or false if not.
         */
        bool has_nodes() const;

        /**
         * Registers a type alias.
         * @param name The normalized name of the type alias (e.g. foo::bar).
         * @param alias The type alias to register.
         */
        void register_type_alias(std::string name, type_alias alias);

        /**
         * Finds a type alias by normalized name.
         * @param name The normalized name of the type alias (e.g. foo::bar).
         * @return Returns the type alias or nullptr if the type alias does not exist.
         */
        type_alias const* find_type_alias(std::string const& name) const;

        /**
         * Registers a resource type.
         * @param type The resource type to register.
         */
        void register_resource_type(resource_type type);

        /**
         * Finds a resource type by normalized name.
         * @param name The normalized name of the resource type to find (e.g. foo::bar).
         * @return Returns the resource type or nullptr if not found.
         */
        resource_type const* find_resource_type(std::string const& name) const;

        /**
         * Imports a Ruby resource type into the registry.
         * @param environment The name of the environment containing the Puppet resource type.
         * @param name The name of the resource type to import (e.g. foo::bar).
         * @param context The AST context where the type is being imported.
         * @return Returns a pointer to the resource type or nullptr if the type does not exist.
         */
        resource_type const* import_ruby_type(std::string const& environment, std::string const& name, ast::context const& context);

        /**
         * Registers a function.
         * @param descriptor The descriptor of the function to register.
         */
        void register_function(evaluation::functions::descriptor descriptor);

        /**
         * Finds a function.
         * @param name The function to find.
         * @return Returns the function descriptor if found or nullptr if not found.
         */
        evaluation::functions::descriptor const* find_function(std::string const& name) const;

        /**
         * Imports a Ruby function into the registry.
         * @param environment The name of the environment containing the Puppet function.
         * @param name The name of the function to import.
         * @param context The AST context where the function is being imported.
         * @return Returns the imported function descriptor or nullptr if the function does not exist.
         */
        evaluation::functions::descriptor const* import_ruby_function(std::string const& environment, std::string const& name, ast::context const& context);

        /**
         * Registers a binary operator.
         * @param descriptor The descriptor of the binary operator to register.
         */
        void register_binary_operator(evaluation::operators::binary::descriptor descriptor);

        /**
         * Finds a binary operator descriptor.
         * @param oper The binary operator to find.
         * @return Returns the binary operator descriptor or nullptr if not found.
         */
        evaluation::operators::binary::descriptor const* find_binary_operator(ast::binary_operator oper) const;

        /**
         * Registers a unary operator.
         * @param descriptor The descriptor of the unary operator to register.
         */
        void register_unary_operator(evaluation::operators::unary::descriptor descriptor);

        /**
         * Finds a unary operator descriptor.
         * @param oper The unary operator to find.
         * @return Returns the unary operator descriptor or nullptr if not found.
         */
        evaluation::operators::unary::descriptor const* find_unary_operator(ast::unary_operator oper) const;

        /**
         * Normalizes the given class, defined type, type alias, or resource type name.
         * @param name The name to normalize.
         */
        static void normalize(std::string& name);

     private:
        registry(registry&) = delete;
        registry& operator=(registry&) = delete;

        std::unique_ptr<PuppetRubyHost::Protocols::Type::Stub> _type_service;
        std::unique_ptr<PuppetRubyHost::Protocols::Function::Stub> _function_service;
        std::unordered_map<std::string, klass> _classes;
        std::unordered_map<std::string, defined_type> _defined_types;
        std::vector<node_definition> _nodes;
        std::unordered_map<std::string, size_t> _named_nodes;
        std::vector<std::pair<runtime::values::regex, size_t>> _regex_nodes;
        boost::optional<size_t> _default_node_index;
        std::unordered_map<std::string, type_alias> _aliases;
        std::unordered_map<std::string, resource_type> _resource_types;
        std::unordered_map<std::string, evaluation::functions::descriptor> _functions;
        std::vector<evaluation::operators::binary::descriptor> _binary_operators;
        std::vector<evaluation::operators::unary::descriptor> _unary_operators;
    };

}}  // puppet::compiler
