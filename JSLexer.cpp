#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <cctype>

enum class TokenType {
    Keyword, Identifier, Number,
    Operator, String, Comment, Punctuation,
    EndOfFile
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line, column;
};

class Lexer {
    std::string input;
    size_t pos = 0;
    int line = 1, col = 1;
    std::unordered_set<std::string> keywords = {
        "var", "if", "else", "function", "return", "let", "const", "while"
    };

public:
    Lexer(const std::string& src) : input(src) {}

    std::vector<Token> tokenize(bool trace = false) {
        std::vector<Token> tokens;

        while (pos < input.size()) {
            while (pos < input.size() && isspace(input[pos])) {
                if (input[pos] == '\n') { ++line; col = 1; }
                else ++col;
                ++pos;
            }

            if (pos >= input.size()) break;

            char c = input[pos];

            if ((c == '+' || c == '-') && pos + 1 < input.size() && isdigit(input[pos + 1])) {
                tokens.push_back(readNumber(trace));
            } else if (isdigit(c)) {
                tokens.push_back(readNumber(trace));
            } else if (isalpha(c) || c == '_' || c == '$') {
                tokens.push_back(readIdentifier(trace));
            }else if (c == '"' || c == '\'') {
                tokens.push_back(readString(trace));
            } else if (c == '/' && pos + 1 < input.size() && (input[pos + 1] == '/' || input[pos + 1] == '*')) {
                tokens.push_back(readComment(trace));
            } else if (isOperatorChar(c)) {
                tokens.push_back(readOperator(trace));
            } else if (isPunctuation(c)) {
                tokens.push_back(readPunctuation(trace));
            }else {
                ++pos;
                ++col;
            }
        }

        tokens.push_back({ TokenType::EndOfFile, "", line, col });
        return tokens;
    }

private:
    Token readIdentifier(bool trace) {
        std::string buf;
        int startCol = col;

        while (pos < input.size() &&
               (isalnum(input[pos]) || input[pos] == '_' || input[pos] == '$')) {
            buf += input[pos++];
            ++col;
        }
        TokenType type = keywords.count(buf) ? TokenType::Keyword : TokenType::Identifier;
        Token t = { type, buf, line, startCol };
        if (trace) printToken(t);
        return t;
    }
    Token readNumber(bool trace) {
        enum class State {
            START, SIGN, ZERO, INT_PART, DOT, FRAC_PART,
            EXP, EXP_SIGN, EXP_NUM, ACCEPT, ERROR
        };

        State state = State::START;
        std::string buf;
        int startCol = col;

        while (pos < input.size()) {
            char c = input[pos];
            switch (state) {
                case State::START:
                    if (c == '+' || c == '-') {
                        buf += c; pos++; col++;
                        state = State::SIGN;
                    } else if (c == '0') {
                        buf += c; pos++; col++;
                        state = State::ZERO;
                    } else if (isdigit(c)) {
                        buf += c; pos++; col++;
                        state = State::INT_PART;
                    } else {
                        goto done;
                    }
                    break;

                case State::SIGN:
                    if (c == '0') {
                        buf += c; pos++; col++;
                        state = State::ZERO;
                    } else if (isdigit(c)) {
                        buf += c; pos++; col++;
                        state = State::INT_PART;
                    } else {
                        throw std::runtime_error("Malformed number at line " + std::to_string(line) + ", column " + std::to_string(startCol));
                    }
                    break;

                case State::ZERO:
                    if (isdigit(c)) {
                        throw std::runtime_error("Invalid number: leading zeros not allowed at line " + std::to_string(line) + ", col " + std::to_string(startCol));
                    } else if (c == '.') {
                        buf += c; pos++; col++;
                        state = State::DOT;
                    } else if (c == 'e' || c == 'E') {
                        buf += c; pos++; col++;
                        state = State::EXP;
                    } else {
                        state = State::ACCEPT;
                    }
                    break;

                case State::INT_PART:
                    if (isdigit(c)) {
                        buf += c; pos++; col++;
                    } else if (c == '.') {
                        buf += c; pos++; col++;
                        state = State::DOT;
                    } else if (c == 'e' || c == 'E') {
                        buf += c; pos++; col++;
                        state = State::EXP;
                    } else {
                        state = State::ACCEPT;
                    }
                    break;

                case State::DOT:
                    if (isdigit(c)) {
                        buf += c; pos++; col++;
                        state = State::FRAC_PART;
                    } else {
                        throw std::runtime_error("Malformed number at line " + std::to_string(line) + ", column " + std::to_string(startCol));
                    }
                    break;

                case State::FRAC_PART:
                    if (isdigit(c)) {
                        buf += c; pos++; col++;
                    } else if (c == 'e' || c == 'E') {
                        buf += c; pos++; col++;
                        state = State::EXP;
                    } else {
                        state = State::ACCEPT;
                    }
                    break;

                case State::EXP:
                    if (c == '+' || c == '-') {
                        buf += c; pos++; col++;
                        state = State::EXP_SIGN;
                    } else if (isdigit(c)) {
                        buf += c; pos++; col++;
                        state = State::EXP_NUM;
                    } else {
                        throw std::runtime_error("Malformed exponent at line " + std::to_string(line) + ", column " + std::to_string(startCol));
                    }
                    break;

                case State::EXP_SIGN:
                    if (isdigit(c)) {
                        buf += c; pos++; col++;
                        state = State::EXP_NUM;
                    } else {
                        throw std::runtime_error("Malformed exponent at line " + std::to_string(line) + ", column " + std::to_string(startCol));
                    }
                    break;

                case State::EXP_NUM:
                    if (isdigit(c)) {
                        buf += c; pos++; col++;
                    } else {
                        state = State::ACCEPT;
                    }
                    break;

                case State::ACCEPT:
                    goto done;
            }
        }

    done:
        if (pos < input.size() && (isalnum(input[pos]) || input[pos] == '_' || input[pos] == '$')) {
            throw std::runtime_error("Invalid token: '" + buf + input[pos] + "' at line " + std::to_string(line) + ", col " + std::to_string(startCol));
        }
        Token t = { TokenType::Number, buf, line, startCol };
        if (trace) printToken(t);
        return t;
    }
    
    Token readString(bool trace) {
        std::string buf;
        int startCol = col;
        char quote = input[pos++];
        col++;
        bool closed = false;

        while (pos < input.size()) {
            char c = input[pos++];
            col++;

            if (c == '\n') break;
            if (c == quote) {
                closed = true;
                break;
            }
            if (c == '\\' && pos < input.size()) {
                buf += c;
                buf += input[pos++];
                col++;
            } else {
                buf += c;
            }
        }

        if (!closed) {
            throw std::runtime_error("Unterminated string at line " + std::to_string(line) + ", col " + std::to_string(startCol));
        }
        Token t = { TokenType::String, buf, line, startCol };
        if (trace) printToken(t);
        return t;
    }

    Token readComment(bool trace) {
        std::string buf;
        int startCol = col;

        pos++; col++;

        if (input[pos] == '/') {
            pos++; col++;
            while (pos < input.size() && input[pos] != '\n') {
                buf += input[pos++];
                col++;
            }
        } else if (input[pos] == '*') {
            pos++; col++;
            bool closed = false;

            while (pos + 1 < input.size()) {
                if (input[pos] == '*' && input[pos + 1] == '/') {
                    closed = true;
                    break;
                }
                if (input[pos] == '\n') { ++line; col = 1; }
                else ++col;
                buf += input[pos++];
            }

            if (!closed) {
                throw std::runtime_error("Unterminated comment at line " + std::to_string(line) + ", col " + std::to_string(startCol));
            }

            pos += 2; col += 2;
        }
        Token t = { TokenType::Comment, buf, line, startCol };
        if (trace) printToken(t);
        return t;

    }

    Token readOperator(bool trace) {
        std::string buf;
        int startCol = col;

        auto peek = [&](int offset = 0) -> char {
            return (pos + offset < input.size()) ? input[pos + offset] : '\0';
        };

        auto advance = [&]() {
            buf += input[pos];
            pos++; col++;
        };

        char c = peek();
        switch (c) {
            case '=': advance(); if (peek() == '=') advance(); if (peek() == '=') advance(); break;
            case '!': advance(); if (peek() == '=') advance(); if (peek() == '=') advance(); break;
            case '<': advance(); if (peek() == '<') advance(); if (peek() == '=') advance(); break;
            case '>': advance(); if (peek() == '>') advance(); if (peek() == '>') advance(); if (peek() == '=') advance(); break;
            case '&': advance(); if (peek() == '&') advance(); break;
            case '|': advance(); if (peek() == '|') advance(); break;
            case '+': case '-': case '*': case '/': case '%': case '^':
                advance(); if (peek() == '=') advance(); break;
            default:
                throw std::runtime_error("Unknown operator at line " + std::to_string(line) + ", column " + std::to_string(col));
        }

        Token t = { TokenType::Operator, buf, line, startCol };
        if (trace) printToken(t);
        return t;
    }

    Token readPunctuation(bool trace) {
        Token t = { TokenType::Punctuation, std::string(1, input[pos]), line, col };
        pos++; col++;
        return t;
    }

    bool isOperatorChar(char c) {
        return std::string("+-*/=<>!&|^%").find(c) != std::string::npos;
    }

    bool isPunctuation(char c) {
        return std::string("(){}[],;.").find(c) != std::string::npos;
    }

        void printToken(const Token& t) {
        std::string type;
        switch (t.type) {
            case TokenType::Keyword: type = "KW"; break;
            case TokenType::Identifier: type = "ID"; break;
            case TokenType::Number: type = "NUM"; break;
            case TokenType::Operator: type = "OP"; break;
            case TokenType::String: type = "STR"; break;
            case TokenType::Comment: type = "CMT"; break;
            case TokenType::Punctuation: type = "PUN"; break;
            case TokenType::EndOfFile: type = "EOF"; break;
            default: type = "UNK";
        }
        std::cout << "[" << t.line << ":" << t.column << "] " << type << " '" << t.lexeme << "'\n";
    }
};

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::Keyword: return "KW";
        case TokenType::Identifier: return "ID";
        case TokenType::Number: return "NUM";
         case TokenType::Operator: return "OP";
        case TokenType::String: return "STR";
        case TokenType::Comment: return "CMT";
        case TokenType::Punctuation: return "PUN";
        case TokenType::EndOfFile: return "EOF";
        default: return "UNK";
    }
}

int main() {
while (true) {
        std::string code, mode;
        bool trace = false;

        std::cout << "\n=== JavaScript Lexer ===\n";
        std::cout << "Select mode:\n";
        std::cout << "1 = Manual input\n";
        std::cout << "2 = Trace mode input\n";
        std::cout << "3 = Demo\n";
        std::cout << "4 = Exit\n";
        std::cout << "> ";
        std::getline(std::cin, mode);

        if (mode == "4") {
            std::cout << "Exiting...\n";
            break;
        }

        if (mode == "1" || mode == "2") {
            trace = (mode == "2");
            std::cout << "Enter code (2 empty lines to end):\n";
            std::string line;
            int empty = 0;

            while (std::getline(std::cin, line)) {
                if (line.empty()) {
                    if (++empty == 2) break;
                } else {
                    empty = 0;
                    code += line + "\n";
                }
            }
        } else if (mode == "3") {
            trace = true;
            code = R"(
        let x = -12.5e+3;
        const y = 42; // comment
        /* multi
           line */
        if (x >= 0 && x !== y) {
            console.log("ok");
        }
    )";
        }else {
            std::cout << "Invalid option. Please choose 1–4.\n";
            continue;
        }
    try {
        std::cout << "\n--- Source Code ---\n" << code << "\n";
        Lexer lexer(code);
        std::vector<Token> tokens = lexer.tokenize(trace);
    } catch (const std::runtime_error& err) {
            std::cerr << "[ERROR] " << err.what() << "\n";
        } catch (...) {
            std::cerr << "[ERROR] Unknown lexer failure.\n";
        }
    }

    return 0;
}
