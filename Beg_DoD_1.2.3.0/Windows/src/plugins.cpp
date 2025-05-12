#include "plugins.h"
#include <iostream>
#include <windows.h> // For GetModuleFileNameA and directory operations
#include <filesystem>
#include <thread>
#include <mutex>
#include <atomic>
#include <fstream>

// Ensure g_cheatdata is declared (it should be defined and initialized in your main project)
// If not, you'll get a linker error.
// For testing within this file, you could add a dummy one, but it's better to link properly.
// extern CheatGlobalData* g_cheatdata; 
// CheatGlobalData dummy_cheat_data; // If you must define it here for a self-contained example
// CheatGlobalData* g_cheatdata = &dummy_cheat_data;

namespace BegeerteScript {

    static std::filesystem::path ScriptDirectory;
    static std::filesystem::path LogDirectory;
    static std::mutex LogMutex;

    // --- Interpreter Implementation (Simplified) ---
    void Interpreter::Execute(const std::string& script_content, ScriptContext& context) {
        std::vector<Token> tokens = Tokenize(script_content, context.current_script_path);
        size_t index = 0;
        try {
            while (index < tokens.size() && tokens[index].type != Token::Type::END_OF_FILE) {
                ParseStatement(tokens, index, context, context.current_script_path);
                if (index < tokens.size() && tokens[index].type == Token::Type::END_OF_LINE) {
                    index++; // Consume EOL
                }
            }
        }
        catch (const std::exception& e) {
            // SyntaxError already prints, this catches other runtime_errors from parsing if any
            // Or if SyntaxError doesn't rethrow but we want to stop.
            std::cerr << "Execution halted in '" << context.current_script_path << "' due to error: " << e.what() << std::endl;
            throw; // Re-throw to allow caller to handle unloading
        }
    }

    // Very basic tokenizer
    std::vector<Interpreter::Token> Interpreter::Tokenize(const std::string& script_content, const std::string& script_path) {
        std::vector<Token> tokens;
        std::string current_token_text;
        size_t line_number = 1;

        for (size_t i = 0; i < script_content.length(); ++i) {
            char c = script_content[i];

            if (c == '\n') {
                line_number++;
                tokens.push_back({ Token::Type::END_OF_LINE, ";", line_number - 1 }); // Treat newline as EOL/semicolon
                continue;
            }
            if (std::isspace(c)) continue; // Skip whitespace

            // Comments
            if (c == '/' && i + 1 < script_content.length()) {
                if (script_content[i + 1] == '/') { // Single line comment
                    while (i < script_content.length() && script_content[i] != '\n') {
                        i++;
                    }
                    if (i < script_content.length() && script_content[i] == '\n') line_number++;
                    tokens.push_back({ Token::Type::END_OF_LINE, ";", line_number - 1 });
                    continue;
                }
                else if (script_content[i + 1] == '*') { // Multi-line comment
                    i += 2;
                    while (i + 1 < script_content.length() && !(script_content[i] == '*' && script_content[i + 1] == '/')) {
                        if (script_content[i] == '\n') line_number++;
                        i++;
                    }
                    i++; // Skip the final '/'
                    continue;
                }
            }

            // Operators and special characters
            if (std::string("=(){},;+-*/%&|!<>").find(c) != std::string::npos) {
                if (c == '=' && i + 1 < script_content.length() && script_content[i + 1] == '=') { // ==
                    tokens.push_back({ Token::Type::OPERATOR, "==", line_number });
                    i++;
                }
                else if (c == '!' && i + 1 < script_content.length() && script_content[i + 1] == '=') { // !=
                    tokens.push_back({ Token::Type::OPERATOR, "!=", line_number });
                    i++;
                }
                else if (c == '<' && i + 1 < script_content.length() && script_content[i + 1] == '=') { // <=
                    tokens.push_back({ Token::Type::OPERATOR, "<=", line_number });
                    i++;
                }
                else if (c == '>' && i + 1 < script_content.length() && script_content[i + 1] == '=') { // >=
                    tokens.push_back({ Token::Type::OPERATOR, ">=", line_number });
                    i++;
                }
                else if (c == '&' && i + 1 < script_content.length() && script_content[i + 1] == '&') { // &&
                    tokens.push_back({ Token::Type::OPERATOR, "&&", line_number });
                    i++;
                }
                else if (c == '|' && i + 1 < script_content.length() && script_content[i + 1] == '|') { // ||
                    tokens.push_back({ Token::Type::OPERATOR, "||", line_number });
                    i++;
                }
                else {
                    tokens.push_back({ Token::Type::OPERATOR, std::string(1, c), line_number });
                }
                continue;
            }

            // Identifiers (and keywords)
            if (std::isalpha(c) || c == '_') {
                current_token_text = c;
                while (i + 1 < script_content.length() && (std::isalnum(script_content[i + 1]) || script_content[i + 1] == '_')) {
                    current_token_text += script_content[++i];
                }
                if (current_token_text == "let" || current_token_text == "if" || current_token_text == "else" ||
                    current_token_text == "while" || current_token_text == "true" || current_token_text == "false" ||
                    current_token_text == "nil") {
                    tokens.push_back({ Token::Type::KEYWORD, current_token_text, line_number });
                }
                else {
                    tokens.push_back({ Token::Type::IDENTIFIER, current_token_text, line_number });
                }
                continue;
            }

            // Numbers (integer and float)
            if (std::isdigit(c) || (c == '.' && i + 1 < script_content.length() && std::isdigit(script_content[i + 1]))) {
                current_token_text = c;
                bool has_decimal = (c == '.');
                while (i + 1 < script_content.length() && (std::isdigit(script_content[i + 1]) || (!has_decimal && script_content[i + 1] == '.'))) {
                    current_token_text += script_content[++i];
                    if (script_content[i] == '.') has_decimal = true;
                }
                tokens.push_back({ Token::Type::NUMBER, current_token_text, line_number });
                continue;
            }

            // Strings
            if (c == '"') {
                current_token_text = ""; // Don't include quotes in value
                i++; // Skip opening quote
                while (i < script_content.length() && script_content[i] != '"') {
                    if (script_content[i] == '\\' && i + 1 < script_content.length()) { // Handle escape sequences (basic)
                        i++;
                        switch (script_content[i]) {
                        case 'n': current_token_text += '\n'; break;
                        case 't': current_token_text += '\t'; break;
                        case '"': current_token_text += '"'; break;
                        case '\\': current_token_text += '\\'; break;
                        default: current_token_text += script_content[i]; // Add char as is
                        }
                    }
                    else {
                        current_token_text += script_content[i];
                    }
                    if (script_content[i] == '\n') line_number++; // String can span lines
                    i++;
                }
                if (i == script_content.length()) { // Unterminated string
                    SyntaxError("Unterminated string literal", script_path, line_number);
                }
                tokens.push_back({ Token::Type::STRING, current_token_text, line_number });
                continue;
            }

            SyntaxError("Unexpected character: " + std::string(1, c), script_path, line_number);
            tokens.push_back({ Token::Type::UNKNOWN, std::string(1,c), line_number }); // Should be caught by SyntaxError
        }
        tokens.push_back({ Token::Type::END_OF_FILE, "", line_number });
        return tokens;
    }

    Value Interpreter::ParseFactor(const std::vector<Token>& tokens, size_t& index, ScriptContext& context, const std::string& script_path) {
        if (index >= tokens.size()) SyntaxError("Unexpected end of input, expected factor", script_path, (tokens.empty() ? 0 : tokens.back().line_number));
        const auto& token = tokens[index];

        if (token.type == Token::Type::NUMBER) {
            index++;
            if (token.text.find('.') != std::string::npos) return Value(std::stod(token.text));
            return Value(std::stoll(token.text));
        }
        if (token.type == Token::Type::STRING) {
            index++;
            return Value(token.text);
        }
        if (token.type == Token::Type::KEYWORD && token.text == "true") {
            index++;
            return Value(true);
        }
        if (token.type == Token::Type::KEYWORD && token.text == "false") {
            index++;
            return Value(false);
        }
        if (token.type == Token::Type::KEYWORD && token.text == "nil") {
            index++;
            return Value();
        }
        if (token.type == Token::Type::IDENTIFIER) {
            std::string var_name = token.text;
            index++;
            if (index < tokens.size() && tokens[index].type == Token::Type::OPERATOR && tokens[index].text == "(") { // Function call
                index++; // Consume '('
                std::vector<Value> args = ParseArgumentList(tokens, index, context, script_path);
                if (index >= tokens.size() || tokens[index].type != Token::Type::OPERATOR || tokens[index].text != ")") {
                    SyntaxError("Expected ')' after function arguments", script_path, token.line_number);
                }
                index++; // Consume ')'
                return context.CallFunction(var_name, args);
            }
            return context.GetVariable(var_name); // Variable access
        }
        if (token.type == Token::Type::OPERATOR && token.text == "(") { // Parenthesized expression
            index++; // Consume '('
            Value val = ParseExpression(tokens, index, context, script_path);
            if (index >= tokens.size() || tokens[index].type != Token::Type::OPERATOR || tokens[index].text != ")") {
                SyntaxError("Expected ')' after expression", script_path, token.line_number);
            }
            index++; // Consume ')'
            return val;
        }
        // Basic unary minus/not for demonstration. Could be expanded.
        if (token.type == Token::Type::OPERATOR && (token.text == "-" || token.text == "!")) {
            std::string op = token.text;
            index++;
            Value operand = ParseFactor(tokens, index, context, script_path); // Higher precedence for unary
            if (op == "-") {
                if (operand.GetType() == Value::Type::NUMBER_INT) return Value(-operand.AsInt());
                if (operand.GetType() == Value::Type::NUMBER_FLOAT) return Value(-operand.AsFloat());
                SyntaxError("Operand for unary '-' must be a number.", script_path, token.line_number);
            }
            if (op == "!") {
                return Value(!operand.IsTruthy());
            }
        }

        SyntaxError("Unexpected token '" + token.text + "', expected a value, variable, or function call.", script_path, token.line_number);
        return Value(); // Should not reach here
    }

    // Placeholder for Term (handles * / %) - for simplicity, merge into Expression or Factor for now
    Value Interpreter::ParseTerm(const std::vector<Token>& tokens, size_t& index, ScriptContext& context, const std::string& script_path) {
        Value left = ParseFactor(tokens, index, context, script_path);
        while (index < tokens.size() && tokens[index].type == Token::Type::OPERATOR &&
            (tokens[index].text == "*" || tokens[index].text == "/" || tokens[index].text == "%")) {
            Token op_token = tokens[index];
            index++;
            Value right = ParseFactor(tokens, index, context, script_path);

            if (left.GetType() != Value::Type::NUMBER_INT && left.GetType() != Value::Type::NUMBER_FLOAT ||
                right.GetType() != Value::Type::NUMBER_INT && right.GetType() != Value::Type::NUMBER_FLOAT) {
                SyntaxError("Operands for '" + op_token.text + "' must be numbers.", script_path, op_token.line_number);
            }

            bool use_float = (left.GetType() == Value::Type::NUMBER_FLOAT || right.GetType() == Value::Type::NUMBER_FLOAT);
            double lval_f = left.AsFloat();
            double rval_f = right.AsFloat();
            long long lval_i = left.AsInt();
            long long rval_i = right.AsInt();

            if (op_token.text == "*") {
                left = use_float ? Value(lval_f * rval_f) : Value(lval_i * rval_i);
            }
            else if (op_token.text == "/") {
                if ((use_float && rval_f == 0.0) || (!use_float && rval_i == 0)) SyntaxError("Division by zero.", script_path, op_token.line_number);
                left = use_float ? Value(lval_f / rval_f) : Value(lval_i / rval_i);
            }
            else if (op_token.text == "%") {
                if (use_float) SyntaxError("Modulo operator '%' not supported for floats.", script_path, op_token.line_number);
                if (rval_i == 0) SyntaxError("Modulo by zero.", script_path, op_token.line_number);
                left = Value(lval_i % rval_i);
            }
        }
        return left;
    }

    // Placeholder for Expression (handles + - and logical operators)
    Value Interpreter::ParseExpression(const std::vector<Token>& tokens, size_t& index, ScriptContext& context, const std::string& script_path) {
        Value left = ParseTerm(tokens, index, context, script_path); // Or ParseComparison for logical ops
        while (index < tokens.size() && tokens[index].type == Token::Type::OPERATOR &&
            (tokens[index].text == "+" || tokens[index].text == "-" ||
                tokens[index].text == "==" || tokens[index].text == "!=" ||
                tokens[index].text == "<" || tokens[index].text == "<=" ||
                tokens[index].text == ">" || tokens[index].text == ">=" ||
                tokens[index].text == "&&" || tokens[index].text == "||")) {
            Token op_token = tokens[index];
            index++;
            Value right = ParseTerm(tokens, index, context, script_path); // Or ParseComparison

            // Arithmetic
            if (op_token.text == "+") {
                if ((left.GetType() == Value::Type::NUMBER_INT || left.GetType() == Value::Type::NUMBER_FLOAT) &&
                    (right.GetType() == Value::Type::NUMBER_INT || right.GetType() == Value::Type::NUMBER_FLOAT)) {
                    bool use_float = (left.GetType() == Value::Type::NUMBER_FLOAT || right.GetType() == Value::Type::NUMBER_FLOAT);
                    left = use_float ? Value(left.AsFloat() + right.AsFloat()) : Value(left.AsInt() + right.AsInt());
                }
                else if (left.GetType() == Value::Type::STRING || right.GetType() == Value::Type::STRING) { // String concatenation
                    left = Value(left.AsString() + right.AsString());
                }
                else {
                    SyntaxError("Invalid operands for '+'. Must be numbers or at least one string.", script_path, op_token.line_number);
                }
            }
            else if (op_token.text == "-") {
                if ((left.GetType() == Value::Type::NUMBER_INT || left.GetType() == Value::Type::NUMBER_FLOAT) &&
                    (right.GetType() == Value::Type::NUMBER_INT || right.GetType() == Value::Type::NUMBER_FLOAT)) {
                    bool use_float = (left.GetType() == Value::Type::NUMBER_FLOAT || right.GetType() == Value::Type::NUMBER_FLOAT);
                    left = use_float ? Value(left.AsFloat() - right.AsFloat()) : Value(left.AsInt() - right.AsInt());
                }
                else {
                    SyntaxError("Operands for '-' must be numbers.", script_path, op_token.line_number);
                }
            }
            // Comparisons (simplified, no type coercion beyond basic number types)
            else if (op_token.text == "==") {
                // Basic equality, can be expanded
                if (left.GetType() == right.GetType()) {
                    if (left.GetType() == Value::Type::NIL) left = Value(true);
                    else if (left.GetType() == Value::Type::BOOL) left = Value(left.AsBool() == right.AsBool());
                    else if (left.GetType() == Value::Type::NUMBER_INT) left = Value(left.AsInt() == right.AsInt());
                    else if (left.GetType() == Value::Type::NUMBER_FLOAT) left = Value(left.AsFloat() == right.AsFloat()); // Careful with float equality
                    else if (left.GetType() == Value::Type::STRING) left = Value(left.AsString() == right.AsString());
                    else if (left.GetType() == Value::Type::PLAYER_PTR) left = Value(left.AsPlayer() == right.AsPlayer());
                    else left = Value(false); // Cannot compare other types for now
                }
                else if ((left.GetType() == Value::Type::NUMBER_INT && right.GetType() == Value::Type::NUMBER_FLOAT) ||
                    (left.GetType() == Value::Type::NUMBER_FLOAT && right.GetType() == Value::Type::NUMBER_INT)) {
                    left = Value(left.AsFloat() == right.AsFloat());
                }
                else left = Value(false);
            }
            else if (op_token.text == "!=") { // Implement similar to == but negated
                if (left.GetType() == right.GetType()) {
                    if (left.GetType() == Value::Type::NIL) left = Value(false);
                    else if (left.GetType() == Value::Type::BOOL) left = Value(left.AsBool() != right.AsBool());
                    else if (left.GetType() == Value::Type::NUMBER_INT) left = Value(left.AsInt() != right.AsInt());
                    else if (left.GetType() == Value::Type::NUMBER_FLOAT) left = Value(left.AsFloat() != right.AsFloat());
                    else if (left.GetType() == Value::Type::STRING) left = Value(left.AsString() != right.AsString());
                    else if (left.GetType() == Value::Type::PLAYER_PTR) left = Value(left.AsPlayer() != right.AsPlayer());
                    else left = Value(true);
                }
                else if ((left.GetType() == Value::Type::NUMBER_INT && right.GetType() == Value::Type::NUMBER_FLOAT) ||
                    (left.GetType() == Value::Type::NUMBER_FLOAT && right.GetType() == Value::Type::NUMBER_INT)) {
                    left = Value(left.AsFloat() != right.AsFloat());
                }
                else left = Value(true);
            }
            // Relational (numbers only for now)
#define NUMERIC_COMPARE(op_char) \
                if ((left.GetType() == Value::Type::NUMBER_INT || left.GetType() == Value::Type::NUMBER_FLOAT) && \
                    (right.GetType() == Value::Type::NUMBER_INT || right.GetType() == Value::Type::NUMBER_FLOAT)) { \
                    left = Value(left.AsFloat() op_char right.AsFloat()); \
                } else { SyntaxError("Operands for '" + op_token.text + "' must be numbers.", script_path, op_token.line_number); }

            else if (op_token.text == "<") { NUMERIC_COMPARE(< ) }
            else if (op_token.text == "<=") { NUMERIC_COMPARE(<= ) }
            else if (op_token.text == ">") { NUMERIC_COMPARE(> ) }
            else if (op_token.text == ">=") { NUMERIC_COMPARE(>= ) }
            // Logical
            else if (op_token.text == "&&") {
                if (!left.IsTruthy()) { // Short-circuit
                    left = Value(false);
                    // Need to skip evaluation of 'right' by advancing index past it properly
                    // This simplified parser doesn't handle short-circuiting perfectly in ParseTerm yet
                }
                else {
                    left = Value(right.IsTruthy());
                }
            }
            else if (op_token.text == "||") {
                if (left.IsTruthy()) { // Short-circuit
                    left = Value(true);
                    // Skip 'right'
                }
                else {
                    left = Value(right.IsTruthy());
                }
            }
        }
        return left;
    }

    std::vector<Value> Interpreter::ParseArgumentList(const std::vector<Token>& tokens, size_t& index, ScriptContext& context, const std::string& script_path) {
        std::vector<Value> args;
        if (tokens[index].type == Token::Type::OPERATOR && tokens[index].text == ")") { // Empty arg list
            return args;
        }
        while (index < tokens.size()) {
            args.push_back(ParseExpression(tokens, index, context, script_path));
            if (index < tokens.size() && tokens[index].type == Token::Type::OPERATOR && tokens[index].text == ",") {
                index++; // Consume ','
            }
            else {
                break; // End of arguments or syntax error (handled by caller checking for ')')
            }
        }
        return args;
    }

    void Interpreter::ParseStatement(const std::vector<Token>& tokens, size_t& index, ScriptContext& context, const std::string& script_path) {
        if (index >= tokens.size() || tokens[index].type == Token::Type::END_OF_FILE) return;

        const auto& current_token = tokens[index];

        if (current_token.type == Token::Type::KEYWORD && current_token.text == "let") {
            ParseAssignmentOrFunctionCall(tokens, index, context, script_path);
        }
        else if (current_token.type == Token::Type::IDENTIFIER) {
            // Could be assignment (if next is '=') or just a function call
            if (index + 1 < tokens.size() && tokens[index + 1].type == Token::Type::OPERATOR && tokens[index + 1].text == "=") {
                ParseAssignmentOrFunctionCall(tokens, index, context, script_path);
            }
            else { // Assume function call statement if not assignment
                ParseFactor(tokens, index, context, script_path); // This will parse and execute the function call
            }
        }
        else if (current_token.type == Token::Type::KEYWORD && current_token.text == "if") {
            ParseIfStatement(tokens, index, context, script_path);
        }
        else if (current_token.type == Token::Type::KEYWORD && current_token.text == "while") {
            ParseWhileStatement(tokens, index, context, script_path);
        }
        else if (current_token.type == Token::Type::OPERATOR && current_token.text == "{") {
            ParseBlock(tokens, index, context, script_path); // Standalone block (less common but possible)
        }
        else if (current_token.type == Token::Type::END_OF_LINE) {
            index++; // Consume EOL, do nothing
            return;
        }
        else {
            SyntaxError("Unexpected token at start of statement: " + current_token.text, script_path, current_token.line_number);
        }

        // Expect a semicolon or EOL after most statements, but our tokenizer treats EOL as a token
        if (index < tokens.size() && tokens[index].type == Token::Type::OPERATOR && tokens[index].text == ";") {
            index++; // Consume semicolon
        }
    }

    void Interpreter::ParseAssignmentOrFunctionCall(const std::vector<Token>& tokens, size_t& index, ScriptContext& context, const std::string& script_path) {
        bool is_declaration = false;
        if (tokens[index].type == Token::Type::KEYWORD && tokens[index].text == "let") {
            is_declaration = true;
            index++; // Consume 'let'
        }

        if (index >= tokens.size() || tokens[index].type != Token::Type::IDENTIFIER) {
            SyntaxError("Expected identifier after 'let' or at start of assignment.", script_path, (tokens.empty() ? 0 : tokens[index - 1].line_number));
        }
        std::string var_name = tokens[index].text;
        size_t var_line = tokens[index].line_number;
        index++;

        if (!is_declaration && index < tokens.size() && tokens[index].type == Token::Type::OPERATOR && tokens[index].text == "(") {
            // It was just an identifier, likely a function call statement.
            // Rewind and parse as factor (which handles function calls).
            index -= 1; // Rewind to identifier
            ParseFactor(tokens, index, context, script_path); // This will parse and execute the function call
            return;
        }

        if (index >= tokens.size() || tokens[index].type != Token::Type::OPERATOR || tokens[index].text != "=") {
            SyntaxError("Expected '=' after identifier in assignment.", script_path, var_line);
        }
        index++; // Consume '='

        Value val = ParseExpression(tokens, index, context, script_path);
        context.SetVariable(var_name, val);
    }

    void Interpreter::SkipBlock(const std::vector<Token>& tokens, size_t& index, const std::string& script_path) {
        if (index >= tokens.size() || tokens[index].type != Token::Type::OPERATOR || tokens[index].text != "{") {
            // If not a block, assume single statement and try to skip it.
            // This is tricky without full grammar. For now, just skip until EOL or ';'.
            while (index < tokens.size() && tokens[index].type != Token::Type::END_OF_LINE &&
                !(tokens[index].type == Token::Type::OPERATOR && tokens[index].text == ";")) {
                index++;
            }
            if (index < tokens.size() && (tokens[index].type == Token::Type::END_OF_LINE || (tokens[index].type == Token::Type::OPERATOR && tokens[index].text == ";"))) {
                index++; // consume EOL or ;
            }
            return;
        }

        index++; // Consume '{'
        int brace_level = 1;
        while (index < tokens.size() && brace_level > 0) {
            if (tokens[index].type == Token::Type::OPERATOR && tokens[index].text == "{") {
                brace_level++;
            }
            else if (tokens[index].type == Token::Type::OPERATOR && tokens[index].text == "}") {
                brace_level--;
            }
            index++;
        }
        if (brace_level > 0) {
            SyntaxError("Unterminated block while skipping.", script_path, tokens[index - 1].line_number);
        }
    }

    void Interpreter::ParseBlock(const std::vector<Token>& tokens, size_t& index, ScriptContext& context, const std::string& script_path) {
        if (index >= tokens.size() || tokens[index].type != Token::Type::OPERATOR || tokens[index].text != "{") {
            SyntaxError("Expected '{' to start a block.", script_path, (tokens.empty() ? 0 : tokens[index].line_number));
        }
        index++; // Consume '{'

        while (index < tokens.size() && !(tokens[index].type == Token::Type::OPERATOR && tokens[index].text == "}")) {
            ParseStatement(tokens, index, context, script_path);
            if (index < tokens.size() && tokens[index].type == Token::Type::END_OF_LINE) {
                index++; // Consume EOL within block
            }
        }

        if (index >= tokens.size() || !(tokens[index].type == Token::Type::OPERATOR && tokens[index].text == "}")) {
            SyntaxError("Expected '}' to end a block.", script_path, (tokens.empty() ? 0 : tokens[index - 1].line_number));
        }
        index++; // Consume '}'
    }

    void Interpreter::ParseIfStatement(const std::vector<Token>& tokens, size_t& index, ScriptContext& context, const std::string& script_path) {
        size_t if_line = tokens[index].line_number;
        index++; // Consume 'if'

        if (index >= tokens.size() || tokens[index].type != Token::Type::OPERATOR || tokens[index].text != "(") {
            SyntaxError("Expected '(' after 'if'.", script_path, if_line);
        }
        index++; // Consume '('

        Value condition = ParseExpression(tokens, index, context, script_path);

        if (index >= tokens.size() || tokens[index].type != Token::Type::OPERATOR || tokens[index].text != ")") {
            SyntaxError("Expected ')' after if condition.", script_path, if_line);
        }
        index++; // Consume ')'

        bool condition_true = condition.IsTruthy();
        bool executed_branch = false;

        if (condition_true) {
            executed_branch = true;
            if (tokens[index].type == Token::Type::OPERATOR && tokens[index].text == "{") {
                ParseBlock(tokens, index, context, script_path);
            }
            else { // Single statement
                ParseStatement(tokens, index, context, script_path);
            }
        }
        else {
            // Skip the 'then' block/statement
            if (tokens[index].type == Token::Type::OPERATOR && tokens[index].text == "{") {
                SkipBlock(tokens, index, script_path);
            }
            else { // Single statement
                // A bit naive, just skip one statement (until EOL/;)
                size_t temp_idx = index;
                ParseStatement(tokens, temp_idx, context, script_path); // "Dry run" to advance index past one statement
                index = temp_idx; // This is problematic, SkipBlock is better
            }
        }

        // Handle 'else if' and 'else'
        if (index < tokens.size() && tokens[index].type == Token::Type::KEYWORD && tokens[index].text == "else") {
            index++; // Consume 'else'
            if (index < tokens.size() && tokens[index].type == Token::Type::KEYWORD && tokens[index].text == "if") { // else if
                if (!condition_true) { // Only parse 'else if' if previous conditions were false
                    ParseIfStatement(tokens, index, context, script_path); // Recursive call for 'else if' (consumes the 'if' part)
                }
                else {
                    // Skip the entire 'else if (...){...}' block
                    index++; // consume 'if' for skipping
                    if (index < tokens.size() && tokens[index].text == "(") index++; else SyntaxError("Expected ( for else if skip", script_path, tokens[index - 1].line_number);
                    int paren_level = 1;
                    while (index < tokens.size() && paren_level > 0) {
                        if (tokens[index].text == "(") paren_level++;
                        if (tokens[index].text == ")") paren_level--;
                        index++;
                    }
                    if (paren_level > 0) SyntaxError("Unterminated condition in 'else if' during skip.", script_path, tokens[index - 1].line_number);
                    SkipBlock(tokens, index, script_path);
                }
            }
            else { // 'else' block
                if (!condition_true && !executed_branch) { // Only execute if no prior 'if' or 'else if' was true
                    if (tokens[index].type == Token::Type::OPERATOR && tokens[index].text == "{") {
                        ParseBlock(tokens, index, context, script_path);
                    }
                    else { // Single statement
                        ParseStatement(tokens, index, context, script_path);
                    }
                }
                else {
                    SkipBlock(tokens, index, script_path);
                }
            }
        }
    }

    void Interpreter::ParseWhileStatement(const std::vector<Token>& tokens, size_t& index, ScriptContext& context, const std::string& script_path) {
        size_t while_line = tokens[index].line_number;
        index++; // Consume 'while'

        if (index >= tokens.size() || tokens[index].type != Token::Type::OPERATOR || tokens[index].text != "(") {
            SyntaxError("Expected '(' after 'while'.", script_path, while_line);
        }
        size_t condition_start_index = index; // For re-evaluation
        index++; // Consume '('

        // Loop execution
        while (true) {
            size_t current_eval_index = condition_start_index + 1; // Start after '('
            Value condition = ParseExpression(tokens, current_eval_index, context, script_path);

            if (current_eval_index >= tokens.size() || tokens[current_eval_index].type != Token::Type::OPERATOR || tokens[current_eval_index].text != ")") {
                SyntaxError("Expected ')' after while condition.", script_path, while_line);
            }
            // current_eval_index is now at ')'

            if (!condition.IsTruthy()) {
                index = current_eval_index + 1; // Move main index past condition ')'
                SkipBlock(tokens, index, script_path); // Skip the loop body
                break; // Exit loop
            }

            // Condition is true, execute body
            size_t body_start_index = current_eval_index + 1; // After ')'
            size_t temp_body_index = body_start_index; // Use a temporary index for the body execution

            if (temp_body_index >= tokens.size()) { // Should not happen if ')' was found
                SyntaxError("Unexpected end of script after while condition.", script_path, while_line);
                break;
            }

            if (tokens[temp_body_index].type == Token::Type::OPERATOR && tokens[temp_body_index].text == "{") {
                ParseBlock(tokens, temp_body_index, context, script_path);
            }
            else {
                ParseStatement(tokens, temp_body_index, context, script_path);
            }
            // After body execution, loop back to re-evaluate condition (main 'index' is not changed by body parsing)
        }
    }

    // --- Plugin Namespace Functions ---
    namespace Plugins {

        Value Printf(std::vector<Value>& args) {
            if (args.empty()) return Value();
            printf(args[0].AsString().c_str());
            return Value();
        }

        Value Print(std::vector<Value>& args) {
            for (size_t i = 0; i < args.size(); ++i) {
                std::cout << args[i].AsString();
                if (i < args.size() - 1) {
                    std::cout << " ";
                }
            }
            std::cout << std::endl;
            return Value(); // Print returns nil
        }

        Value LogToFile(std::vector<Value>& args) {
            if (args.empty() || args[0].GetType() != Value::Type::STRING) {
                std::cerr << "LogToFile Error: Requires a string argument for the message." << std::endl;
                return Value();
            }
            std::filesystem::path log_path = BegeerteScript::LogDirectory / "Begeerte_script.log";
            {
                std::lock_guard<std::mutex> lock(LogMutex);
                std::ofstream log_file(log_path, std::ios::app);
                if (log_file.is_open()) {
                    for (const auto& arg : args) {
                        log_file << arg.AsString() << " ";
                    }
                    log_file << std::endl;
                    log_file.close();
                }
                else {
                    std::cerr << "LogToFile Error: Could not open log file: " << log_path << std::endl;
                }
            }
            return Value();
        }

        void RegisterEntityListAPI(ScriptContext& context) {
            // 确保 g_cheatdata 已初始化
            if (!g_cheatdata) {
                std::cerr << "Error: g_cheatdata is null. Cannot register EntityList API." << std::endl;
                return;
            }

            // 注册 EntityList 相关函数
            context.RegisterFunction("EntityList_Update", [](std::vector<Value>& args) -> Value {
                EntityList::Update();
                return Value();
                });

            context.RegisterFunction("EntityList_GetMaxPlayers", [](std::vector<Value>& args) -> Value {
                return Value((long long)EntityList::GetMaxPlayers());
                });

            context.RegisterFunction("EntityList_GetEntity", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("EntityList_GetEntity requires 1 integer argument (id).");
                }
                DWORD64 entity_addr = EntityList::GetEntity(static_cast<int>(args[0].AsInt()));
                return Value(static_cast<long long>(entity_addr));
                });

            context.RegisterFunction("EntityList_GetPlayer", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("EntityList_GetPlayer requires 1 integer argument (id).");
                }
                EntityList::Player* player = EntityList::GetPlayer(static_cast<int>(args[0].AsInt()));
                return Value(player);
                });

            context.RegisterFunction("EntityList_GetAllEntities", [](std::vector<Value>& args) -> Value {
                const auto& entities = EntityList::GetAllEntities();
                return Value((long long)entities.size());
                });

            // 注册 Player 相关函数
            context.RegisterFunction("Player_IsValid", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_IsValid requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value(false);
                return Value(p->IsValid());
                });

            context.RegisterFunction("Player_GetCharacter", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetCharacter requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value("Error: Null Player");
                const wchar_t* w_char = p->GetCharacter();
                char buffer[256];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, buffer, sizeof(buffer), w_char, _TRUNCATE);
                return Value(std::string(buffer));
                });

            context.RegisterFunction("Player_GetGrowthStage", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetGrowthStage requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value("Error: Null Player");
                const wchar_t* w_gs = p->GetGrowthStage();
                char buffer[256];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, buffer, sizeof(buffer), w_gs, _TRUNCATE);
                return Value(std::string(buffer));
                });

            // 为每个 byte 字段注册 Get 和 Set 方法
            // validFlag
            context.RegisterFunction("Player_GetValidFlag", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetValidFlag requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->validFlag);
                });

            context.RegisterFunction("Player_SetValidFlag", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetValidFlag requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->validFlag = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            // SkinIndex
            context.RegisterFunction("Player_GetSkinIndex", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetSkinIndex requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->SkinIndex);
                });

            context.RegisterFunction("Player_SetSkinIndex", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetSkinIndex requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->SkinIndex = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            // Gender
            context.RegisterFunction("Player_GetGenderRaw", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetGenderRaw requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->Gender);
                });

            context.RegisterFunction("Player_SetGender", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetGender requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->Gender = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            // GrowthStage
            context.RegisterFunction("Player_GetGrowthStageRaw", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetGrowthStageRaw requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->GrowthStage);
                });

            context.RegisterFunction("Player_SetGrowthStage", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetGrowthStage requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->GrowthStage = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            // SavedGrowth
            context.RegisterFunction("Player_GetSavedGrowth", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetSavedGrowth requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->SavedGrowth);
                });

            context.RegisterFunction("Player_SetSavedGrowth", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetSavedGrowth requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->SavedGrowth = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            // VitalityHealth
            context.RegisterFunction("Player_GetVitalityHealth", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetVitalityHealth requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->VitalityHealth);
                });

            context.RegisterFunction("Player_SetVitalityHealth", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetVitalityHealth requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->VitalityHealth = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            context.RegisterFunction("Player_GetVitalityHealthGrade", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetVitalityHealthGrade requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value("Error: Null Player");
                const wchar_t* w_grade = p->GetGeneticGrades(p->VitalityHealth);
                char buffer[256];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, buffer, sizeof(buffer), w_grade, _TRUNCATE);
                return Value(std::string(buffer));
                });

            // VitalityArmor
            context.RegisterFunction("Player_GetVitalityArmor", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetVitalityArmor requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->VitalityArmor);
                });

            context.RegisterFunction("Player_SetVitalityArmor", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetVitalityArmor requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->VitalityArmor = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            context.RegisterFunction("Player_GetVitalityArmorGrade", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetVitalityArmorGrade requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value("Error: Null Player");
                const wchar_t* w_grade = p->GetGeneticGrades(p->VitalityArmor);
                char buffer[256];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, buffer, sizeof(buffer), w_grade, _TRUNCATE);
                return Value(std::string(buffer));
                });

            // VitalityBile
            context.RegisterFunction("Player_GetVitalityBile", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetVitalityBile requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->VitalityBile);
                });

            context.RegisterFunction("Player_SetVitalityBile", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetVitalityBile requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->VitalityBile = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            context.RegisterFunction("Player_GetVitalityBileGrade", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetVitalityBileGrade requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value("Error: Null Player");
                const wchar_t* w_grade = p->GetGeneticGrades(p->VitalityBile);
                char buffer[256];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, buffer, sizeof(buffer), w_grade, _TRUNCATE);
                return Value(std::string(buffer));
                });

            // VitalityStamina
            context.RegisterFunction("Player_GetVitalityStamina", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetVitalityStamina requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->VitalityStamina);
                });

            context.RegisterFunction("Player_SetVitalityStamina", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetVitalityStamina requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->VitalityStamina = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            context.RegisterFunction("Player_GetVitalityStaminaGrade", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetVitalityStaminaGrade requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value("Error: Null Player");
                const wchar_t* w_grade = p->GetGeneticGrades(p->VitalityStamina);
                char buffer[256];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, buffer, sizeof(buffer), w_grade, _TRUNCATE);
                return Value(std::string(buffer));
                });

            // VitalityHunger
            context.RegisterFunction("Player_GetVitalityHunger", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetVitalityHunger requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->VitalityHunger);
                });

            context.RegisterFunction("Player_SetVitalityHunger", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetVitalityHunger requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->VitalityHunger = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            context.RegisterFunction("Player_GetVitalityHungerGrade", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetVitalityHungerGrade requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value("Error: Null Player");
                const wchar_t* w_grade = p->GetGeneticGrades(p->VitalityHunger);
                char buffer[256];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, buffer, sizeof(buffer), w_grade, _TRUNCATE);
                return Value(std::string(buffer));
                });

            // VitalityThirst
            context.RegisterFunction("Player_GetVitalityThirst", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetVitalityThirst requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->VitalityThirst);
                });

            context.RegisterFunction("Player_SetVitalityThirst", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetVitalityThirst requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->VitalityThirst = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            context.RegisterFunction("Player_GetVitalityThirstGrade", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetVitalityThirstGrade requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value("Error: Null Player");
                const wchar_t* w_grade = p->GetGeneticGrades(p->VitalityThirst);
                char buffer[256];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, buffer, sizeof(buffer), w_grade, _TRUNCATE);
                return Value(std::string(buffer));
                });

            // VitalityTorpor
            context.RegisterFunction("Player_GetVitalityTorpor", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetVitalityTorpor requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->VitalityTorpor);
                });

            context.RegisterFunction("Player_SetVitalityTorpor", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetVitalityTorpor requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->VitalityTorpor = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            context.RegisterFunction("Player_GetVitalityTorporGrade", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetVitalityTorporGrade requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value("Error: Null Player");
                const wchar_t* w_grade = p->GetGeneticGrades(p->VitalityTorpor);
                char buffer[256];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, buffer, sizeof(buffer), w_grade, _TRUNCATE);
                return Value(std::string(buffer));
                });

            // DamageBite
            context.RegisterFunction("Player_GetDamageBite", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetDamageBite requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->DamageBite);
                });

            context.RegisterFunction("Player_SetDamageBite", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetDamageBite requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->DamageBite = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            context.RegisterFunction("Player_GetDamageBiteGrade", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetDamageBiteGrade requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value("Error: Null Player");
                const wchar_t* w_grade = p->GetGeneticGrades(p->DamageBite);
                char buffer[256];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, buffer, sizeof(buffer), w_grade, _TRUNCATE);
                return Value(std::string(buffer));
                });

            // DamageProjectile
            context.RegisterFunction("Player_GetDamageProjectile", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetDamageProjectile requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->DamageProjectile);
                });

            context.RegisterFunction("Player_SetDamageProjectile", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetDamageProjectile requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->DamageProjectile = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            context.RegisterFunction("Player_GetDamageProjectileGrade", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetDamageProjectileGrade requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value("Error: Null Player");
                const wchar_t* w_grade = p->GetGeneticGrades(p->DamageProjectile);
                char buffer[256];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, buffer, sizeof(buffer), w_grade, _TRUNCATE);
                return Value(std::string(buffer));
                });

            // DamageSwipe
            context.RegisterFunction("Player_GetDamageSwipe", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetDamageSwipe requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->DamageSwipe);
                });

            context.RegisterFunction("Player_SetDamageSwipe", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetDamageSwipe requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->DamageSwipe = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            context.RegisterFunction("Player_GetDamageSwipeGrade", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetDamageSwipeGrade requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value("Error: Null Player");
                const wchar_t* w_grade = p->GetGeneticGrades(p->DamageSwipe);
                char buffer[256];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, buffer, sizeof(buffer), w_grade, _TRUNCATE);
                return Value(std::string(buffer));
                });

            // MitigationBlunt
            context.RegisterFunction("Player_GetMitigationBlunt", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetMitigationBlunt requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->MitigationBlunt);
                });

            context.RegisterFunction("Player_SetMitigationBlunt", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetMitigationBlunt requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->MitigationBlunt = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            context.RegisterFunction("Player_GetMitigationBluntGrade", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetMitigationBluntGrade requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value("Error: Null Player");
                const wchar_t* w_grade = p->GetGeneticGrades(p->MitigationBlunt);
                char buffer[256];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, buffer, sizeof(buffer), w_grade, _TRUNCATE);
                return Value(std::string(buffer));
                });

            // MitigationPierce
            context.RegisterFunction("Player_GetMitigationPierce", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetMitigationPierce requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->MitigationPierce);
                });

            context.RegisterFunction("Player_SetMitigationPierce", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetMitigationPierce requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->MitigationPierce = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            context.RegisterFunction("Player_GetMitigationPierceGrade", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetMitigationPierceGrade requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value("Error: Null Player");
                const wchar_t* w_grade = p->GetGeneticGrades(p->MitigationPierce);
                char buffer[256];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, buffer, sizeof(buffer), w_grade, _TRUNCATE);
                return Value(std::string(buffer));
                });

            // MitigationFire
            context.RegisterFunction("Player_GetMitigationFire", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetMitigationFire requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->MitigationFire);
                });

            context.RegisterFunction("Player_SetMitigationFire", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetMitigationFire requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->MitigationFire = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            context.RegisterFunction("Player_GetMitigationFireGrade", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetMitigationFireGrade requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value("Error: Null Player");
                const wchar_t* w_grade = p->GetGeneticGrades(p->MitigationFire);
                char buffer[256];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, buffer, sizeof(buffer), w_grade, _TRUNCATE);
                return Value(std::string(buffer));
                });

            // MitigationFrost
            context.RegisterFunction("Player_GetMitigationFrost", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetMitigationFrost requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->MitigationFrost);
                });

            context.RegisterFunction("Player_SetMitigationFrost", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetMitigationFrost requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->MitigationFrost = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            context.RegisterFunction("Player_GetMitigationFrostGrade", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetMitigationFrostGrade requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value("Error: Null Player");
                const wchar_t* w_grade = p->GetGeneticGrades(p->MitigationFrost);
                char buffer[256];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, buffer, sizeof(buffer), w_grade, _TRUNCATE);
                return Value(std::string(buffer));
                });

            // MitigationAcid
            context.RegisterFunction("Player_GetMitigationAcid", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetMitigationAcid requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->MitigationAcid);
                });

            context.RegisterFunction("Player_SetMitigationAcid", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetMitigationAcid requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->MitigationAcid = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            context.RegisterFunction("Player_GetMitigationAcidGrade", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetMitigationAcidGrade requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value("Error: Null Player");
                const wchar_t* w_grade = p->GetGeneticGrades(p->MitigationAcid);
                char buffer[256];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, buffer, sizeof(buffer), w_grade, _TRUNCATE);
                return Value(std::string(buffer));
                });

            // MitigationVenom
            context.RegisterFunction("Player_GetMitigationVenom", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetMitigationVenom requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->MitigationVenom);
                });

            context.RegisterFunction("Player_SetMitigationVenom", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetMitigationVenom requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->MitigationVenom = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            context.RegisterFunction("Player_GetMitigationVenomGrade", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetMitigationVenomGrade requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value("Error: Null Player");
                const wchar_t* w_grade = p->GetGeneticGrades(p->MitigationVenom);
                char buffer[256];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, buffer, sizeof(buffer), w_grade, _TRUNCATE);
                return Value(std::string(buffer));
                });

            // MitigationPlasma
            context.RegisterFunction("Player_GetMitigationPlasma", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetMitigationPlasma requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->MitigationPlasma);
                });

            context.RegisterFunction("Player_SetMitigationPlasma", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetMitigationPlasma requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->MitigationPlasma = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            context.RegisterFunction("Player_GetMitigationPlasmaGrade", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetMitigationPlasmaGrade requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value("Error: Null Player");
                const wchar_t* w_grade = p->GetGeneticGrades(p->MitigationPlasma);
                char buffer[256];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, buffer, sizeof(buffer), w_grade, _TRUNCATE);
                return Value(std::string(buffer));
                });

            // MitigationElectricity
            context.RegisterFunction("Player_GetMitigationElectricity", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetMitigationElectricity requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->MitigationElectricity);
                });

            context.RegisterFunction("Player_SetMitigationElectricity", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetMitigationElectricity requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->MitigationElectricity = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            context.RegisterFunction("Player_GetMitigationElectricityGrade", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetMitigationElectricityGrade requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value("Error: Null Player");
                const wchar_t* w_grade = p->GetGeneticGrades(p->MitigationElectricity);
                char buffer[256];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, buffer, sizeof(buffer), w_grade, _TRUNCATE);
                return Value(std::string(buffer));
                });

            // OverallQuality
            context.RegisterFunction("Player_GetOverallQuality", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetOverallQuality requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->OverallQuality);
                });

            context.RegisterFunction("Player_SetOverallQuality", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetOverallQuality requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->OverallQuality = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            context.RegisterFunction("Player_GetOverallQualityGrade", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetOverallQualityGrade requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value("Error: Null Player");
                const wchar_t* w_grade = p->GetGeneticGrades(p->OverallQuality);
                char buffer[256];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, buffer, sizeof(buffer), w_grade, _TRUNCATE);
                return Value(std::string(buffer));
                });

            // Character
            context.RegisterFunction("Player_GetCharacterRaw", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetCharacterRaw requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->Character);
                });

            context.RegisterFunction("Player_SetCharacter", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetCharacter requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->Character = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            // Health
            context.RegisterFunction("Player_GetHealth", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetHealth requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value((long long)0);
                return Value((long long)p->Health);
                });

            context.RegisterFunction("Player_SetHealth", [](std::vector<Value>& args) -> Value {
                if (args.size() != 2 || args[0].GetType() != Value::Type::PLAYER_PTR || args[1].GetType() != Value::Type::NUMBER_INT) {
                    throw std::runtime_error("Player_SetHealth requires 1 Player object and 1 integer argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value();
                p->Health = static_cast<byte>(args[1].AsInt());
                return Value();
                });

            context.RegisterFunction("Player_GetHealthGrade", [](std::vector<Value>& args) -> Value {
                if (args.size() != 1 || args[0].GetType() != Value::Type::PLAYER_PTR) {
                    throw std::runtime_error("Player_GetHealthGrade requires 1 Player object argument.");
                }
                EntityList::Player* p = args[0].AsPlayer();
                if (!p) return Value("Error: Null Player");
                const wchar_t* w_grade = p->GetGeneticGrades(p->Health);
                char buffer[256];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, buffer, sizeof(buffer), w_grade, _TRUNCATE);
                return Value(std::string(buffer));
                });
        }

        // Structure to hold script execution data
        struct ScriptTask {
            std::string path;
            std::string content;
            bool executed = false;
            std::string error;
        };

        void Init() {
            std::cout << "[BegeerteScript] Initializing..." << std::endl;

            // Get the root directory of the current process
            char path_buffer[MAX_PATH];
            GetModuleFileNameA(NULL, path_buffer, MAX_PATH); // Gets EXE path
            std::filesystem::path root_dir = std::filesystem::path(path_buffer).parent_path();

            // Set script and log directories
            ScriptDirectory = root_dir / "Begeerte" / "Scripts";
            LogDirectory = root_dir / "Begeerte" / "Logs";

            // Create directories if they don't exist
            std::filesystem::create_directories(ScriptDirectory);
            std::filesystem::create_directories(LogDirectory);

            std::cout << "[BegeerteScript] Scanning for .beg files in: " << ScriptDirectory << std::endl;

            // Collect all script tasks
            std::vector<ScriptTask> tasks;
            for (const auto& entry : std::filesystem::directory_iterator(ScriptDirectory)) {
                if (entry.is_regular_file() && entry.path().extension() == ".beg") {
                    std::string script_path_str = entry.path().string();
                    std::cout << "[BegeerteScript] Found script: " << script_path_str << std::endl;

                    std::ifstream script_file(entry.path());
                    if (!script_file.is_open()) {
                        std::cerr << "[BegeerteScript] Error: Could not open script file: " << script_path_str << std::endl;
                        continue;
                    }

                    std::stringstream script_buffer;
                    script_buffer << script_file.rdbuf();
                    script_file.close();
                    std::string script_content = script_buffer.str();

                    tasks.emplace_back(ScriptTask{ script_path_str, script_content });
                }
            }

            std::cout << "[BegeerteScript] Total scripts found: " << tasks.size() << std::endl;

            // Create a separate thread for each script
            for (auto& task : tasks) {
                std::thread script_thread([task]() {  // Capture task by value
                    std::cout << "[BegeerteScript] Loading script: " << std::filesystem::path(task.path).filename() << std::endl;
                    Interpreter interpreter;
                    ScriptContext context(task.path);
                    // Register common functions
                    context.RegisterFunction("printf", Printf);
                    context.RegisterFunction("print", Print);
                    context.RegisterFunction("LogToFile", LogToFile);
                    // Register EntityList API for this script
                    RegisterEntityListAPI(context);

                    if (!g_cheatdata) {
                        std::string error = "[BegeerteScript] FATAL: g_cheatdata not initialized. Aborting script: " + task.path;
                        std::cerr << error << std::endl;
                        // Log to file
                        {
                            std::lock_guard<std::mutex> lock(LogMutex);
                            std::ofstream log_file(LogDirectory / "Begeerte_script.log", std::ios::app);
                            if (log_file.is_open()) {
                                log_file << error << std::endl;
                                log_file.close();
                            }
                        }
                        return;
                    }

                    try {
                        interpreter.Execute(task.content, context);
                        std::cout << "[BegeerteScript] Finished executing: " << std::filesystem::path(task.path).filename() << std::endl;
                    }
                    catch (const std::exception& e) {
                        std::string error = "[BegeerteScript] Unhandled exception in " + std::filesystem::path(task.path).filename().string() + ": " + e.what();
                        std::cerr << error << std::endl;
                        // Log to file
                        {
                            std::lock_guard<std::mutex> lock(LogMutex);
                            std::ofstream log_file(LogDirectory / "Begeerte_script.log", std::ios::app);
                            if (log_file.is_open()) {
                                log_file << error << std::endl;
                                log_file.close();
                            }
                        }
                    }
                    });
                script_thread.detach();  // Detach the thread to run independently
            }

            std::cout << "[BegeerteScript] Initialization complete. Scripts are running in separate threads." << std::endl;
        }

    } // namespace Plugins
} // namespace BegeerteScript