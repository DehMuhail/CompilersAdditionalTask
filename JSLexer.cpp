#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <cctype>

enum class TokenType {
    Keyword, Identifier,
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

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;

        while (pos < input.size()) {
            while (pos < input.size() && isspace(input[pos])) {
                if (input[pos] == '\n') { ++line; col = 1; }
                else ++col;
                ++pos;
            }

            if (pos >= input.size()) break;

            char c = input[pos];

            if (isalpha(c) || c == '_' || c == '$') {
                tokens.push_back(readIdentifier());
            } else {
                ++pos;
                ++col;
            }
        }

        tokens.push_back({ TokenType::EndOfFile, "", line, col });
        return tokens;
    }

private:
    Token readIdentifier() {
        std::string buf;
        int startCol = col;

        while (pos < input.size() &&
               (isalnum(input[pos]) || input[pos] == '_' || input[pos] == '$')) {
            buf += input[pos++];
            ++col;
        }

        TokenType type = keywords.count(buf) ? TokenType::Keyword : TokenType::Identifier;
        return { type, buf, line, startCol };
    }
};

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::Keyword: return "KW";
        case TokenType::Identifier: return "ID";
        case TokenType::EndOfFile: return "EOF";
        default: return "UNK";
    }
}

int main() {
    std::string input = "let x = value";

    Lexer lexer(input);
    std::vector<Token> tokens = lexer.tokenize();

    for (const auto& token : tokens) {
        std::cout << "[" << token.line << ":" << token.column << "] "
                  << tokenTypeToString(token.type) << " '" << token.lexeme << "'\n";
    }

    return 0;
}
