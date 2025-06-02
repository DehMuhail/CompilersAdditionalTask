#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <cctype>

// This is the list of token types the lexer can find. I chose enum class so names do not collide.
// Each type tells how to handle the slice of code later in the parser.
// For example, if this is a Keyword, we know it is a reserved word.
enum class TokenType {
    Keyword, Identifier, Number,
    Operator, String, Comment, Punctuation,
    EndOfFile
};


// The Token struct stores the token’s type, the lexeme) and its position (line and column).
// I added line and column so it is easier to report errors with exact location.
// This helps debugging code and printing error messages.
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
    // Constructor takes the entire input code as a single string and stores it.
    // I do this such way so lexer be able to read characters one by one later.
    // The fields pos, line and col are initialized here to start at the beginning.
    Lexer(const std::string& src) : input(src) {}

    // I create a vector tokens to collect all found tokens.
    std::vector<Token> tokenize(bool trace = false) {
        std::vector<Token> tokens;
        
        // Here I skip whitespace, tabs, and newline characters.
        // When lexer see '\n', it increase the line counter and reset column to 1.
        // This is needed so programm correctly track where it is in the file.
        while (pos < input.size()) {
            while (pos < input.size() && isspace(input[pos])) {
                if (input[pos] == '\n') { ++line; col = 1; }
                else ++col;
                ++pos;
            }
            // Check again, because after skipping whitespace programm can be at the end.
            if (pos >= input.size()) break;
            // Reads the current character to decide what to do next.
            char c = input[pos];
            
            // Here programm check each possible token start:
            // 1) If we see '+' or '-' followed by a digit, start reading a signed number.
            // 2) If just a digit, read a number.
            // 3) If a letter or '_' or '$', read an identifier (or keyword).
            // 4) If see a quote, I read a string literal.
            // 5) If  see '/' next to '/' or '*', read a comment.
            // 6) If it is an operator character, call readOperator.
            // 7) If it is punctuation (brackets, commas, semicolons), call readPunctuation.
            // Otherwise just go on to avoid getting stuck on unknown character.
            if ((c == '+' || c == '-') && pos + 1 < input.size() && isdigit(input[pos + 1])) {
                tokens.push_back(readNumberFA(trace));
            } else if (isdigit(c)) {
                tokens.push_back(readNumberFA(trace));
            } else if (isalpha(c) || c == '_' || c == '$') {
                tokens.push_back(readIdentifierFA(trace));
            }else if (c == '"' || c == '\'') {
                tokens.push_back(readStringFA(trace));
            } else if (c == '/' && pos + 1 < input.size() && (input[pos + 1] == '/' || input[pos + 1] == '*')) {
                tokens.push_back(readCommentFA(trace));
            } else if (isOperatorChar(c)) {
                tokens.push_back(readOperatorFA(trace));
            } else if (isPunctuation(c)) {
                tokens.push_back(readPunctuation(trace));
            }else {
                ++pos;
                ++col;
            }
        }
        // After finishing all checks, push an EndOfFile token.
        tokens.push_back({ TokenType::EndOfFile, "", line, col });
        return tokens;
    }

private:
    // This function reads identifiers and keywords using a small DFA.
    // We use states: START (before reading), IDENT (reading letters/digits/_/$), ACCEPT (done).
    Token readIdentifierFA(bool trace) {
        enum class State { START, IDENT, ACCEPT };
        State state = State::START;
        std::string buf;
        int startCol = col;

        while (pos < input.size()) {
            char c = input[pos];
            switch (state) {
                case State::START:
                    // In START, we expect a letter, '_' or '$' to begin identifier.
                    // If it matches, we add to buf, move pos, update col, go to IDENT.
                    if (isalpha(c) || c == '_' || c == '$') {
                        buf += c;
                        pos++;
                        col++;
                        state = State::IDENT;
                    } else {
                        // If not valid start char, go to ACCEPT (should not happen normally).
                        state = State::ACCEPT;
                    }
                    break;

                case State::IDENT:
                    // In IDENT, we accept letters, digits, '_' and '$'.
                    // If char matches, add to buf and continue in IDENT.
                    if (isalnum(c) || c == '_' || c == '$') {
                        buf += c;
                        pos++;
                        col++;
                    } else {
                        // When next char is not valid, go to ACCEPT.
                        state = State::ACCEPT;
                    }
                    break;

                case State::ACCEPT:
                    // We break out when we reach ACCEPT.
                    goto done;
            }
        }

    done:
        // After loop, buf holds full lexeme. Check if it's a keyword.
        TokenType type = keywords.count(buf) ? TokenType::Keyword : TokenType::Identifier;
        Token t = { type, buf, line, startCol };
        if (trace) printToken(t);
        return t;
    }
    // In this function, I implement a determined finite automaton for numbers.
    // First programm handle the optional sign (SIGN), then various parts: ZERO, INT_PART (integer part), DOT and FRAC_PART (fraction).
    // Then it handle the exponent (EXP, EXP_SIGN, EXP_NUM). When all characters are read,afther that go to ACCEPT.
    // This ensures the number has the correct format, for example no leading zeros.
    // If it see an invalid character (like a letter after digits) throw an error.
    Token readNumberFA(bool trace) {
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
    // This function reads string literals using a DFA.
    // States: START (see opening quote), IN_STRING (reading content), ESCAPE (after '\'), ACCEPT (closing).
    Token readStringFA(bool trace) {
        enum class State { START, IN_STRING, ESCAPE, ACCEPT };
        State state = State::START;
        std::string buf;
        int startCol = col;
        char quote = input[pos];  // store '"' or '\''

        // In START, we consume the opening quote and move to IN_STRING.
        buf += quote;
        pos++;
        col++;

        while (pos < input.size()) {
            char c = input[pos];
            switch (state) {
                case State::START:
                    // After consuming opening quote, go to IN_STRING.
                    state = State::IN_STRING;
                    break;

                case State::IN_STRING:
                    if (c == '\\') {
                        // If backslash, it's escape start. Add to buf and go to ESCAPE.
                        buf += c;
                        pos++;
                        col++;
                        state = State::ESCAPE;
                    }
                    else if (c == quote) {
                        // If we see matching quote, add and accept string.
                        buf += c;
                        pos++;
                        col++;
                        state = State::ACCEPT;
                    }
                    else if (c == '\n') {
                        // Newline inside string without closing - error.
                        throw std::runtime_error(
                            "Unterminated string at line " + std::to_string(line) +
                            ", col " + std::to_string(startCol)
                        );
                    }
                    else {
                        // Regular character, add and stay in IN_STRING.
                        buf += c;
                        pos++;
                        col++;
                    }
                    break;

                case State::ESCAPE:
                    // After '\' consume next char regardless of what it is.
                    buf += c;
                    pos++;
                    col++;
                    // Back to IN_STRING to continue.
                    state = State::IN_STRING;
                    break;

                case State::ACCEPT:
                    // Once closing quote found, break out.
                    goto done;
            }
        }

    done:
        if (state != State::ACCEPT) {
            // If EOF reached without closing quote.
            throw std::runtime_error(
                "Unterminated string at line " + std::to_string(line) +
                ", col " + std::to_string(startCol)
            );
        }
        Token t = { TokenType::String, buf, line, startCol };
        if (trace) printToken(t);
        return t;
    }

    
     // This function reads comments (single-line // or multi-line /* */) using a DFA.
    // States: START (we saw '/'), SLASH (decide / or *), SINGLE (in // comment), MULTI (in /* comment), STAR (saw '*' inside multi), ACCEPT (end).
    Token readCommentFA(bool trace) {
        enum class State { START, SLASH, SINGLE, MULTI, STAR, ACCEPT };
        State state = State::START;
        std::string buf;
        int startCol = col;

        // We know input[pos] == '/', so add and move to SLASH state.
        buf += input[pos];
        pos++;
        col++;
        state = State::SLASH;

        while (pos < input.size()) {
            char c = input[pos];
            switch (state) {
                case State::SLASH:
                    if (c == '/') {
                        // This is "//" start. Add and go to SINGLE state.
                        buf += c;
                        pos++;
                        col++;
                        state = State::SINGLE;
                    }
                    else if (c == '*') {
                        // This is "/*" start. Add and go to MULTI state.
                        buf += c;
                        pos++;
                        col++;
                        state = State::MULTI;
                    }
                    else {
                        // Not a comment after '/', treat '/' alone as operator error.
                        throw std::runtime_error(
                            "Unexpected '/' at line " + std::to_string(line) +
                            ", col " + std::to_string(startCol)
                        );
                    }
                    break;

                case State::SINGLE:
                    // In single-line, read until newline or EOF.
                    if (c == '\n') {
                        // Do not include newline in comment content.
                        state = State::ACCEPT;
                    } else {
                        buf += c;
                        pos++;
                        col++;
                    }
                    break;

                case State::MULTI:
                    if (c == '*') {
                        // Saw '*', might be end "*/". Go to STAR state.
                        buf += c;
                        pos++;
                        col++;
                        state = State::STAR;
                    }
                    else {
                        // Normal character inside multi-line. Track newlines.
                        if (c == '\n') {
                            buf += c;
                            pos++;
                            line++;
                            col = 1;
                        } else {
                            buf += c;
                            pos++;
                            col++;
                        }
                        // Stay in MULTI.
                    }
                    break;

                case State::STAR:
                    if (c == '/') {
                        // Found closing "*/". Add and accept.
                        buf += c;
                        pos++;
                        col++;
                        state = State::ACCEPT;
                    }
                    else if (c == '*') {
                        // Another '*' still inside. Stay in STAR.
                        buf += c;
                        pos++;
                        col++;
                    }
                    else {
                        // Not closing, go back to MULTI.
                        buf += c;
                        if (c == '\n') {
                            line++;
                            col = 1;
                        } else {
                            col++;
                        }
                        pos++;
                        state = State::MULTI;
                    }
                    break;

                case State::ACCEPT:
                    // Break out when comment is done.
                    goto done;
            }
        }

    done:
        // If we ended without ACCEPT state in multi-line, it's unterminated.
        if (state != State::ACCEPT) {
            throw std::runtime_error(
                "Unterminated comment at line " + std::to_string(line) +
                ", col " + std::to_string(startCol)
            );
        }
        Token t = { TokenType::Comment, buf, line, startCol };
        if (trace) printToken(t);
        return t;
    }
    // In this function we read JavaScript operators like =, ==, ===, !=, !==, <, <<, <=, >, >>, >>>, and so on.
    // we use peek to see the next character without moving pos immediately.
    // If programm find a multi-character operator (for example "==" or "!=="), advance() adds each character to buf one by one.
Token readOperatorFA(bool trace) {
    enum class State {
        START,      // before reading any operator
        GOT_EQ,     // we saw '='
        GOT_BANG,   // we saw '!'
        GOT_LT,     // we saw '<'
        GOT_GT,     // we saw '>'
        GOT_AND,    // we saw '&'
        GOT_OR,     // we saw '|'
        AFTER_GT1,  // we saw ">>"
        AFTER_GT2,  // we saw ">>>"
        ACCEPT      // finished reading the operator
    };

    State state = State::START;
    std::string buf;
    int startCol = col;

    auto peek = [&](int offset = 0) -> char {
        // return character at current position + offset, or '\0' if out of range
        return (pos + offset < input.size()) ? input[pos + offset] : '\0';
    };
    auto advance = [&]() {
        // add current char to buffer, move position and column forward
        buf += input[pos];
        pos++;
        col++;
    };

    while (pos < input.size()) {
        char c = peek();
        switch (state) {
            case State::START:
                // Here we decide which operator we begin with
                if (c == '=') {
                    advance();       // take '='
                    state = State::GOT_EQ;
                }
                else if (c == '!') {
                    advance();       // take '!'
                    state = State::GOT_BANG;
                }
                else if (c == '<') {
                    advance();       // take '<'
                    state = State::GOT_LT;
                }
                else if (c == '>') {
                    advance();       // take '>'
                    state = State::GOT_GT;
                }
                else if (c == '&') {
                    advance();       // take '&'
                    state = State::GOT_AND;
                }
                else if (c == '|') {
                    advance();       // take '|'
                    state = State::GOT_OR;
                }
                else if (std::string("+-*/%^").find(c) != std::string::npos) {
                    // Single-character operators: +, -, *, /, %, ^
                    advance();
                    state = State::ACCEPT;
                }
                else {
                    // If we did not match any operator start, throw error
                    throw std::runtime_error(
                        "Unknown operator at line " + std::to_string(line) +
                        ", column " + std::to_string(col)
                    );
                }
                break;

            case State::GOT_EQ:
                // After seeing '=', check if next is '=' or '==='
                if (peek() == '=') {
                    advance();    // take second '='
                    if (peek() == '=') {
                        advance(); // take third '=' for '==='
                    }
                }
                state = State::ACCEPT; // now operator is complete
                break;

            case State::GOT_BANG:
                // After seeing '!', check if next is '!=' or '!=='
                if (peek() == '=') {
                    advance();    // take '=' for '!='
                    if (peek() == '=') {
                        advance(); // take second '=' for '!=='
                    }
                }
                state = State::ACCEPT; // operator is complete
                break;

            case State::GOT_LT:
                // After seeing '<', check if next is '<=' or '<<'
                if (peek() == '<') {
                    advance();    // take second '<' for '<<'
                }
                else if (peek() == '=') {
                    advance();    // take '=' for '<='
                }
                state = State::ACCEPT; // operator is complete
                break;

            case State::GOT_GT:
                // After seeing '>', check if next is '>>', '>>>' or '>='
                if (peek() == '>') {
                    advance();    // take second '>' for '>>'
                    state = State::AFTER_GT1;
                }
                else if (peek() == '=') {
                    advance();    // take '=' for '>='
                    state = State::ACCEPT;
                }
                else {
                    state = State::ACCEPT; // just '>' is done
                }
                break;

            case State::AFTER_GT1:
                // We have '>>'; now check if next is '>>>' or '>>='
                if (peek() == '>') {
                    advance();    // take third '>' for '>>>'
                    state = State::AFTER_GT2;
                }
                else if (peek() == '=') {
                    advance();    // take '=' for '>>='
                    state = State::ACCEPT;
                }
                else {
                    state = State::ACCEPT; // just '>>' is done
                }
                break;

            case State::AFTER_GT2:
                // We have '>>>'; now check if next is '>>>='
                if (peek() == '=') {
                    advance();    // take '=' for '>>>='
                }
                state = State::ACCEPT; // done reading
                break;

            case State::GOT_AND:
                // After seeing '&', check if next is '&&'
                if (peek() == '&') {
                    advance();    // take second '&' for '&&'
                }
                state = State::ACCEPT; // done
                break;

            case State::GOT_OR:
                // After seeing '|', check if next is '||'
                if (peek() == '|') {
                    advance();    // take second '|' for '||'
                }
                state = State::ACCEPT; // done
                break;

            case State::ACCEPT:
                // When ACCEPT, break out of loop
                goto done;
        }
    }

done:
    // Build the token with collected characters
    Token t = { TokenType::Operator, buf, line, startCol };
    if (trace) printToken(t);
    return t;
}


    Token readPunctuation(bool trace) {
        Token t = { TokenType::Punctuation, std::string(1, input[pos]), line, col };
        pos++; col++;
        return t;
    }
    // These helper functions return true if the character is in the set of operators or punctuation.
    // Programm store those characters in strings so I can quickly check membership with find().
    // This lets choose the correct token-reading function quickly.
    bool isOperatorChar(char c) {
        return std::string("+-*/=<>!&|^%").find(c) != std::string::npos;
    }

    bool isPunctuation(char c) {
        return std::string("(){}[],;.").find(c) != std::string::npos;
    }
    // This helper function is for trace mode. If trace=true, I print each token in the format [line:column] TYPE 'lexeme'.
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

// In main(), I run an infinite loop so the user can input code multiple times.
// Options: manual input, trace mode input, demo code, or exit.
// If demo is chosen, console will show hardcoded piece of code 
// On exception, user get print the error message and return to the mode selection.

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
