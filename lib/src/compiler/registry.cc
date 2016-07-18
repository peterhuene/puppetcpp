#include <puppet/compiler/registry.hpp>
#include <puppet/compiler/exceptions.hpp>
#include <puppet/compiler/node.hpp>
#include <puppet/compiler/evaluation/functions/call_context.hpp>
#include <puppet/compiler/evaluation/functions.hpp>
#include <puppet/compiler/evaluation/operators.hpp>
#include <puppet/cast.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace puppet::runtime;
using namespace puppet::compiler::evaluation;
using namespace PuppetRubyHost;
using namespace PuppetRubyHost::Protocols;

namespace puppet { namespace compiler {

    klass::klass(string name, ast::class_statement const& statement) :
        _name(rvalue_cast(name)),
        _tree(statement.tree->shared_from_this()),
        _statement(statement)
    {
    }

    string const& klass::name() const
    {
        return _name;
    }

    ast::class_statement const& klass::statement() const
    {
        return _statement;
    }

    defined_type::defined_type(string name, ast::defined_type_statement const& statement) :
        _name(rvalue_cast(name)),
        _tree(statement.tree->shared_from_this()),
        _statement(statement)
    {
    }

    string const& defined_type::name() const
    {
        return _name;
    }

    ast::defined_type_statement const& defined_type::statement() const
    {
        return _statement;
    }

    node_definition::node_definition(ast::node_statement const& statement) :
        _tree(statement.tree->shared_from_this()),
        _statement(statement)
    {
    }

    ast::node_statement const& node_definition::statement() const
    {
        return _statement;
    }

    type_alias::type_alias(ast::type_alias_statement const& statement) :
        _tree(statement.alias.tree->shared_from_this()),
        _statement(statement)
    {
    }

    ast::type_alias_statement const& type_alias::statement() const
    {
        return _statement;
    }

    resource_type::parameter::parameter(string name, bool namevar) :
        _name(rvalue_cast(name)),
        _namevar(namevar)
    {
    }

    resource_type::parameter::parameter(DescribeTypeResponse::Type::Parameter const& parameter) :
        _name(parameter.name()),
        _namevar(parameter.namevar())
    {
        for (auto const& value : parameter.values()) {
            _values.emplace_back(value);
        }
        for (auto const& regex : parameter.regexes()) {
            _regexes.emplace_back(regex);
        }
    }

    string const& resource_type::parameter::name() const
    {
        return _name;
    }

    vector<string> const& resource_type::parameter::values() const
    {
        return _values;
    }

    vector<runtime::values::regex> const& resource_type::parameter::regexes() const
    {
        return _regexes;
    }

    bool resource_type::parameter::namevar() const
    {
        return _namevar;
    }

    void resource_type::parameter::add_value(string value)
    {
        _values.emplace_back(rvalue_cast(value));
    }

    void resource_type::parameter::add_regex(runtime::values::regex regex)
    {
        _regexes.emplace_back(rvalue_cast(regex));
    }

    resource_type::resource_type(string name, string file, size_t line) :
        _name(rvalue_cast(name)),
        _file(rvalue_cast(file)),
        _line(line)
    {
    }

    resource_type::resource_type(DescribeTypeResponse::Type const& type) :
        _name(type.name()),
        _file(type.file()),
        _line(type.line())
    {
        for (auto const& property : type.properties()) {
            _properties.emplace_back(property);
        }
        for (auto const& parameter : type.parameters()) {
            _parameters.emplace_back(parameter);
        }
    }

    string const& resource_type::name() const
    {
        return _name;
    }

    string const& resource_type::file() const
    {
        return _file;
    }

    size_t resource_type::line() const
    {
        return _line;
    }

    vector<resource_type::parameter> const& resource_type::properties() const
    {
        return _properties;
    }

    vector<resource_type::parameter> const& resource_type::parameters() const
    {
        return _parameters;
    }

    void resource_type::add_property(resource_type::parameter p)
    {
        _properties.emplace_back(rvalue_cast(p));
    }

    void resource_type::add_parameter(resource_type::parameter p)
    {
        _parameters.emplace_back(rvalue_cast(p));
    }

    registry::registry(shared_ptr<grpc::ChannelInterface> const& channel)
    {
        if (channel) {
            _type_service = Type::NewStub(channel);
            _function_service = Protocols::Function::NewStub(channel);
        }
    }

    void registry::register_builtins()
    {
        // Register the built-in functions
        register_function(functions::alert::create_descriptor());
        register_function(functions::assert_type::create_descriptor());
        register_function(functions::contain::create_descriptor());
        register_function(functions::crit::create_descriptor());
        register_function(functions::debug::create_descriptor());
        register_function(functions::defined::create_descriptor());
        register_function(functions::each::create_descriptor());
        register_function(functions::emerg::create_descriptor());
        register_function(functions::epp::create_descriptor());
        register_function(functions::err::create_descriptor());
        register_function(functions::fail::create_descriptor());
        register_function(functions::file::create_descriptor());
        register_function(functions::filter::create_descriptor());
        register_function(functions::include::create_descriptor());
        register_function(functions::info::create_descriptor());
        register_function(functions::inline_epp::create_descriptor());
        register_function(functions::new_::create_descriptor());
        register_function(functions::map::create_descriptor());
        register_function(functions::notice::create_descriptor());
        register_function(functions::realize::create_descriptor());
        register_function(functions::reduce::create_descriptor());
        register_function(functions::require::create_descriptor());
        register_function(functions::reverse_each::create_descriptor());
        register_function(functions::split::create_descriptor());
        register_function(functions::step::create_descriptor());
        register_function(functions::tag::create_descriptor());
        register_function(functions::tagged::create_descriptor());
        register_function(functions::type::create_descriptor());
        register_function(functions::versioncmp::create_descriptor());
        register_function(functions::warning::create_descriptor());
        register_function(functions::with::create_descriptor());

        // Register the built-in binary operators
        register_binary_operator(operators::binary::assignment::create_descriptor());
        register_binary_operator(operators::binary::divide::create_descriptor());
        register_binary_operator(operators::binary::equals::create_descriptor());
        register_binary_operator(operators::binary::greater::create_descriptor());
        register_binary_operator(operators::binary::greater_equal::create_descriptor());
        register_binary_operator(operators::binary::in::create_descriptor());
        register_binary_operator(operators::binary::left_shift::create_descriptor());
        register_binary_operator(operators::binary::less::create_descriptor());
        register_binary_operator(operators::binary::less_equal::create_descriptor());
        register_binary_operator(operators::binary::logical_and::create_descriptor());
        register_binary_operator(operators::binary::logical_or::create_descriptor());
        register_binary_operator(operators::binary::match::create_descriptor());
        register_binary_operator(operators::binary::minus::create_descriptor());
        register_binary_operator(operators::binary::modulo::create_descriptor());
        register_binary_operator(operators::binary::multiply::create_descriptor());
        register_binary_operator(operators::binary::not_equals::create_descriptor());
        register_binary_operator(operators::binary::not_match::create_descriptor());
        register_binary_operator(operators::binary::plus::create_descriptor());
        register_binary_operator(operators::binary::right_shift::create_descriptor());

        // Register the built-in unary operators
        register_unary_operator(operators::unary::logical_not::create_descriptor());
        register_unary_operator(operators::unary::negate::create_descriptor());
        register_unary_operator(operators::unary::splat::create_descriptor());
    }

    klass const* registry::find_class(string const& name) const
    {
        auto it = _classes.find(name);
        if (it == _classes.end()) {
            return nullptr;
        }
        return &it->second;
    }

    void registry::register_class(compiler::klass klass)
    {
        auto name = klass.name();
        _classes.emplace(rvalue_cast(name), rvalue_cast(klass));
    }

    defined_type const* registry::find_defined_type(string const& name) const
    {
        auto it = _defined_types.find(name);
        if (it == _defined_types.end()) {
            return nullptr;
        }
        return &it->second;
    }

    void registry::register_defined_type(defined_type type)
    {
        auto name = type.name();
        _defined_types.emplace(rvalue_cast(name), rvalue_cast(type));
    }

    std::pair<node_definition const*, std::string> registry::find_node(compiler::node const& node) const
    {
        // If there are no node definitions, do nothing
        if (_nodes.empty()) {
            return make_pair(nullptr, string());
        }

        // Find a node definition
        string node_name;
        node_definition const* definition = nullptr;
        node.each_name([&](string const& name) {
            // First check by name
            auto it = _named_nodes.find(name);
            if (it != _named_nodes.end()) {
                node_name = it->first;
                definition = &_nodes[it->second];
                return false;
            }
            // Next, check by looking at every regex
            for (auto const& kvp : _regex_nodes) {
                if (kvp.first.search(name)) {
                    node_name = "/" + kvp.first.pattern() + "/";
                    definition = &_nodes[kvp.second];
                    return false;
                }
            }
            return true;
        });

        if (!definition) {
            if (!_default_node_index) {
                return make_pair(nullptr, string());
            }
            node_name = "default";
            definition = &_nodes[*_default_node_index];
        }
        return make_pair(definition, rvalue_cast(node_name));
    }

    node_definition const* registry::find_node(ast::node_statement const& statement) const
    {
        for (auto const& hostname : statement.hostnames) {
            // Check for default node
            if (hostname.is_default()) {
                if (_default_node_index) {
                    return &_nodes[*_default_node_index];
                }
                continue;
            }

            auto name = hostname.to_string();

            // Check for regular expression names
            if (hostname.is_regex()) {
                auto it = find_if(_regex_nodes.begin(), _regex_nodes.end(), [&](std::pair<values::regex, size_t> const& existing) { return existing.first.pattern() == name; });
                if (it != _regex_nodes.end()) {
                    return &_nodes[it->second];
                }
                continue;
            }

            // Otherwise, this is a qualified node name
            auto it = _named_nodes.find(name);
            if (it != _named_nodes.end()) {
                return &_nodes[it->second];
            }
        }
        return nullptr;
    }

    node_definition const* registry::register_node(node_definition node)
    {
        // Check for a node that would conflict with the given one
        if (auto existing = find_node(node.statement())) {
            return existing;
        }

        // Create all the regexes now before modifying any data
        vector<values::regex> regexes;
        for (auto const& hostname : node.statement().hostnames) {
            // Check for regular expressions
            if (!hostname.is_regex()) {
                continue;
            }
            try {
                regexes.emplace_back(hostname.to_string());
            } catch (utility::regex_exception const& ex) {
                throw parse_exception(
                    (boost::format("invalid regular expression: %1%") %
                     ex.what()
                    ).str(),
                    hostname.context().begin,
                    hostname.context().end
                );
            }
        }

        // Add the node
        _nodes.emplace_back(rvalue_cast(node));
        size_t node_index = _nodes.size() - 1;
        for (auto const& hostname : _nodes.back().statement().hostnames) {
            // Skip regexes
            if (hostname.is_regex()) {
                continue;
            }

            // Check for default node
            if (hostname.is_default()) {
                _default_node_index = node_index;
                continue;
            }

            // Add a named node
            _named_nodes.emplace(boost::to_lower_copy(hostname.to_string()), node_index);
        }

        // Populate the regexes
        for (auto& regex : regexes)
        {
            _regex_nodes.emplace_back(rvalue_cast(regex), node_index);
        }
        return nullptr;
    }

    bool registry::has_nodes() const
    {
        return !_nodes.empty();
    }

    void registry::register_type_alias(string name, type_alias alias)
    {
        _aliases.emplace(rvalue_cast(name), rvalue_cast(alias));
    }

    type_alias const* registry::find_type_alias(string const& name) const
    {
        auto it = _aliases.find(name);
        if (it == _aliases.end()) {
            return nullptr;
        }
        return &it->second;
    }

    void registry::register_resource_type(resource_type type)
    {
        string name = type.name();
        _resource_types.emplace(rvalue_cast(name), rvalue_cast(type));
    }

    resource_type const* registry::find_resource_type(string const& name) const
    {
        auto it = _resource_types.find(name);
        if (it == _resource_types.end()) {
            return nullptr;
        }
        return &it->second;
    }

    void registry::normalize(string& name)
    {
        if (boost::starts_with(name, "::")) {
            name = name.substr(2);
        }

        boost::to_lower(name);
    }

    resource_type const* registry::import_ruby_type(string const& environment, string const& name, ast::context const& context)
    {
        // Don't import if there's no service or the resource type already exists
        if (!_type_service || _resource_types.find(name) != _resource_types.end()) {
            return nullptr;
        }

        grpc::ClientContext client_context;

        DescribeTypeRequest request;
        request.set_environment(environment);
        request.set_name(name);

        DescribeTypeResponse response;
        auto status = _type_service->Describe(&client_context, request, &response);
        if (!status.ok()) {
            if (status.error_code() == grpc::UNAVAILABLE) {
                throw evaluation_exception{
                    (boost::format("failed to import resource type '%1%': cannot connect to Ruby host process.") %
                     name
                    ).str(),
                    context
                };
            }
            auto message = status.error_message();
            if (!message.empty()) {
                throw evaluation_exception{
                    (boost::format("failed to import resource type '%1%': %2% (error code %3%).") %
                     name %
                     message %
                     status.error_code()
                    ).str(),
                    context
                };
            }
            throw evaluation_exception{
                (boost::format("failed to import resource type '%1%': RPC error code %2%.") %
                 name %
                 status.error_code()
                ).str(),
                context
            };
        }

        if (!response.has_type() && !response.has_exception()) {
            // Not found
            return nullptr;
        }

        if (response.has_exception()) {
            // Raise an evaluation exception to keep the remote backtrace
            auto const& exception = response.exception();
            vector<stack_frame> backtrace;
            for (auto const& frame : exception.backtrace()) {
                backtrace.emplace_back(frame);
            }
            throw evaluation_exception{
                (boost::format("exception while importing resource type '%1%': %2%") %
                 name %
                 exception.message()
                ).str(),
                context,
                rvalue_cast(backtrace)
            };
        }
        return &_resource_types.emplace(response.type().name(), resource_type{ response.type() }).first->second;
    }

    void registry::register_function(functions::descriptor descriptor)
    {
        if (descriptor.name().empty()) {
            throw runtime_error("cannot register a function with an empty name.");
        }
        if (!descriptor.dispatchable()) {
            throw runtime_error("cannot register a function that is not dispatchable.");
        }
        string name = descriptor.name();
        if (!_functions.emplace(rvalue_cast(name), rvalue_cast(descriptor)).second) {
            throw runtime_error((boost::format("function '%1%' already exists in the registry.") % name).str());
        }
    }

    functions::descriptor const* registry::find_function(std::string const& name) const
    {
        auto it = _functions.find(name);
        if (it == _functions.end()) {
            return nullptr;
        }
        return &it->second;
    }

    functions::descriptor const* registry::import_ruby_function(string const& environment, string const& name, ast::context const& context)
    {
        // Don't import if there's no service or the function already exists
        if (!_function_service || find_function(name)) {
            return nullptr;
        }

        grpc::ClientContext client_context;

        DescribeFunctionRequest request;
        request.set_environment(environment);
        request.set_name(name);

        DescribeFunctionResponse response;
        auto status = _function_service->Describe(&client_context, request, &response);
        if (!status.ok()) {
            if (status.error_code() == grpc::UNAVAILABLE) {
                throw evaluation_exception{
                    (boost::format("failed to import function '%1%': cannot connect to Ruby host process.") %
                     name
                    ).str(),
                    context
                };
            }
            auto message = status.error_message();
            if (!message.empty()) {
                throw evaluation_exception{
                    (boost::format("failed to import function '%1%': %2% (error code %3%).") %
                     name %
                     message %
                     status.error_code()
                    ).str(),
                    context
                };
            }
            throw evaluation_exception{
                (boost::format("failed to import function '%1%': RPC error code %2%.") %
                 name %
                 status.error_code()
                ).str(),
                context
            };
        }

        if (!response.has_function() && !response.has_exception()) {
            // Not found
            return nullptr;
        }

        if (response.has_exception()) {
            // Raise an evaluation exception to keep the remote backtrace
            auto const& exception = response.exception();
            vector<stack_frame> backtrace;
            for (auto const& frame : exception.backtrace()) {
                backtrace.emplace_back(frame);
            }
            throw evaluation_exception{
                (boost::format("exception while importing function '%1%': %2%") %
                 name %
                 exception.message()
                ).str(),
                context,
                rvalue_cast(backtrace)
            };
        }
        return &_functions.emplace(response.function().name(), functions::descriptor{ *_function_service, environment, response.function() }).first->second;
    }

    void registry::register_binary_operator(operators::binary::descriptor descriptor)
    {
        if (!descriptor.dispatchable()) {
            throw runtime_error("cannot register a binary operator that is not dispatchable.");
        }
        if (find_binary_operator(descriptor.oper())) {
            throw runtime_error((boost::format("operator '%1%' already exists in the registry.") % descriptor.oper()).str());
        }
        _binary_operators.emplace_back(rvalue_cast(descriptor));
    }

    operators::binary::descriptor const* registry::find_binary_operator(ast::binary_operator oper) const
    {
        auto it = std::find_if(_binary_operators.begin(), _binary_operators.end(), [=](auto const& descriptor) {
            return descriptor.oper() == oper;
        });
        if (it == _binary_operators.end()) {
            return nullptr;
        }
        return &*it;
    }

    void registry::register_unary_operator(operators::unary::descriptor descriptor)
    {
        if (!descriptor.dispatchable()) {
            throw runtime_error("cannot register a unary operator that is not dispatchable.");
        }
        if (find_unary_operator(descriptor.oper())) {
            throw runtime_error((boost::format("operator '%1%' already exists in the registry.") % descriptor.oper()).str());
        }
        _unary_operators.emplace_back(rvalue_cast(descriptor));
    }

    operators::unary::descriptor const* registry::find_unary_operator(ast::unary_operator oper) const
    {
        auto it = std::find_if(_unary_operators.begin(), _unary_operators.end(), [=](auto const& descriptor) {
            return descriptor.oper() == oper;
        });
        if (it == _unary_operators.end()) {
            return nullptr;
        }
        return &*it;
    }

}}  // namespace puppet::compiler
