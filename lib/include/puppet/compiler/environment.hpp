/**
 * @file
 * Declares the compilation environment.
 */
#pragma once

#include "ast/ast.hpp"
#include "registry.hpp"
#include "module.hpp"
#include "finder.hpp"
#include "settings.hpp"
#include "../logging/logger.hpp"
#include <string>
#include <vector>
#include <deque>
#include <memory>
#include <unordered_map>
#include <functional>
#include <mutex>

namespace grpc {
    // Forward declaration of ChannelInterface
    class ChannelInterface;
}  // namespace grpc

namespace puppet { namespace compiler {

    namespace evaluation {

        // Forward declaration of context.
        struct context;

    }  // namespace puppet::compiler::evaluation

    /**
     * Represents a compilation environment.
     */
    struct environment : finder
    {
        /**
         * Creates a new environment given the compiler settings.
         * @param logger The logger to use for logging messages.
         * @param settings The settings to use for the environment.
         * @param channel The Ruby host RPC channel to use for the environment.
         * @return Returns the new environment.
         */
        static std::shared_ptr<environment> create(logging::logger& logger, compiler::settings settings, std::shared_ptr<grpc::ChannelInterface> channel = nullptr);

        /**
         * Gets the name of the environment.
         * @return Returns the name of the environment.
         */
        std::string const& name() const;

        /**
         * Gets the compiler settings for the environment.
         * @return Returns the compiler settings for the environment.
         */
        compiler::settings const& settings() const;

        /**
         * Gets the environment's modules.
         * @return Returns the environment's modules.
         */
        std::deque<module> const& modules() const;

        /**
         * Finds a module by name.
         * @param name The module name to find.
         * @return Returns a pointer to the module or nullptr if the module does not exist.
         */
        module const* find_module(std::string const& name) const;

        /**
         * Enumerates the modules in the environment.
         * @param callback The callback to invoke for each module in the environment.
         */
        void each_module(std::function<bool(module const&)> const& callback) const;

        /**
         * Registers the built-in resource types, functions, and operators with the environment.
         */
        void register_builtins();

        /**
         * Imports the initial manifests into the environment.
         * @param logger The logger to use.
         * @return Returns the initial loaded syntax trees.
         */
        std::vector<std::shared_ptr<ast::syntax_tree>> import_initial_manifests(logging::logger& logger);

        /**
         * Imports the given manifest file into the environment.
         * @param logger The logger to use.
         * @param path The path to the manifest file to import.
         * @return Returns the imported syntax tree.
         */
        std::shared_ptr<ast::syntax_tree> import_manifest(logging::logger& logger, std::string const& path);

        /**
         * Imports the given Puppet source code into the environment.
         * @param logger The logger to use.
         * @param source The Puppet source code to import.
         * @param path The path where the source code came from.
         * @return Returns the syntax tree representing the imported source.
         */
        std::shared_ptr<ast::syntax_tree> import_source(logging::logger& logger, std::string source, std::string path = "<string>");

        /**
         * Finds a class definition by normalized name.
         * @param logger The logger to use.
         * @param name The normalized name of the class to find (e.g. foo::bar).
         * @return Returns the class definition or nullptr if the class is not defined.
         */
        compiler::klass const* find_class(logging::logger& logger, std::string const& name);

        /**
         * Finds a defined type definition by normalized name.
         * @param logger The logger to use.
         * @param name The normalized name of the defined type to find (e.g. foo::bar).
         * @return Returns the defined type or nullptr if the defined type is not defined.
         */
        compiler::defined_type const* find_defined_type(logging::logger& logger, std::string const& name);

        /**
         * Finds a function by name.
         * @param logger The logger to use.
         * @param name The name of the function to find (e.g. foo::bar).
         * @param context The AST context of where the function is being imported if not currently imported.
         * @return Returns the function descriptor or nullptr if not found.
         */
        evaluation::functions::descriptor const* find_function(logging::logger& logger, std::string const& name, ast::context const& context);

        /**
         * Finds a binary operator descriptor.
         * @param oper The binary operator to find.
         * @return Returns the binary operator descriptor or nullptr if not found.
         */
        evaluation::operators::binary::descriptor const* find_binary_operator(ast::binary_operator oper) const;

        /**
         * Finds a unary operator descriptor.
         * @param oper The unary operator to find.
         * @return Returns the unary operator descriptor or nullptr if not found.
         */
        evaluation::operators::unary::descriptor const* find_unary_operator(ast::unary_operator oper) const;

        /**
         * Finds a type alias by normalized name.
         * @param logger The logger to use.
         * @param name The normalized name of the type alias to find (e.g. foo::bar).
         * @return Returns the type alias or nullptr if not found.
         */
        compiler::type_alias const* find_type_alias(logging::logger& logger, std::string const& name);

        /**
         * Finds a resource type by normalized name.
         * @param logger The logger to use.
         * @param name The normalized name of the resource type (e.g. foo::bar).
         * @param context The AST context of where the resource type is being imported if not currently imported.
         * @return Returns the resource type or nullptr if not found.
         */
        resource_type const* find_resource_type(logging::logger& logger, std::string const& name, ast::context const& context);

        /**
         * Finds a matching node definition and node resource name for the given node.
         * @param node The node to find the node definition for.
         * @return Returns the pair of node definition and resource name if found or a nullptr if one does not exist.
         */
        std::pair<node_definition const*, std::string> find_node_definition(compiler::node const& node);

        /**
         * Resolves the path to a file.
         * Note: if the given path is absolute and the file exists, the same path will be returned.
         * @param logger The logger to use to log messages.
         * @param type The type of file to find.
         * @param path The path to the file to resolve (i.e. 'foo/bar/baz.txt' => ''.../modules/foo/files/bar/baz.txt')
         * @return Returns the resolved path to the file if it exists or an empty string is the file does not exist.
         */
        std::string resolve_path(logging::logger& logger, find_type type, std::string const& path) const;

     private:
        environment(std::string name, std::string directory, compiler::settings settings, std::shared_ptr<grpc::ChannelInterface> channel);
        void add_modules(logging::logger& logger);
        void add_modules(logging::logger& logger, std::string const& directory);
        std::pair<std::string, module const*> resolve_name(logging::logger& logger, std::string const& name, find_type type) const;
        std::shared_ptr<ast::syntax_tree> import(logging::logger& logger, std::string const& path, compiler::module const* module = nullptr);

        std::mutex _mutex;
        std::string _name;
        compiler::settings _settings;
        compiler::registry _registry;
        std::deque<module> _modules;
        std::unordered_map<std::string, module*> _module_map;
        std::vector<std::shared_ptr<ast::syntax_tree>> _initial_manifests;
        std::unordered_map<std::string, std::shared_ptr<ast::syntax_tree>> _parsed;
    };

}}  // puppet::compiler
