/**
 * @file
 * Declares the definition scanner.
 */
#pragma once

#include "ast/ast.hpp"
#include "registry.hpp"
#include "../logging/logger.hpp"
#include <vector>
#include <string>
#include <memory>

namespace puppet { namespace compiler {

    /**
     * Represents the definition scanner.
     */
    struct scanner
    {
        /**
         * Constructs a definition scanner.
         * Note: it is assumed the scanner is operating under the environment lock.
         * @param logger The logger to use.
         * @param environment The name of the environment.
         * @param registry The registry to populate with definitions.
         */
        scanner(logging::logger& logger, std::string const& environment, compiler::registry& registry);

        /**
         * Scans the given syntax tree for definitions.
         * This requires that the syntax tree has been validated.
         * Only top-level and class statements are visited.
         * Throws parse exceptions if there are conflicting definitions.
         * @param tree The syntax tree to scan for definitions.
         * @return Returns true if a definition was regsitered or false if not.
         */
        bool scan(ast::syntax_tree const& tree);

     private:
        void register_class(std::string name, ast::class_statement const& statement);
        void register_defined_type(std::string name, ast::defined_type_statement const& statement);
        void register_node(ast::node_statement const& statement);
        void register_function(ast::function_statement const& statement);
        void register_type_alias(ast::type_alias_statement const& statement);
        void register_produces(ast::produces_statement const& statement);
        void register_consumes(ast::consumes_statement const& statement);
        void register_application(ast::application_statement const& statement);
        void register_site(ast::site_statement const& statement);
        void check_resource_type(std::string const& name, std::string const& normalized_name, ast::context const& context, char const* type);

        logging::logger& _logger;
        std::string const& _environment;
        registry& _registry;
    };

}}  // puppet::compiler
