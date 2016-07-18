#include <puppet/compiler/evaluation/stack_frame.hpp>

using namespace std;

namespace puppet { namespace compiler { namespace evaluation {

    stack_frame::stack_frame(char const* name, shared_ptr<evaluation::scope> scope, bool external) :
        _name(name),
        _scope(rvalue_cast(scope)),
        _external(external)
    {
        if (!_scope) {
            throw runtime_error("expected a scope for stack frame.");
        }
    }

    stack_frame::stack_frame(expression_type const& expression, shared_ptr<evaluation::scope> scope) :
        _name(expression_name(expression)),
        _scope(rvalue_cast(scope))
    {
        context(expression_context(expression));
    }

    stack_frame::stack_frame(PuppetRubyHost::Protocols::Exception::StackFrame const& frame) :
        _name(frame.name())
    {
        if (!frame.file().empty()) {
            _path = frame.file();
            _line = frame.line();
        }
    }

    char const* stack_frame::name() const
    {
        if (auto ptr = boost::get<string>(&_name)) {
            return ptr->c_str();
        }
        return boost::get<char const*>(_name);
    }

    char const* stack_frame::path() const
    {
        if (auto str = boost::get<string>(&_path)) {
            return str->c_str();
        }
        auto ptr = boost::get<shared_ptr<string>>(_path);
        return ptr ? ptr->c_str() : nullptr;
    }

    size_t stack_frame::line() const
    {
        return _line;
    }

    bool stack_frame::external() const
    {
        return _external;
    }

    shared_ptr<evaluation::scope> const& stack_frame::scope() const
    {
        return _scope;
    }

    void stack_frame::context(ast::context const& context)
    {
        // Update the context for Puppet frames only
        if (_external) {
            return;
        }

        if (context.tree) {
            _path = context.tree->shared_path();
            _line = context.begin.line();
        } else {
            _path = shared_ptr<string>{};
            _line = 0;
        }
    }

    ast::context stack_frame::expression_context(expression_type const& expression)
    {
        struct context_visitor : boost::static_visitor<ast::context>
        {
            result_type operator()(ast::function_statement const* statement) const
            {
                return *statement;
            }

            result_type operator()(ast::class_statement const* statement) const
            {
                return *statement;
            }

            result_type operator()(ast::defined_type_statement const* statement) const
            {
                return *statement;
            }

            result_type operator()(ast::node_statement const* statement) const
            {
                return *statement;
            }

            result_type operator()(ast::collector_expression const* expression) const
            {
                return expression->context();
            }

            result_type operator()(ast::type_alias_statement const* statement) const
            {
                return statement->context();
            }
        };
        return boost::apply_visitor(context_visitor{}, expression);
    }

    boost::variant<char const*, string> stack_frame::expression_name(expression_type const& expression)
    {
        struct name_visitor : boost::static_visitor<boost::variant<char const*, string>>
        {
            result_type operator()(ast::function_statement const* statement) const
            {
                return statement->name.value.c_str();
            }

            result_type operator()(ast::class_statement const* statement) const
            {
                ostringstream ss;
                ss << "<class " << statement->name << ">";
                return ss.str();
            }

            result_type operator()(ast::defined_type_statement const* statement) const
            {
                ostringstream ss;
                ss << "<define " << statement->name << ">";
                return ss.str();
            }

            result_type operator()(ast::node_statement const* statement) const
            {
                return "<node>";
            }

            result_type operator()(ast::collector_expression const* expression) const
            {
                return "<collector>";
            }

            result_type operator()(ast::type_alias_statement const* statement) const
            {
                ostringstream ss;
                ss << "<type alias " << statement->alias << ">";
                return ss.str();
            }
        };
        return boost::apply_visitor(name_visitor{}, expression);
    }

    ostream& operator<<(ostream& os, stack_frame const& frame)
    {
        os << "in '" << frame.name() << '\'';

        auto path = frame.path();
        if (path && *path) {
            os << " at " << path << ":" << frame.line();
        } else {
            os << " (no source)";
        }
        return os;
    }

}}}  // namespace puppet::compiler::evaluation
