/**
 * @file
 * Declares Boost Spirit parsing rules.
 */
#pragma once

#include "parsers.hpp"
#include "../ast/adapted.hpp"

namespace puppet { namespace compiler { namespace parser {

/**
 * Macro for declaring a parser rule.
 * @param name The name of the rule.
 * @param description The rule description.
 * @param ... The AST type generated by the rule.
 */
#define DECLARE_RULE(name, description, ...) boost::spirit::x3::rule<class name, __VA_ARGS__> const name = description;
/**
 * Macro for defining a rule.
 * @param name The rule name.
 * @param rule The rule definition.
 */
#define DEFINE_RULE(name, rule) auto const name##_def = rule;

    /// @cond NOT_DOCUMENTED

    // Rule declarations
    DECLARE_RULE(undef,                         "undef",                         ast::undef)
    DECLARE_RULE(defaulted,                     "default",                       ast::defaulted)
    DECLARE_RULE(boolean,                       "boolean",                       ast::boolean)
    DECLARE_RULE(number,                        "number",                        ast::number)
    DECLARE_RULE(string,                        "string",                        ast::string)
    DECLARE_RULE(regex,                         "regex",                         ast::regex)
    DECLARE_RULE(variable,                      "variable",                      ast::variable)
    DECLARE_RULE(name,                          "name",                          ast::name)
    DECLARE_RULE(bare_word,                     "bare word",                     ast::bare_word)
    DECLARE_RULE(type,                          "type",                          ast::type)
    DECLARE_RULE(array,                         "array",                         ast::array)
    DECLARE_RULE(hash,                          "hash",                          ast::hash)
    DECLARE_RULE(pairs,                         "pairs",                         std::vector<ast::pair>)
    DECLARE_RULE(pair,                          "pair",                          ast::pair)
    DECLARE_RULE(case_expression,               "case expression",               ast::case_expression)
    DECLARE_RULE(case_proposition,              "case proposition",              ast::case_proposition)
    DECLARE_RULE(if_expression,                 "if expression",                 ast::if_expression)
    DECLARE_RULE(elsif_expression,              "elsif expression",              ast::elsif_expression)
    DECLARE_RULE(else_expression,               "else expression",               ast::else_expression)
    DECLARE_RULE(unless_expression,             "unless expression",             ast::unless_expression)
    DECLARE_RULE(function_call_expression,      "function call expression",      ast::function_call_expression)
    DECLARE_RULE(parameters,                    "parameters",                    std::vector<ast::parameter>)
    DECLARE_RULE(parameter,                     "parameter",                     ast::parameter)
    DECLARE_RULE(type_expression,               "type expression",               ast::postfix_expression)
    DECLARE_RULE(lambda_expression,             "lambda expression",             ast::lambda_expression)
    DECLARE_RULE(statement_call_expression,     "statement call expression",     ast::function_call_expression)
    DECLARE_RULE(statement_call_name,           "name",                          ast::name)
    DECLARE_RULE(resource_expression,           "resource expression",           ast::resource_expression)
    DECLARE_RULE(resource_type,                 "resource type",                 ast::postfix_expression)
    DECLARE_RULE(class_name,                    "name",                          ast::name)
    DECLARE_RULE(resource_bodies,               "resource bodies",               std::vector<ast::resource_body>)
    DECLARE_RULE(resource_body,                 "resource body",                 ast::resource_body)
    DECLARE_RULE(attributes,                    "attributes",                    std::vector<ast::attribute>)
    DECLARE_RULE(attribute,                     "attribute",                     ast::attribute)
    DECLARE_RULE(attribute_operator,            "attribute operator",            ast::attribute_operator)
    DECLARE_RULE(attribute_name,                "attribute name",                ast::name)
    DECLARE_RULE(keyword_name,                  "name",                          ast::name)
    DECLARE_RULE(resource_override_expression,  "resource override expression",  ast::resource_override_expression)
    DECLARE_RULE(resource_reference_expression, "resource reference expression", ast::postfix_expression)
    DECLARE_RULE(resource_defaults_expression,  "resource defaults expression",  ast::resource_defaults_expression)
    DECLARE_RULE(class_expression,              "class expression",              ast::class_expression)
    DECLARE_RULE(defined_type_expression,       "defined type expression",       ast::defined_type_expression)
    DECLARE_RULE(node_expression,               "node expression",               ast::node_expression)
    DECLARE_RULE(hostnames,                     "hostnames",                     std::vector<ast::hostname>)
    DECLARE_RULE(hostname,                      "hostname",                      ast::hostname)
    DECLARE_RULE(collector_expression,          "resource collector",            ast::collector_expression)
    DECLARE_RULE(exported_collector_expression, "exported resource collector",   ast::collector_expression)
    DECLARE_RULE(collector_query_expression,    "collector query expression",    ast::collector_query_expression)
    DECLARE_RULE(attribute_query_expression,    "attribute query expression",    ast::attribute_query_expression)
    DECLARE_RULE(attribute_query,               "attribute query",               ast::attribute_query)
    DECLARE_RULE(attribute_query_operator,      "attribute query operator",      ast::attribute_query_operator)
    DECLARE_RULE(attribute_query_value,         "attribute value",               ast::primary_expression)
    DECLARE_RULE(binary_attribute_query,        "binary attribute query",        ast::binary_attribute_query)
    DECLARE_RULE(binary_query_operator,         "binary query operator",         ast::binary_query_operator)
    DECLARE_RULE(unary_expression,              "unary expression",              ast::unary_expression)
    DECLARE_RULE(unary_operator,                "unary operator",                ast::unary_operator)
    DECLARE_RULE(postfix_expression,            "postfix expression",            ast::postfix_expression)
    DECLARE_RULE(postfix_subexpression,         "postfix subexpression",         ast::postfix_subexpression)
    DECLARE_RULE(selector_expression,           "selector expression",           ast::selector_expression)
    DECLARE_RULE(access_expression,             "access expression",             ast::access_expression)
    DECLARE_RULE(method_call_expression,        "method call expression",        ast::method_call_expression)
    DECLARE_RULE(statements,                    "statements",                    std::vector<ast::expression>)
    DECLARE_RULE(statement,                     "statement",                     ast::expression)
    DECLARE_RULE(postfix_statement,             "postfix statement",             ast::postfix_expression)
    DECLARE_RULE(primary_statement,             "primary statement",             ast::primary_expression)
    DECLARE_RULE(binary_statement,              "binary statement",              ast::binary_expression)
    DECLARE_RULE(binary_operator,               "binary operator",               ast::binary_operator)
    DECLARE_RULE(expressions,                   "expressions",                   std::vector<ast::expression>)
    DECLARE_RULE(expression,                    "expression",                    ast::expression)
    DECLARE_RULE(binary_expression,             "binary expression",             ast::binary_expression)
    DECLARE_RULE(primary_expression,            "primary expression",            ast::primary_expression)
    DECLARE_RULE(syntax_tree,                   "syntax tree",                   ast::syntax_tree)
    DECLARE_RULE(interpolated_syntax_tree,      "syntax tree",                   ast::syntax_tree)
    DECLARE_RULE(epp_render_expression,         "render expression",             ast::epp_render_expression)
    DECLARE_RULE(epp_render_block,              "render expression",             ast::epp_render_block)
    DECLARE_RULE(epp_render_string,             "render string",                 ast::epp_render_string)
    DECLARE_RULE(epp_syntax_tree,               "syntax tree",                   ast::syntax_tree)

    // Literal rules
    // Note: the use of `eps` is to assist in populating a single-member fusion structure;
    //       without it, the attribute would be assigned directly instead of via fusion.
    DEFINE_RULE(
        undef,
        boost::spirit::x3::eps >> context(lexer::token_id::keyword_undef)
    )
    DEFINE_RULE(
        defaulted,
        boost::spirit::x3::eps >> context(lexer::token_id::keyword_default)
    )
    DEFINE_RULE(
        boolean,
        (context(lexer::token_id::keyword_true) > boost::spirit::x3::attr(true)) |
        (context(lexer::token_id::keyword_false) > boost::spirit::x3::attr(false))
    )
    DEFINE_RULE(
        number,
        number_token()
    )
    DEFINE_RULE(
        string,
        string_token(lexer::token_id::single_quoted_string) |
        string_token(lexer::token_id::double_quoted_string) |
        string_token(lexer::token_id::heredoc)
    )
    DEFINE_RULE(
        regex,
        context(lexer::token_id::regex, false) > token(lexer::token_id::regex, true, true)
    )
    DEFINE_RULE(
        variable,
        context(lexer::token_id::variable, false) > token(lexer::token_id::variable, true)
    )
    DEFINE_RULE(
        name,
        (context(lexer::token_id::name, false) > token(lexer::token_id::name)) |
        statement_call_name
    )
    DEFINE_RULE(
        bare_word,
        context(lexer::token_id::bare_word, false) > token(lexer::token_id::bare_word)
    )
    DEFINE_RULE(
        type,
        context(lexer::token_id::type, false) > token(lexer::token_id::type)
    )
    DEFINE_RULE(
        array,
        (
            context('[') |
            context(lexer::token_id::array_start)
        ) >
        (
            raw_token(']') |
            (expressions > raw_token(']'))
        )
    )
    DEFINE_RULE(
        hash,
        context('{') >
        (
            raw_token('}') |
            (pairs > raw_token('}'))
        )
    )
    DEFINE_RULE(
        pairs,
        (pair % raw_token(',')) > -raw_token(',')
    )
    DEFINE_RULE(
        pair,
        expression > raw_token(lexer::token_id::fat_arrow) > expression
    )

    // Control-flow expressions
    DEFINE_RULE(
        case_expression,
        context(lexer::token_id::keyword_case) > expression > raw_token('{') > +case_proposition > raw_token('}')
    )
    DEFINE_RULE(
        case_proposition,
        expressions > raw_token(':') > raw_token('{') >
        (
            raw_token('}') |
            (statements > raw_token('}'))
        )
    )
    DEFINE_RULE(
        if_expression,
        context(lexer::token_id::keyword_if) > expression > raw_token('{') >
        (
            raw_token('}') |
            (statements > raw_token('}'))
        ) > *elsif_expression > -else_expression
    )
    DEFINE_RULE(
        elsif_expression,
        context(lexer::token_id::keyword_elsif) > expression > raw_token('{') >
        (
            raw_token('}') |
            (statements > raw_token('}'))
        )
    )
    DEFINE_RULE(
        else_expression,
        context(lexer::token_id::keyword_else) > raw_token('{') >
        (
            raw_token('}') |
            (statements > raw_token('}'))
        )
    )
    DEFINE_RULE(
        unless_expression,
        context(lexer::token_id::keyword_unless) > expression > raw_token('{') >
        (
            raw_token('}') |
            (statements > raw_token('}'))
        ) > -else_expression
    )
    DEFINE_RULE(
        function_call_expression,
        name >>
        (
            raw_token('(') >
            (
                raw_token(')') |
                (expressions > raw_token(')')
            )
        ) > -lambda_expression)
    )
    DEFINE_RULE(
        parameters,
        (parameter % raw_token(',')) > -raw_token(',')
    )
    DEFINE_RULE(
        parameter,
        -type_expression >> boost::spirit::x3::matches[raw_token('*')] >> (variable > -(raw_token('=') > expression))
    )
    DEFINE_RULE(
        type_expression,
        type > *access_expression
    )
    DEFINE_RULE(
        lambda_expression,
        context('|') >
        (
            raw_token('|') |
            (parameters > raw_token('|'))
        ) > raw_token('{') >
        (
            raw_token('}') |
            (statements > raw_token('}'))
        )
    )
    DEFINE_RULE(
        statement_call_expression,
        statement_call_name >> !raw_token('(') >> expressions >> -lambda_expression
    )
    DEFINE_RULE(
        statement_call_name,
        context(lexer::token_id::statement_call, false) > token(lexer::token_id::statement_call)
    )

    // Catalog expressions
    DEFINE_RULE(
        resource_expression,
        (
            (raw_token('@')                   > boost::spirit::x3::attr(ast::resource_status::virtualized)) |
            (raw_token(lexer::token_id::atat) > boost::spirit::x3::attr(ast::resource_status::exported))    |
            (boost::spirit::x3::eps           > boost::spirit::x3::attr(ast::resource_status::realized))
        ) >> resource_type >>
        (
            raw_token('{') >
            (
                raw_token('}') |
                (resource_bodies > raw_token('}'))
            )
        )
    )
    DEFINE_RULE(
        resource_type,
        ((name | class_name) > boost::spirit::x3::attr(std::vector<ast::postfix_subexpression>())) |
        type_expression
    )
    DEFINE_RULE(
        class_name,
        context(lexer::token_id::keyword_class, false) > token(lexer::token_id::keyword_class)
    )
    DEFINE_RULE(
        resource_bodies,
        (resource_body % raw_token(';')) > -raw_token(';')
    )
    DEFINE_RULE(
        resource_body,
        primary_expression > raw_token(':') > (attributes | boost::spirit::x3::eps)
    )
    DEFINE_RULE(
        attributes,
        (attribute % raw_token(',')) > -raw_token(',')
    )
    DEFINE_RULE(
        attribute,
        attribute_name > attribute_operator > expression
    )
    DEFINE_RULE(
        attribute_operator,
        (raw_token(lexer::token_id::fat_arrow)  > boost::spirit::x3::attr(ast::attribute_operator::assignment)) |
        (raw_token(lexer::token_id::plus_arrow) > boost::spirit::x3::attr(ast::attribute_operator::append))
    )
    DEFINE_RULE(
        attribute_name,
        name                |
        statement_call_name |
        keyword_name        |
        (context('*', false) > token('*'))
    )
    DEFINE_RULE(
        keyword_name,
        (context(lexer::token_id::keyword_and, false)      > token(lexer::token_id::keyword_and))      |
        (context(lexer::token_id::keyword_case, false)     > token(lexer::token_id::keyword_case))     |
        (context(lexer::token_id::keyword_class, false)    > token(lexer::token_id::keyword_class))    |
        (context(lexer::token_id::keyword_default, false)  > token(lexer::token_id::keyword_default))  |
        (context(lexer::token_id::keyword_define, false)   > token(lexer::token_id::keyword_define))   |
        (context(lexer::token_id::keyword_else, false)     > token(lexer::token_id::keyword_else))     |
        (context(lexer::token_id::keyword_elsif, false)    > token(lexer::token_id::keyword_elsif))    |
        (context(lexer::token_id::keyword_if, false)       > token(lexer::token_id::keyword_if))       |
        (context(lexer::token_id::keyword_in, false)       > token(lexer::token_id::keyword_in))       |
        (context(lexer::token_id::keyword_inherits, false) > token(lexer::token_id::keyword_inherits)) |
        (context(lexer::token_id::keyword_node, false)     > token(lexer::token_id::keyword_node))     |
        (context(lexer::token_id::keyword_or, false)       > token(lexer::token_id::keyword_or))       |
        (context(lexer::token_id::keyword_undef, false)    > token(lexer::token_id::keyword_undef))    |
        (context(lexer::token_id::keyword_unless, false)   > token(lexer::token_id::keyword_unless))   |
        (context(lexer::token_id::keyword_type, false)     > token(lexer::token_id::keyword_type))     |
        (context(lexer::token_id::keyword_attr, false)     > token(lexer::token_id::keyword_attr))     |
        (context(lexer::token_id::keyword_function, false) > token(lexer::token_id::keyword_function)) |
        (context(lexer::token_id::keyword_private, false)  > token(lexer::token_id::keyword_private))
    )
    DEFINE_RULE(
        resource_override_expression,
        resource_reference_expression >>
        (
            raw_token('{') >
            (
                raw_token('}') |
                (attributes > raw_token('}'))
            )
        )
    )
    DEFINE_RULE(
        resource_reference_expression,
        (type | variable) > *access_expression
    )
    DEFINE_RULE(
        resource_defaults_expression,
        type >>
        (
            raw_token('{') >
            (
                raw_token('}') |
                (attributes > raw_token('}'))
            )
        )
    )
    DEFINE_RULE(
        class_expression,
        context(lexer::token_id::keyword_class) > name >
        (
            (raw_token('(') >> raw_token(')'))             |
            (raw_token('(') > parameters > raw_token(')')) |
            boost::spirit::x3::eps
        ) > -(raw_token(lexer::token_id::keyword_inherits) > name) > raw_token('{') >
        (
            raw_token('}') |
            (statements > raw_token('}'))
        )
    )
    DEFINE_RULE(
        defined_type_expression,
        context(lexer::token_id::keyword_define) > name >
        (
            (raw_token('(') >> raw_token(')'))             |
            (raw_token('(') > parameters > raw_token(')')) |
            boost::spirit::x3::eps
        ) > raw_token('{') >
        (
            raw_token('}') |
            (statements > raw_token('}'))
        )
    )
    DEFINE_RULE(
        node_expression,
        context(lexer::token_id::keyword_node) > hostnames > raw_token('{') >
        (
            raw_token('}') |
            (statements > raw_token('}'))
        )
    )
    DEFINE_RULE(
        hostnames,
        (hostname % ',') > -raw_token(',')
    )
    DEFINE_RULE(
        hostname,
        string    |
        defaulted |
        regex     |
        ((name | bare_word | number) % raw_token('.'))
    )
    DEFINE_RULE(
        collector_expression,
        type >>
        (
            (raw_token(lexer::token_id::left_collect) > boost::spirit::x3::attr(false)) >
            -collector_query_expression >
            raw_token(lexer::token_id::right_collect)
        )
    )
    DEFINE_RULE(
        exported_collector_expression,
        type >>
        (
            (raw_token(lexer::token_id::left_double_collect) > boost::spirit::x3::attr(true)) >
            -collector_query_expression >
            raw_token(lexer::token_id::right_double_collect)
        )
    )
    DEFINE_RULE(
        collector_query_expression,
        attribute_query_expression > *binary_attribute_query
    )
    DEFINE_RULE(
        attribute_query_expression,
        attribute_query | (raw_token('(') > collector_query_expression > raw_token(')'))
    )
    DEFINE_RULE(
        attribute_query,
        name > attribute_query_operator > attribute_query_value
    )
    DEFINE_RULE(
        attribute_query_operator,
        (raw_token(lexer::token_id::equals)     > boost::spirit::x3::attr(ast::attribute_query_operator::equals)) |
        (raw_token(lexer::token_id::not_equals) > boost::spirit::x3::attr(ast::attribute_query_operator::not_equals))
    )
    DEFINE_RULE(
        attribute_query_value,
        undef     |
        defaulted |
        boolean   |
        number    |
        string    |
        regex     |
        variable  |
        name      |
        bare_word |
        type      |
        array     |
        hash
    )
    DEFINE_RULE(
        binary_attribute_query,
        current_context >> (binary_query_operator > attribute_query_expression)
    )
    DEFINE_RULE(
        binary_query_operator,
        (raw_token(lexer::token_id::keyword_and) > boost::spirit::x3::attr(ast::binary_query_operator::logical_and)) |
        (raw_token(lexer::token_id::keyword_or)  > boost::spirit::x3::attr(ast::binary_query_operator::logical_or))
    )

    // Unary expressions
    DEFINE_RULE(
        unary_expression,
        current_context >> (unary_operator > postfix_expression)
    )
    DEFINE_RULE(
        unary_operator,
        (raw_token('-') > boost::spirit::x3::attr(ast::unary_operator::negate))     |
        (raw_token('*') > boost::spirit::x3::attr(ast::unary_operator::splat))      |
        (raw_token('!') > boost::spirit::x3::attr(ast::unary_operator::logical_not))
    )

    // Postfix expressions
    DEFINE_RULE(
        postfix_expression,
        primary_expression > *postfix_subexpression
    )
    DEFINE_RULE(
        postfix_subexpression,
        selector_expression | access_expression | method_call_expression
    )
    DEFINE_RULE(
        selector_expression,
        context('?') > raw_token('{') > pairs > raw_token('}')
    )
    DEFINE_RULE(
        access_expression,
        context('[') > expressions > raw_token(']')
    )
    DEFINE_RULE(
        method_call_expression,
        context('.') > name >
        (
            (raw_token('(') >> raw_token(')')) |
            (raw_token('(') > expressions > raw_token(')')) |
            boost::spirit::x3::eps
        ) > -lambda_expression
    )

    // Statement rules
    DEFINE_RULE(
        statements,
        (statement % -raw_token(';')) > -raw_token(';')
    )
    DEFINE_RULE(
        statement,
        postfix_statement > *binary_statement
    )
    DEFINE_RULE(
        postfix_statement,
        primary_statement > *postfix_subexpression
    )
    DEFINE_RULE(
        primary_statement,
        statement_call_expression    |
        resource_expression          |
        resource_override_expression |
        resource_defaults_expression |
        class_expression             |
        defined_type_expression      |
        node_expression              |
        primary_expression
    )
    DEFINE_RULE(
        binary_statement,
        current_context >> (binary_operator > postfix_expression)
    )
    DEFINE_RULE(
        binary_operator,
        (raw_token(lexer::token_id::keyword_in)     > boost::spirit::x3::attr(ast::binary_operator::in))                 |
        (raw_token(lexer::token_id::match)          > boost::spirit::x3::attr(ast::binary_operator::match))              |
        (raw_token(lexer::token_id::not_match)      > boost::spirit::x3::attr(ast::binary_operator::not_match))          |
        (raw_token('*')                             > boost::spirit::x3::attr(ast::binary_operator::multiply))           |
        (raw_token('/')                             > boost::spirit::x3::attr(ast::binary_operator::divide))             |
        (raw_token('%')                             > boost::spirit::x3::attr(ast::binary_operator::modulo))             |
        (raw_token('+')                             > boost::spirit::x3::attr(ast::binary_operator::plus))               |
        (raw_token('-')                             > boost::spirit::x3::attr(ast::binary_operator::minus))              |
        (raw_token(lexer::token_id::left_shift)     > boost::spirit::x3::attr(ast::binary_operator::left_shift))         |
        (raw_token(lexer::token_id::right_shift)    > boost::spirit::x3::attr(ast::binary_operator::right_shift))        |
        (raw_token(lexer::token_id::equals)         > boost::spirit::x3::attr(ast::binary_operator::equals))             |
        (raw_token(lexer::token_id::not_equals)     > boost::spirit::x3::attr(ast::binary_operator::not_equals))         |
        (raw_token('>')                             > boost::spirit::x3::attr(ast::binary_operator::greater_than))       |
        (raw_token(lexer::token_id::greater_equals) > boost::spirit::x3::attr(ast::binary_operator::greater_equals))     |
        (raw_token('<')                             > boost::spirit::x3::attr(ast::binary_operator::less_than))          |
        (raw_token(lexer::token_id::less_equals)    > boost::spirit::x3::attr(ast::binary_operator::less_equals))        |
        (raw_token(lexer::token_id::keyword_and)    > boost::spirit::x3::attr(ast::binary_operator::logical_and))        |
        (raw_token(lexer::token_id::keyword_or)     > boost::spirit::x3::attr(ast::binary_operator::logical_or))         |
        (raw_token('=')                             > boost::spirit::x3::attr(ast::binary_operator::assignment))         |
        (raw_token(lexer::token_id::in_edge)        > boost::spirit::x3::attr(ast::binary_operator::in_edge))            |
        (raw_token(lexer::token_id::in_edge_sub)    > boost::spirit::x3::attr(ast::binary_operator::in_edge_subscribe))  |
        (raw_token(lexer::token_id::out_edge)       > boost::spirit::x3::attr(ast::binary_operator::out_edge))           |
        (raw_token(lexer::token_id::out_edge_sub)   > boost::spirit::x3::attr(ast::binary_operator::out_edge_subscribe))
    )

    // Expression rules
    DEFINE_RULE(
        expressions,
        (expression % raw_token(',')) > -raw_token(',')
    )
    DEFINE_RULE(
        expression,
        postfix_expression > *binary_expression
    )
    DEFINE_RULE(
        binary_expression,
        current_context >> (binary_operator > postfix_expression)
    )
    // Note: literal expressions must come last because some complex expressions depend on them
    // Note: parsing of EPP render block must come before EPP render expression
    DEFINE_RULE(
        primary_expression,
        epp_render_block              |
        epp_render_expression         |
        epp_render_string             |
        unary_expression              |
        case_expression               |
        if_expression                 |
        unless_expression             |
        function_call_expression      |
        collector_expression          |
        exported_collector_expression |
        undef                         |
        defaulted                     |
        boolean                       |
        number                        |
        string                        |
        regex                         |
        variable                      |
        name                          |
        bare_word                     |
        type                          |
        array                         |
        hash                          |
        (raw_token('(') > expression > raw_token(')'))
    )
    DEFINE_RULE(
        syntax_tree,
        boost::spirit::x3::attr(boost::none) > statements > boost::spirit::x3::attr(lexer::position())
    )
    DEFINE_RULE(
        interpolated_syntax_tree,
        boost::spirit::x3::attr(boost::none) > raw_token('{') > statements > position('}')
    )

    // EPP rules
    DEFINE_RULE(
        epp_render_expression,
        context(lexer::token_id::epp_render_expression) > expression > (raw_token(lexer::token_id::epp_end) | raw_token(lexer::token_id::epp_end_trim))
    )
    DEFINE_RULE(
        epp_render_block,
        context(lexer::token_id::epp_render_expression) >> (raw_token('{') > statements > raw_token('}') > (raw_token(lexer::token_id::epp_end) | raw_token(lexer::token_id::epp_end_trim)))
    )
    DEFINE_RULE(
        epp_render_string,
        context(lexer::token_id::epp_render_string, false) > token(lexer::token_id::epp_render_string)
    )
    DEFINE_RULE(
        epp_syntax_tree,
        -(
            raw_token('|') >
            (
                raw_token('|') |
                (parameters > raw_token('|'))
            )
        ) > statements > boost::spirit::x3::attr(lexer::position())
    )

    // These macros associate the above rules with their definitions
    // Too many rules to associate in a single macro
    BOOST_SPIRIT_DEFINE(
        undef,
        defaulted,
        boolean,
        number,
        string,
        regex,
        variable,
        name,
        bare_word,
        type,
        array,
        hash,
        pairs,
        pair
    );

    BOOST_SPIRIT_DEFINE(
        case_expression,
        case_proposition,
        if_expression,
        elsif_expression,
        else_expression,
        unless_expression,
        function_call_expression,
        parameters,
        parameter,
        type_expression,
        lambda_expression,
        statement_call_expression,
        statement_call_name
    );

    BOOST_SPIRIT_DEFINE(
        resource_expression,
        resource_type,
        class_name,
        resource_bodies,
        resource_body,
        attributes,
        attribute,
        attribute_operator,
        attribute_name,
        keyword_name,
        resource_override_expression,
        resource_reference_expression,
        resource_defaults_expression,
        class_expression,
        defined_type_expression,
        node_expression,
        hostnames,
        hostname,
        collector_expression,
        exported_collector_expression,
        collector_query_expression,
        attribute_query_expression,
        attribute_query,
        attribute_query_operator,
        attribute_query_value,
        binary_attribute_query,
        binary_query_operator
    );

    BOOST_SPIRIT_DEFINE(
        unary_expression,
        unary_operator,
        postfix_expression,
        postfix_subexpression,
        selector_expression,
        access_expression,
        method_call_expression,
        statements,
        statement,
        postfix_statement,
        primary_statement,
        binary_statement,
        binary_operator,
        expressions,
        expression,
        binary_expression,
        primary_expression,
        syntax_tree,
        interpolated_syntax_tree
    );

    BOOST_SPIRIT_DEFINE(
        epp_render_expression,
        epp_render_block,
        epp_render_string,
        epp_syntax_tree
    )

    /// @endcond

}}}  // namespace puppet::compiler::parser