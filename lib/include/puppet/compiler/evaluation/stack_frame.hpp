/**
 * @file
 * Declares the evaluation stack frame.
 */
#pragma once

#include "../ast/ast.hpp"
#include <exception.pb.h>
#include <boost/variant.hpp>
#include <memory>

namespace puppet { namespace compiler { namespace evaluation {

    // Forward declaration of scope
    struct scope;

    /**
     * Represents a Puppet stack frame.
     */
    struct stack_frame
    {
        /**
         * Represents the different types of Puppet statements/expressions that can be on the call stack.
         */
        using expression_type = boost::variant<
            ast::function_statement const*,
            ast::class_statement const*,
            ast::defined_type_statement const*,
            ast::node_statement const*,
            ast::collector_expression const*,
            ast::type_alias_statement const*
        >;

        /**
         * Constructs a stack frame for a native function.
         * @param name The name of the function.
         * @param scope The associated scope.
         * @param external True if the frame is external (not Puppet) or false if the frame is Puppet.
         */
        stack_frame(char const* name, std::shared_ptr<evaluation::scope> scope, bool external = true);

        /**
         * Constructs a stack frame for the given expression.
         * @param expression The expression associated with the stack frame.
         * @param scope The associated scope.
         */
        stack_frame(expression_type const& expression, std::shared_ptr<evaluation::scope> scope);

        /**
         * Constructs a stack frame from a ruby host representation.
         * @param frame The ruby host respresentation of the stack frame.
         */
        explicit stack_frame(PuppetRubyHost::Protocols::Exception::StackFrame const& frame);

        /**
         * Gets the name of frame.
         * @return Returns the name of the frame.
         */
        char const* name() const;

        /**
         * Gets the path of the source for the frame.
         * @return Returns the path of the frame or nullptr if there is no source.
         */
        char const* path() const;

        /**
         * Gets the line number for the frame.
         * @return Returns the line number for the frame.
         */
        size_t line() const;

        /**
         * Gets whether or not the frame is external (not Puppet).
         * @return Returns true if the frame is external (not Puppet) or false if the frame represents Puppet code.
         */
        bool external() const;

        /**
         * Gets the scope of the stack frame.
         * @return Returns the scope of the stack frame.
         */
        std::shared_ptr<evaluation::scope> const& scope() const;

        /**
         * Sets the current AST context (i.e. context of currently evaluating expression) for the frame.
         * @param context The new AST context.
         */
        void context(ast::context const& context);

     private:
        static ast::context expression_context(expression_type const& expression);
        static boost::variant<char const*, std::string> expression_name(expression_type const& expression);

        boost::variant<char const*, std::string> _name;
        boost::variant<std::shared_ptr<std::string>, std::string> _path;
        size_t _line = 0;
        std::shared_ptr<evaluation::scope> _scope;
        bool _external = false;
    };

    /**
     * Stream insertion operator for stack frame.
     * @param os The stream to write the stack frame to.
     * @param frame The frame to write.
     * @return Returns the given stream.
     */
    std::ostream& operator<<(std::ostream& os, stack_frame const& frame);

}}}  // namespace puppet::compiler::evaluation
