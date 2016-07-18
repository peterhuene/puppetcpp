#include <puppet/compiler/evaluation/functions/descriptor.hpp>
#include <puppet/compiler/evaluation/functions/call_context.hpp>
#include <puppet/compiler/evaluation/evaluator.hpp>
#include <puppet/compiler/exceptions.hpp>
#include <boost/format.hpp>
#include <sstream>

using namespace std;
using namespace puppet::runtime;
using namespace PuppetRubyHost;
using namespace PuppetRubyHost::Protocols;

namespace puppet { namespace compiler { namespace evaluation { namespace functions {

    descriptor::descriptor(string name, ast::function_statement const* statement, bool omit_frame) :
        _name(rvalue_cast(name)),
        _statement(statement),
        _omit_frame(omit_frame)
    {
        if (_statement && _statement->tree) {
            _tree = _statement->tree->shared_from_this();
        }
    }

    descriptor::descriptor(Protocols::Function::Stub& service, string const& environment, DescribeFunctionResponse::Function const& function) :
        _name(function.name()),
        _statement(nullptr),
        _file(function.file()),
        _line(function.line()),
        _omit_frame(true)
    {
        // Add the dispatches
        for (auto const& dispatch : function.dispatches()) {
            string id = dispatch.id();

            if (dispatch.types_size() != dispatch.names_size()) {
                throw compilation_exception(
                    (boost::format("unexpected mismatch between count of types and names when describing function '%1%'.") %
                     function.name()
                    ).str(),
                    function.file()
                );
            }

            types::callable signature;
            vector<unique_ptr<values::type>> types;
            unique_ptr<values::type> block;

            // Parse the parameter types
            auto types_count = dispatch.types_size();
            for (int i = 0; i < types_count; ++i) {
                auto& type = dispatch.types(i);
                auto& name = dispatch.names(i);

                auto parsed = values::type::parse(type);
                if (!parsed) {
                    throw compilation_exception(
                        (boost::format("parameter '%1%' for dispatch '%2%' has invalid type '%3%'.") %
                         name %
                         dispatch.name() %
                         type
                        ).str(),
                        function.file()
                    );
                }
                types.emplace_back(make_unique<values::type>(rvalue_cast(*parsed)));
            }

            // Parse the block type
            auto& block_type = dispatch.block_type();
            if (!block_type.empty()) {
                auto parsed = values::type::parse(block_type);
                if (!parsed) {
                    throw compilation_exception(
                        (boost::format("block parameter '%1%' for dispatch '%2%' has invalid type '%3%'.") %
                         dispatch.block_name() %
                         dispatch.name() %
                         block_type
                        ).str(),
                        function.file()
                    );
                }
                block = make_unique<values::type>(rvalue_cast(*parsed));
            }

            // Add the dispatch
            add(types::callable{
                    rvalue_cast(types),
                    dispatch.min(),
                    dispatch.max() < 0 ? numeric_limits<int64_t>::max() : dispatch.max(),
                    rvalue_cast(block)
                },
                [&, environment, id = rvalue_cast(id)](call_context& context) {
                    return descriptor::dispatch(service, environment, id, context);
                }
            );
        }

        if (!dispatchable()) {
            throw compilation_exception(
                (boost::format("cannot import function '%1%' because there are no available dispatches.") %
                 function.name()
                ).str(),
                function.file()
            );
        }
    }

    string const& descriptor::name() const
    {
        return _name;
    }

    string const& descriptor::file() const
    {
        if (_statement) {
            return _statement->tree->path();
        }
        return _file;
    }

    size_t descriptor::line() const
    {
        if (_statement) {
            return _statement->begin.line();
        }
        return _line;
    }

    ast::function_statement const* descriptor::statement() const
    {
        return _statement;
    }

    bool descriptor::dispatchable() const
    {
        return _statement || !_dispatch_descriptors.empty();
    }

    void descriptor::add(string const& signature, callback_type callback)
    {
        auto callable = values::type::parse_as<types::callable>(signature);
        if (!callable) {
            throw runtime_error((boost::format("function '%1%' cannot add a dispatch with invalid signature '%2%'.") % _name % signature).str());
        }

        add(rvalue_cast(*callable), rvalue_cast(callback));
    }

    void descriptor::add(types::callable signature, callback_type callback)
    {
        dispatch_descriptor descriptor;
        descriptor.signature = rvalue_cast(signature);
        descriptor.callback = rvalue_cast(callback);
        _dispatch_descriptors.emplace_back(rvalue_cast(descriptor));
    }

    values::value descriptor::dispatch(call_context& context) const
    {
        auto& evaluation_context = context.context();

        // Handle functions written in Puppet
        if (_statement) {
            // Ensure the caller is allowed to call this function if it is private
            if (_statement->is_private && _statement->tree && context.name().tree &&
                _statement->tree->module() != context.name().tree->module()) {
                if (!_statement->tree->module()) {
                    throw evaluation_exception(
                        (boost::format("function '%1%' (declared at %2%:%3%) is private to the environment.") %
                         context.name() %
                         _statement->tree->path() %
                         _statement->begin.line()
                        ).str(),
                        context.name(),
                        evaluation_context.backtrace()
                    );
                }
                throw evaluation_exception(
                    (boost::format("function '%1%' (declared at %2%:%3%) is private to module '%4%'.") %
                     context.name() %
                     _statement->tree->path() %
                     _statement->begin.line() %
                     _statement->tree->module()->name()
                    ).str(),
                    context.name(),
                    evaluation_context.backtrace()
                );
            }

            try {
                function_evaluator evaluator{ evaluation_context, *_statement };
                return evaluator.evaluate(context.arguments(), nullptr, context.name(), false);
            } catch (argument_exception const& ex) {
                throw evaluation_exception(ex.what(), context.argument_context(ex.index()), evaluation_context.backtrace());
            }
        }

        // Search for a dispatch descriptor with a matching signature
        // TODO: in the future, this should dispatch to the most specific overload rather than the first dispatchable overload
        for (auto& descriptor : _dispatch_descriptors) {
            if (descriptor.signature.can_dispatch(context)) {
                if (_omit_frame) {
                    // Don't push a new stack frame
                    return descriptor.callback(context);
                }
                scoped_stack_frame frame{
                    evaluation_context,
                    stack_frame{
                        _name.c_str(),
                        make_shared<evaluation::scope>(evaluation_context.top_scope())
                    }
                };
                return descriptor.callback(context);
            }
        }

        // Find the reason the call could not be dispatched
        auto invocable = check_argument_count(context);
        check_block_parameters(context, invocable);
        check_parameter_types(context, invocable);

        // Generic error in case the above fails
        throw evaluation_exception(
            (boost::format("function '%1%' cannot be dispatched.") %
             _name
            ).str(),
            context.name(),
            evaluation_context.backtrace()
        );
    }

    vector<descriptor::dispatch_descriptor const*> descriptor::check_argument_count(call_context const& context) const
    {
        auto& evaluation_context = context.context();

        // The call could not be dispatched, determine the reason
        auto argument_count = static_cast<int64_t>(context.arguments().size());
        bool block_passed = static_cast<bool>(context.block());
        int64_t min_arguments = -1;
        int64_t max_arguments = -1;

        // Get the argument counts, block parameter counts, and the set of descriptors that could be invoked
        vector<dispatch_descriptor const*> invocable;
        for (auto& descriptor : _dispatch_descriptors) {

            // Find the minimum number of arguments required for any overload
            if (min_arguments < 0 || min_arguments > descriptor.signature.min()) {
                min_arguments = descriptor.signature.min();
            }
            // Find the maximum number of arguments required for any overload
            if (max_arguments < 0 || max_arguments < descriptor.signature.max()) {
                max_arguments = descriptor.signature.max();
            }

            // Ignore overloads that aren't a match for number of arguments given
            if (argument_count < descriptor.signature.min() || argument_count > descriptor.signature.max()) {
                continue;
            }

            // Ignore overloads with a block mismatch
            types::callable const* block;
            bool required = false;
            tie(block, required) = descriptor.signature.block();
            if ((!block && block_passed) || (block && required && !block_passed)) {
                continue;
            }

            invocable.emplace_back(&descriptor);
        }
        // Check for argument count mismatch
        if (argument_count != min_arguments && min_arguments == max_arguments) {
            throw evaluation_exception(
                (boost::format("function '%1%' expects %2% %3% but was given %4%.") %
                 _name %
                 min_arguments %
                 (min_arguments == 1 ? "argument" : "arguments") %
                 argument_count
                ).str(),
                (argument_count == 0 || argument_count < min_arguments) ? context.name() : context.argument_context(argument_count - 1),
                evaluation_context.backtrace()
            );
        }
        if (argument_count < min_arguments) {
            throw evaluation_exception(
                (boost::format("function '%1%' expects at least %2% %3% but was given %4%.") %
                 _name %
                 min_arguments %
                 (min_arguments == 1 ? "argument" : "arguments") %
                 argument_count
                ).str(),
                context.name(),
                evaluation_context.backtrace()
            );
        }
        if (argument_count > max_arguments) {
            throw evaluation_exception(
                (boost::format("function '%1%' expects at most %2% %3% but was given %4%.") %
                 _name %
                 max_arguments %
                 (max_arguments == 1 ? "argument" : "arguments") %
                 argument_count
                ).str(),
                argument_count == 0 ? context.name() : context.argument_context(argument_count - 1),
                evaluation_context.backtrace()
            );
        }
        return invocable;
    }

    void descriptor::check_block_parameters(call_context const& context, vector<dispatch_descriptor const*> const& invocable) const
    {
        auto& evaluation_context = context.context();
        auto block = context.block();

        // If the invocable set is empty, then there was a block mismatch
        if (invocable.empty()) {
            if (block) {
                throw evaluation_exception(
                    (boost::format("function '%1%' does not accept a block.") %
                     _name
                    ).str(),
                    *block,
                    evaluation_context.backtrace()
                );
            }
            throw evaluation_exception(
                (boost::format("function '%1%' requires a block to be passed.") %
                 _name
                ).str(),
                context.name(),
                evaluation_context.backtrace()
            );
        }

        // If there's no block, nothing to validate
        if (!block) {
            return;
        }

        // Find block parameter count mismatch
        int64_t block_parameter_count = static_cast<int64_t>(block->parameters.size());
        int64_t min_block_parameters = -1;
        int64_t max_block_parameters = -1;
        for (auto descriptor : invocable) {
            // Ignore overloads with a block mismatch
            types::callable const* block_signature;
            bool required = false;
            tie(block_signature, required) = descriptor->signature.block();
            if (!block_signature) {
                continue;
            }

            // Find the minimum number of block parameters for any invocable overload
            if (min_block_parameters < 0 || min_block_parameters > block_signature->min()) {
                min_block_parameters = block_signature->min();
            }
            // Find the minimum number of block parameters for any invocable overload
            if (max_block_parameters < 0 || max_block_parameters < block_signature->max()) {
                max_block_parameters = block_signature->max();
            }
        }

        // Check for parameter count mismatch
        if (block_parameter_count != min_block_parameters && min_block_parameters == max_block_parameters) {
            throw evaluation_exception(
                (boost::format("function '%1%' expects %2% block %3% but was given %4%.") %
                 _name %
                 min_block_parameters %
                 (min_block_parameters == 1 ? "parameter" : "parameters") %
                 block_parameter_count
                ).str(),
                (block_parameter_count == 0 || block_parameter_count < min_block_parameters) ?
                static_cast<ast::context>(*block) :
                block->parameters[block_parameter_count - 1].context(),
                evaluation_context.backtrace()
            );
        }
        if (block_parameter_count < min_block_parameters) {
            throw evaluation_exception(
                (boost::format("function '%1%' expects at least %2% block %3% but was given %4%.") %
                 _name %
                 min_block_parameters %
                 (min_block_parameters == 1 ? "parameter" : "parameters") %
                 block_parameter_count
                ).str(),
                *block,
                evaluation_context.backtrace()
            );
        }
        if (block_parameter_count > max_block_parameters) {
            throw evaluation_exception(
                (boost::format("function '%1%' expects at least %2% block %3% but was given %4%.") %
                 _name %
                 max_block_parameters %
                 (max_block_parameters == 1 ? "parameter" : "parameters") %
                 block_parameter_count
                ).str(),
                block_parameter_count == 0 ?
                static_cast<ast::context>(*block) :
                block->parameters[block_parameter_count - 1].context(),
                evaluation_context.backtrace()
            );
        }
    }

    void descriptor::check_parameter_types(call_context const& context, vector<dispatch_descriptor const*> const& invocable) const
    {
        auto& evaluation_context = context.context();

        // Determine the first (lowest index) argument with a type mismatch
        int64_t min_argument_mismatch = -1;
        for (auto descriptor : invocable) {
            auto index = descriptor->signature.find_mismatch(context.arguments());
            if (min_argument_mismatch < 0 || min_argument_mismatch > index) {
                min_argument_mismatch = index;
            }
        }

        // Number of arguments and block parameters is correct; the problem lies with one of the argument's type
        // Build a set of expected types
        values::type_set set;
        for (auto& descriptor : invocable) {
            auto type = descriptor->signature.parameter_type(min_argument_mismatch);
            if (!type) {
                continue;
            }
            set.add(*type);
        }
        if (set.empty()) {
            return;
        }
        throw evaluation_exception(
            (boost::format("function '%1%' expects %2% but was given %3%.") %
             _name %
             set %
             context.argument(min_argument_mismatch).infer_type()
            ).str(),
            context.argument_context(min_argument_mismatch),
            evaluation_context.backtrace()
        );
    }

    values::value descriptor::dispatch(Protocols::Function::Stub& service, string const& environment, string const& id, call_context& context)
    {
        grpc::ClientContext client_context;

        InvokeFunctionRequest request;
        auto call = request.mutable_call();
        call->set_environment(environment);
        call->set_id(id);
        call->set_has_block(static_cast<bool>(context.block()));

        for (auto const& argument : context.arguments()) {
            auto call_argument = call->add_arguments();
            argument->to_protocol_value(*call_argument);
        }

        auto stream = service.Invoke(&client_context);
        if (!stream->Write(request)) {
            throw evaluation_exception(
                "connection lost to Ruby host process.",
                ast::context{},
                context.context().backtrace()
            );
        }

        boost::optional<values::value> result;
        InvokeFunctionResponse response;
        while (stream->Read(&response)) {
            // Check for a result
            if (response.has_result()) {
                result = values::value{ response.result() };
                break;
            }
            // Check for an exception
            if (response.has_exception()) {
                // Raise an evaluation exception with a merged trace
                auto const& exception = response.exception();
                vector<stack_frame> backtrace;
                for (auto const& frame : exception.backtrace()) {
                    backtrace.emplace_back(frame);
                }
                context.context().append_backtrace(backtrace);

                shared_ptr<ast::syntax_tree> tree;
                ast::context ast_context = context.name();
                if (exception.has_context()) {
                    auto const& remote_context = exception.context();

                    tree = ast::syntax_tree::create(remote_context.file());
                    ast_context.tree = tree.get();

                    auto& begin = remote_context.begin();
                    ast_context.begin = lexer::position{ static_cast<size_t>(begin.offset()), static_cast<size_t>(begin.line()) };
                    auto& end = remote_context.end();
                    ast_context.end = lexer::position{ static_cast<size_t>(end.offset()), static_cast<size_t>(end.line()) };
                }
                throw evaluation_exception(exception.message(), ast_context, rvalue_cast(backtrace));
            }
            // Check for a yield
            if (response.has_yield()) {
                size_t stack_depth = context.context().call_stack_size();

                auto& yield = response.yield();

                // Perform the yield and respond with a continuation request
                InvokeFunctionRequest yield_request;
                auto continuation = yield_request.mutable_continuation();

                try {
                    // Convert the yield arguments
                    values::array yield_arguments;
                    for (auto const& argument : yield.arguments()) {
                        yield_arguments.emplace_back(argument);
                    }

                    // Perform the yield
                    context.yield(yield_arguments).to_protocol_value(*continuation->mutable_result());
                } catch (evaluation_exception const& ex) {
                    auto exception = continuation->mutable_exception();
                    exception->set_message(ex.what());

                    // Copy the relevant frames to the exception response
                    size_t count = ex.backtrace().size() > stack_depth ? ex.backtrace().size() - stack_depth : 0;
                    for (size_t i = 0; i < count; ++i) {
                        auto current = ex.backtrace()[i];
                        auto frame = exception->add_backtrace();
                        frame->set_name(current.name());
                        if (current.path()) {
                            frame->set_file(current.path());
                            frame->set_line(current.line());
                        }
                    }

                    // Emit context when we have source file information
                    auto& ast_context = ex.context();
                    if (ast_context.tree && ast_context.tree->source().empty()) {
                        auto context = exception->mutable_context();
                        context->set_file(ast_context.tree->path());
                        auto begin = context->mutable_begin();
                        begin->set_line(ast_context.begin.line());
                        begin->set_offset(ast_context.begin.offset());
                        auto end = context->mutable_end();
                        end->set_line(ast_context.end.line());
                        end->set_offset(ast_context.end.offset());
                    }
                } catch (exception const& ex) {
                    auto exception = continuation->mutable_exception();
                    exception->set_message(ex.what());
                    if (context.context().call_stack_size() > stack_depth) {
                        auto backtrace = context.context().backtrace(context.context().call_stack_size() - stack_depth);
                        for (auto const& frame : backtrace) {
                            auto f = exception->add_backtrace();
                            f->set_name(frame.name());
                            if (frame.path()) {
                                f->set_file(frame.path());
                                f->set_line(frame.line());
                            }
                        }
                    }
                }
                if (!stream->Write(yield_request)) {
                    throw evaluation_exception(
                        "connection lost to Ruby host process.",
                        context.name(),
                        context.context().backtrace()
                    );
                }
                continue;
            }
            break;
        }

        // Finish the request
        stream->WritesDone();
        auto status = stream->Finish();
        if (!status.ok()) {
            if (status.error_code() == grpc::UNAVAILABLE) {
                throw evaluation_exception(
                    "cannot connect to Ruby host process.",
                    context.name(),
                    context.context().backtrace()
                );
            }
            throw evaluation_exception(
                (boost::format("failed to invoke Puppet function: RPC error code %1%.") %
                 status.error_code()
                ).str(),
                context.name(),
                context.context().backtrace()
            );
        }
        if (!result) {
            throw evaluation_exception(
                "unexpected response from server when invoking a function.",
                context.name(),
                context.context().backtrace()
            );
        }
        return rvalue_cast(*result);
    }

}}}}  // namespace puppet::compiler::evaluation::functions
