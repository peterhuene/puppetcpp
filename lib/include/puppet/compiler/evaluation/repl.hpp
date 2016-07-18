/**
 * @file
 * Declares the REPL (read-evel-print-loop) environment.
 */
#pragma once

#include "context.hpp"
#include "../exceptions.hpp"
#include <boost/optional.hpp>
#include <string>
#include <vector>
#include <memory>

namespace puppet { namespace compiler { namespace evaluation {

    /**
     * Represents the REPL environment.
     */
    struct repl
    {
        /**
         * Represents the result of a REPL evaluation.
         */
        struct result
        {
            /**
             * Stores the text of the source that was evaluated.
             */
            std::string source;

            /**
             * Stores the resulting value of the evaluation.
             */
            runtime::values::value value;

            /**
             * Stores the resulting exception or none if the evaluation completed successfully.
             */
            boost::optional<compilation_exception> exception;
        };

        /**
         * Constructs a new REPL with the given evaluation context.
         * @param context The evaluation context to use.
         */
        explicit repl(evaluation::context& context);

        /**
         * Gets the current prompt.
         * The prompt changes upon every completed evaluation.
         * @return Returns the current prompt.
         */
        std::string const& prompt() const;

        /**
         * Gets the count of completed commands.
         * @return Returns the count of completed commands.
         */
        size_t count() const;

        /**
         * Gets the current command's line number.
         * If the line number is 1, this indicates the start of a new command.
         * @return Returns the current command's line number.
         */
        size_t line() const;

        /**
         * Evaluates the given source.
         * @param source The source to evaluate.
         * @return Returns the evaluation result or boost::none if the source is not yet complete.
         */
        boost::optional<result> evaluate(std::string const& source);

        /**
         * Evaluates the given source.
         * @param source The source to evaluate.
         * @return Returns the evaluation result or boost::none if the source is not yet complete.
         */
        boost::optional<result> evaluate(char const* source);

     private:
        friend struct evaluation_helper;
        void complete(bool multiline);

        evaluation::context& _context;
        std::string _buffer;
        std::string _prompt;
        size_t _count = 1;
        size_t _line = 1;
    };

}}}  // namespace puppet::compiler::evaluation
