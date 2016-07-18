#include <puppet/compiler/scanner.hpp>
#include <puppet/compiler/exceptions.hpp>
#include <puppet/compiler/ast/visitors/definition.hpp>
#include <puppet/cast.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;

namespace puppet { namespace compiler {

    scanner::scanner(logging::logger& logger, string const& environment, compiler::registry& registry) :
        _logger(logger),
        _environment(environment),
        _registry(registry)
    {
    }

    bool scanner::scan(ast::syntax_tree const& tree)
    {
        bool registered = false;
        ast::visitors::definition visitor{
            [&](std::string name, ast::visitors::definition::statement const& definition) {
                if (auto statement = boost::get<ast::class_statement const*>(&definition)) {
                    register_class(rvalue_cast(name), **statement);
                } else if (auto statement = boost::get<ast::defined_type_statement const*>(&definition)) {
                    register_defined_type(rvalue_cast(name), **statement);
                } else if (auto statement = boost::get<ast::node_statement const*>(&definition)) {
                    register_node(**statement);
                }  else if (auto statement = boost::get<ast::function_statement const*>(&definition)) {
                    register_function(**statement);
                } else if (auto statement = boost::get<ast::type_alias_statement const*>(&definition)) {
                    register_type_alias(**statement);
                } else if (auto statement = boost::get<ast::produces_statement const*>(&definition)) {
                    register_produces(**statement);
                } else if (auto statement = boost::get<ast::consumes_statement const*>(&definition)) {
                    register_consumes(**statement);
                } else if (auto statement = boost::get<ast::application_statement const*>(&definition)) {
                    register_application(**statement);
                } else if (auto statement = boost::get<ast::site_statement const*>(&definition)) {
                    register_site(**statement);
                } else {
                    throw runtime_error("unsupported definition statement.");
                }

                registered = true;
            }
        };
        visitor.visit(tree);
        return registered;
    }

    void scanner::register_class(string name, ast::class_statement const& statement)
    {
        registry::normalize(name);

        auto& logger = _logger;
        LOG(debug, "found class '%1%' at %2%:%3%.", name, statement.tree->path(), statement.begin.line());

        if (auto existing = _registry.find_class(name)) {
            throw parse_exception(
                (boost::format("class '%1%' was previously defined at %2%:%3%.") %
                 existing->name() %
                 existing->statement().tree->path() %
                 existing->statement().begin.line()
                ).str(),
                statement.name.begin,
                statement.name.end
            );
        }

        if (auto existing = _registry.find_defined_type(name)) {
            throw parse_exception(
                (boost::format("'%1%' was previously defined as a defined type at %2%:%3%.") %
                 existing->name() %
                 existing->statement().tree->path() %
                 existing->statement().begin.line()
                ).str(),
                statement.name.begin,
                statement.name.end
            );
        }

        check_resource_type(statement.name.value, name, statement.name, "class");

        _registry.register_class(klass{ rvalue_cast(name), statement });
    }

    void scanner::register_defined_type(string name, ast::defined_type_statement const& statement)
    {
        registry::normalize(name);

        auto& logger = _logger;
        LOG(debug, "found defined type '%1%' at %2%:%3%.", name, statement.tree->path(), statement.begin.line());

        if (auto existing = _registry.find_defined_type(name)) {
            throw parse_exception(
                (boost::format("defined type '%1%' was previously defined at %2%:%3%.") %
                 existing->name() %
                 existing->statement().tree->path() %
                 existing->statement().begin.line()
                ).str(),
                statement.name.begin,
                statement.name.end
            );
        }

        if (auto existing = _registry.find_class(name)) {
            throw parse_exception(
                (boost::format("'%1%' was previously defined as a class at %2%:%3%.") %
                 existing->name() %
                 existing->statement().tree->path() %
                 existing->statement().begin.line()
                ).str(),
                statement.name.begin,
                statement.name.end
            );
        }

        check_resource_type(statement.name.value, name, statement.name, "defined type");

        _registry.register_defined_type(defined_type{ rvalue_cast(name), statement });
    }

    void scanner::register_node(ast::node_statement const& statement)
    {
        auto& logger = _logger;
        LOG(debug, "found node definition at %1%:%2%.", statement.tree->path(), statement.begin.line());

        if (auto existing = _registry.find_node(statement)) {
            throw parse_exception(
                (boost::format("a conflicting node definition was previously defined at %1%:%2%.") %
                 existing->statement().tree->path() %
                 existing->statement().begin.line()
                ).str(),
                statement.begin,
                statement.end
            );
        }

        _registry.register_node(node_definition{ statement });
    }

    void scanner::register_function(ast::function_statement const& statement)
    {
        auto& logger = _logger;
        LOG(debug, "found function '%1%' at %2%:%3%.", statement.name, statement.tree->path(), statement.begin.line());

        // Check for an existing function
        auto descriptor = _registry.find_function(statement.name.value);
        if (!descriptor) {
            descriptor = _registry.import_ruby_function(_environment, statement.name.value, statement.name);
        }

        if (descriptor) {
            if (!descriptor->file().empty()) {
                throw parse_exception(
                    (boost::format("cannot define function '%1%' because it conflicts with a previous definition at %2%:%3%.") %
                     statement.name %
                     descriptor->file() %
                     descriptor->line()
                    ).str(),
                    statement.name.begin,
                    statement.name.end
                );
            }
            throw parse_exception(
                (boost::format("cannot define function '%1%' because it conflicts with a built-in function of the same name.") %
                 statement.name
                ).str(),
                statement.name.begin,
                statement.name.end
            );
        }

        _registry.register_function(evaluation::functions::descriptor{ statement.name.value, &statement });
    }

    void scanner::register_type_alias(ast::type_alias_statement const& statement)
    {
        auto& logger = _logger;
        LOG(debug, "found type alias '%1%' at %2%:%3%.", statement.alias, statement.alias.tree->path(), statement.alias.begin.line());

        auto name = statement.alias.name;
        registry::normalize(name);
        auto alias = _registry.find_type_alias(name);
        if (alias) {
            auto context = alias->statement().context();
            throw parse_exception(
                (boost::format("type alias '%1%' was previously defined at %2%:%3%.") %
                 statement.alias %
                 context.tree->path() %
                 context.begin.line()
                ).str(),
                statement.alias.begin,
                statement.alias.end
            );
        }

        if (auto defined_type = _registry.find_defined_type(name)) {
            throw parse_exception(
                (boost::format("type alias '%1%' conflicts with a defined type of the same name defined at %2%:%3%.") %
                 statement.alias %
                 defined_type->statement().tree->path() %
                 defined_type->statement().begin.line()
                ).str(),
                statement.alias.begin,
                statement.alias.end
            );
        }

        check_resource_type(statement.alias.name, name, statement.alias, "type alias");

        _registry.register_type_alias(rvalue_cast(name), type_alias{ statement });
    }

    void scanner::register_produces(ast::produces_statement const& statement)
    {
        // TODO: implement
    }

    void scanner::register_consumes(ast::consumes_statement const& statement)
    {
        // TODO: implement
    }

    void scanner::register_application(ast::application_statement const& statement)
    {
        // TODO: implement
    }

    void scanner::register_site(ast::site_statement const& statement)
    {
        // TODO: implement
    }

    void scanner::check_resource_type(string const& name, string const& normalized_name, ast::context const& context, char const* type)
    {
        auto existing = _registry.find_resource_type(normalized_name);
        if (!existing) {
            existing = _registry.import_ruby_type(_environment, normalized_name, context);
            if (!existing) {
                // Resource type not defined
                return;
            }

            auto& logger = _logger;
            LOG(debug, "imported resource type '%1%' at %2%:%3%.", normalized_name, existing->file(), existing->line());
        }

        if (!existing->file().empty()) {
            throw parse_exception(
                (boost::format("%1% '%2%' conflicts with a resource type of the same name defined at %3%:%4%.") %
                 type %
                 name %
                 existing->file() %
                 existing->line()
                ).str(),
                context.begin,
                context.end
            );
        }
        throw parse_exception(
            (boost::format("%1% '%2%' conflicts with a built-in resource type of the same name.") %
             type %
             name
            ).str(),
            context.begin,
            context.end
        );
    }

}}  // namespace puppet::compiler
