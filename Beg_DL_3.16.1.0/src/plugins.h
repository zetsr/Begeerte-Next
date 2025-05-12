#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <variant>
#include <stdexcept>
#include <filesystem> // For C++17 filesystem operations
#include <iostream>   // For basic error logging
#include <fstream>
#include <sstream>

// Forward declaration for classes within the namespace
namespace BegeerteScript {
    class Value;
    class ScriptContext;
    class Interpreter;
}

// Include your project's headers
#include "EntityList.h" // This will include Windows.h, Vector.h, SDK.h, CheatData.h

namespace BegeerteScript {

    // Represents a script value (can be number, string, boolean, Player*, or null)
    class Value {
    public:
        enum class Type { NIL, BOOL, NUMBER_INT, NUMBER_FLOAT, STRING, PLAYER_PTR, NATIVE_FUNCTION };
        Type type;
        std::variant<std::nullptr_t, bool, long long, double, std::string, EntityList::Player*> value;

        // Store native function pointers if needed, though direct call is simpler for now
        // std::function<Value(std::vector<Value>)> native_func;


        Value() : type(Type::NIL), value(nullptr) {}
        Value(bool b) : type(Type::BOOL), value(b) {}
        Value(int i) : type(Type::NUMBER_INT), value((long long)i) {}
        Value(long long i) : type(Type::NUMBER_INT), value(i) {}
        Value(double d) : type(Type::NUMBER_FLOAT), value(d) {}
        Value(const char* s) : type(Type::STRING), value(std::string(s)) {}
        Value(const std::string& s) : type(Type::STRING), value(s) {}
        Value(EntityList::Player* p) : type(Type::PLAYER_PTR), value(p) {}

        Type GetType() const { return type; }

        bool IsNil() const { return type == Type::NIL; }
        bool IsTruthy() const {
            if (type == Type::NIL) return false;
            if (type == Type::BOOL) return std::get<bool>(value);
            if (type == Type::NUMBER_INT) return std::get<long long>(value) != 0;
            if (type == Type::NUMBER_FLOAT) return std::get<double>(value) != 0.0;
            if (type == Type::PLAYER_PTR) return std::get<EntityList::Player*>(value) != nullptr;
            return true; // Strings are truthy if not empty, but for simplicity now, all non-nil are truthy
        }

        bool AsBool() const {
            if (type == Type::BOOL) return std::get<bool>(value);
            throw std::runtime_error("Value is not a boolean");
        }
        long long AsInt() const {
            if (type == Type::NUMBER_INT) return std::get<long long>(value);
            if (type == Type::NUMBER_FLOAT) return static_cast<long long>(std::get<double>(value));
            throw std::runtime_error("Value is not an integer");
        }
        double AsFloat() const {
            if (type == Type::NUMBER_FLOAT) return std::get<double>(value);
            if (type == Type::NUMBER_INT) return static_cast<double>(std::get<long long>(value));
            throw std::runtime_error("Value is not a float");
        }
        std::string AsString() const {
            if (type == Type::STRING) return std::get<std::string>(value);
            if (type == Type::NUMBER_INT) return std::to_string(std::get<long long>(value));
            if (type == Type::NUMBER_FLOAT) return std::to_string(std::get<double>(value));
            if (type == Type::BOOL) return std::get<bool>(value) ? "true" : "false";
            if (type == Type::NIL) return "nil";
            if (type == Type::PLAYER_PTR) {
                std::stringstream ss;
                ss << "Player@0x" << std::hex << reinterpret_cast<uintptr_t>(std::get<EntityList::Player*>(value));
                return ss.str();
            }
            throw std::runtime_error("Cannot convert value to string");
        }
        EntityList::Player* AsPlayer() const {
            if (type == Type::PLAYER_PTR) return std::get<EntityList::Player*>(value);
            throw std::runtime_error("Value is not a Player pointer");
        }

        // For debugging
        std::string ToString() const {
            switch (type) {
            case Type::NIL: return "nil";
            case Type::BOOL: return AsString();
            case Type::NUMBER_INT: return AsString();
            case Type::NUMBER_FLOAT: return AsString();
            case Type::STRING: return "\"" + AsString() + "\"";
            case Type::PLAYER_PTR: return AsString();
            default: return "Unknown Value Type";
            }
        }
    };

    // Using a type alias for native functions exposed to the script
    using NativeFunction = std::function<Value(std::vector<Value>& args)>;

    class ScriptContext {
    public:
        std::map<std::string, Value> variables;
        std::map<std::string, NativeFunction> functions;
        std::string current_script_path; // For error reporting

        ScriptContext(const std::string& script_path = "") : current_script_path(script_path) {}

        void RegisterFunction(const std::string& name, NativeFunction func) {
            functions[name] = func;
        }

        void SetVariable(const std::string& name, const Value& val) {
            variables[name] = val;
        }

        Value GetVariable(const std::string& name) {
            if (variables.count(name)) {
                return variables[name];
            }
            // Could also throw an error or return Value() (nil)
            std::cerr << "Runtime Error in '" << current_script_path << "': Variable '" << name << "' not found." << std::endl;
            return Value();
        }

        bool HasFunction(const std::string& name) const {
            return functions.count(name);
        }

        Value CallFunction(const std::string& name, std::vector<Value>& args) {
            if (functions.count(name)) {
                try {
                    return functions[name](args);
                }
                catch (const std::exception& e) {
                    std::cerr << "Runtime Error in '" << current_script_path
                        << "' calling function '" << name << "': " << e.what() << std::endl;
                    return Value(); // Return nil on error
                }
            }
            std::cerr << "Runtime Error in '" << current_script_path << "': Function '" << name << "' not found." << std::endl;
            return Value(); // Return nil
        }
    };


    class Interpreter {
    public:
        void Execute(const std::string& script_content, ScriptContext& context);

    private:
        // Basic tokenizer and parser helper (very simplified)
        struct Token {
            enum class Type { IDENTIFIER, NUMBER, STRING, OPERATOR, KEYWORD, END_OF_LINE, UNKNOWN, END_OF_FILE };
            Type type;
            std::string text;
            size_t line_number = 0; // For error reporting
        };

        std::vector<Token> Tokenize(const std::string& script_content, const std::string& script_path);
        Value ParseExpression(const std::vector<Token>& tokens, size_t& index, ScriptContext& context, const std::string& script_path);
        Value ParseTerm(const std::vector<Token>& tokens, size_t& index, ScriptContext& context, const std::string& script_path);
        Value ParseFactor(const std::vector<Token>& tokens, size_t& index, ScriptContext& context, const std::string& script_path);
        std::vector<Value> ParseArgumentList(const std::vector<Token>& tokens, size_t& index, ScriptContext& context, const std::string& script_path);

        // Statement parsing
        void ParseStatement(const std::vector<Token>& tokens, size_t& index, ScriptContext& context, const std::string& script_path);
        void ParseAssignmentOrFunctionCall(const std::vector<Token>& tokens, size_t& index, ScriptContext& context, const std::string& script_path);
        void ParseIfStatement(const std::vector<Token>& tokens, size_t& index, ScriptContext& context, const std::string& script_path);
        void ParseWhileStatement(const std::vector<Token>& tokens, size_t& index, ScriptContext& context, const std::string& script_path);
        void ParseBlock(const std::vector<Token>& tokens, size_t& index, ScriptContext& context, const std::string& script_path);
        // Helper to skip tokens until a certain point, useful for skipping blocks in if/while
        void SkipBlock(const std::vector<Token>& tokens, size_t& index, const std::string& script_path);


        // Error reporting
        void SyntaxError(const std::string& message, const std::string& script_path, size_t line_number = 0) {
            std::cerr << "Syntax Error in '" << script_path << "'";
            if (line_number > 0) {
                std::cerr << " (Line " << line_number << ")";
            }
            std::cerr << ": " << message << std::endl;
            throw std::runtime_error("Syntax error occurred."); // Stop execution
        }
    };


    namespace Plugins {
        // Initializes the scripting system, loads and executes all .beg scripts.
        void Init();

        // Registers all EntityList related functions to a given script context
        void RegisterEntityListAPI(ScriptContext& context);

        // A simple utility function to be exposed to script
        Value Print(std::vector<Value>& args);
        Value LogToFile(std::vector<Value>& args); // Example: LogToFile("message")

    } // namespace Plugins
} // namespace BegeerteScript