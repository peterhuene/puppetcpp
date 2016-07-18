/**
 * @file
 * Declares the function descriptor.
 */
#pragma once

#include "../../ast/ast.hpp"
#include "../../../runtime/values/value.hpp"
#include <function.grpc.pb.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace puppet { namespace compiler { namespace evaluation { namespace functions {

    // Forward declaration of function call_context.
    struct call_context;

    /**
     * Responsible for describing a Puppet function.
     */
    struct descriptor
    {
        /**
         * The callback type to call when the function call is dispatched.
         */
        using callback_type = std::function<runtime::values::value(call_context&)>;

        /**
         * Constructs a function descriptor.
         * @param name The name of the function.
         * @param statement The function statement if the function was defined in Puppet source code.
         * @param omit_frame True to omit a stack frame for the function call or false to include a stack frame.
         */
        explicit descriptor(std::string name, ast::function_statement const* statement = nullptr, bool omit_frame = false);

        /**
         * Constructs a function descriptor from a Ruby host representation.
         * @param service The function service to use when dispatching a function call.
         * @param environment The name of the environment where the function is defined.
         * @param function The Ruby host representation of the function.
         */
        descriptor(PuppetRubyHost::Protocols::Function::Stub& service, std::string const& environment, PuppetRubyHost::Protocols::DescribeFunctionResponse::Function const& function);

        /**
         * Gets the function's name.
         * @return Returns the function's name.
         */
        std::string const& name() const;

        /**
         * Gets the path of the file defining the function.
         * @return Returns the path of the file defining the function or empty if the function is a built-in.
         */
        std::string const& file() const;

        /**
         * Gets the line in the file where the function was defined.
         * @return Returns the line in the file or 0 if the function is a built-in.
         */
        size_t line() const;

        /**
         * Gets the associated function statement if the function was defined in Puppet source code.
         * @return Returns the function statement or nullptr if the function is not defined in Puppet code.
         */
        ast::function_statement const* statement() const;

        /**
         * Determines if the function has dispatch descriptors.
         * @return Returns true if the function has dispatch descriptors or false if not.
         */
        bool dispatchable() const;

        /**
         * Adds a dispatch descriptor for the function.
         * @param signature The signature for function call dispatch.
         * @param callback The callback to invoke when the function call is dispatched.
         */
        void add(std::string const& signature, callback_type callback);

        /**
         * Adds a dispatch descriptor for the function.
         * @param signature The signature for function call dispatch.
         * @param callback The callback to invoke when the function call is dispatched.
         */
        void add(runtime::types::callable signature, callback_type callback);

        /**
         * Dispatches a function call to the matching dispatch descriptor.
         * @param context The call context to dispatch.
         * @return Returns the result of the function call.
         */
        runtime::values::value dispatch(call_context& context) const;

     private:
        struct dispatch_descriptor
        {
            runtime::types::callable signature;
            callback_type callback;
        };

        std::vector<dispatch_descriptor const*> check_argument_count(call_context const& context) const;
        void check_block_parameters(call_context const& context, std::vector<dispatch_descriptor const*> const& invocable) const;
        void check_parameter_types(call_context const& context, std::vector<dispatch_descriptor const*> const& invocable) const;
        static runtime::values::value dispatch(PuppetRubyHost::Protocols::Function::Stub& service, std::string const& environment, std::string const& id, call_context& context);

        std::string _name;
        std::vector<dispatch_descriptor> _dispatch_descriptors;
        std::shared_ptr<ast::syntax_tree> _tree;
        ast::function_statement const* _statement;
        std::string _file;
        size_t _line = 0;
        bool _omit_frame = false;
    };

}}}}  // puppet::compiler::evaluation::functions
