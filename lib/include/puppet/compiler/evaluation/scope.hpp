/**
 * @file
 * Declares the evaluation scope.
 */
#pragma once

#include "../resource.hpp"
#include "../ast/ast.hpp"
#include "../../runtime/values/value.hpp"
#include "../../facts/provider.hpp"
#include <unordered_map>
#include <string>
#include <memory>
#include <cstdint>
#include <functional>

namespace puppet { namespace compiler { namespace evaluation {

    // Forward declaration of context
    struct context;

    /**
     * Represents context about a variable assignment.
     */
    struct assignment_context
    {
        /**
         * Constructs an assignment context given an AST context.
         * @param context The AST context for the variable assignment.
         */
        explicit assignment_context(ast::context const* context);

        /**
         * Gets the path where the variable was assigned.
         * @return Returns the path where the variable was assigned.
         */
        std::shared_ptr<std::string> const& path() const;

        /**
         * Gets the line where the variable was assigned.
         * @return Returns the line where the variable was assigned.
         */
        size_t line() const;

     private:
        std::shared_ptr<std::string> _path;
        size_t _line;
    };

    /**
     * Represents an evaluation scope.
     */
    struct scope
    {
        /**
         * Constructs a scope.
         * @param parent The parent scope.
         * @param resource The resource associated with the scope.
         */
        explicit scope(std::shared_ptr<scope> parent, compiler::resource* resource = nullptr);

        /**
         * Constructs the top scope.
         * @param facts The facts provider to use for the top scope.
         */
        explicit scope(std::shared_ptr<facts::provider> facts);

        /**
         * Gets the parent scope.
         * @return Returns the parent scope or nullptr if at top scope.
         */
        std::shared_ptr<scope> const& parent() const;

        /**
         * Gets the resource associated with the scope.
         * @return Returns the resource associated with the scope or nullptr if there is no associated resource.
         */
        compiler::resource* resource();

        /**
         * Gets the resource associated with the scope.
         * @return Returns the resource associated with the scope or nullptr if there is no associated resource.
         */
        compiler::resource const* resource() const;

        /**
         * Sets the resource associated with the scope.
         * @param resource The new resource to associate with the scope.
         */
        void resource(compiler::resource* resource);

        /**
         * Qualifies the given name using the scope's name.
         * @param name The name to qualify.
         * @return Returns the fully-qualified name.
         */
        std::string qualify(std::string const& name) const;

        /**
         * Sets a variable in the scope.
         * @param name The name of the variable (e.g. 'foo').
         * @param value The value of the variable.
         * @param context The context of where the variable was assigned.
         * @return Returns the previous assignment context if the variable was already set or returns nullptr if the variable was successfully set.
         */
        assignment_context const* set(std::string name, std::shared_ptr<runtime::values::value const> value, ast::context const& context);

        /**
         * Gets a variable in the scope.
         * @param name The name of the variable to get.
         * @return Returns the assigned variable or nullptr if the variable does not exist in the scope.
         */
        std::shared_ptr<runtime::values::value const> get(std::string const& name);

        /**
         * Adds resource attribute defaults to the scope.
         * @param context The current evaluation context.
         * @param type The resource type for the defaults.
         * @param attributes The attributes to use as defaults.
         */
        void add_defaults(evaluation::context& context, runtime::types::resource const& type, compiler::attributes attributes);

        /**
         * Finds a default attribute for the given resource type and attribute name.
         * @param type The resource type to find the default attribute for.
         * @param name The attribute name.
         * @return Returns the attribute or nullptr if there is no default defined.
         */
        std::shared_ptr<attribute> find_default(runtime::types::resource const& type, std::string const& name) const;

        /**
         * Enumerates the default attributes in this scope and the parent scopes.
         * @param type The resource type to find the default attributes for.
         * @param set The attribute set to use to keep track of already seen attributes.
         * @param callback The callback to call for each attribute.
         */
        void each_default(runtime::types::resource const& type, attribute_set& set, std::function<bool(attribute const&)> const& callback) const;

     private:
        std::shared_ptr<facts::provider> _facts;
        std::shared_ptr<scope> _parent;
        compiler::resource* _resource;
        std::unordered_map<std::string, std::pair<std::shared_ptr<runtime::values::value const>, assignment_context>> _variables;
        std::unordered_map<std::string, attributes> _defaults;
    };

    /**
     * Stream insertion operator for evaluatino scope.
     * @param os The output stream to write to.
     * @param scope The scope to write.
     * @return Returns the given output stream.
     */
    std::ostream& operator<<(std::ostream& os, evaluation::scope const& scope);

}}}  // puppet::compiler::evaluation
