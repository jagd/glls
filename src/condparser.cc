#include "condparser.h"
#include "symbollist.h"
#include "parsercommon.h"

#include <istream>
#include <sstream>
#include <cassert>
#include <cctype>

//constexpr int CondDict::ID_CONST;
//constexpr int CondDict::ID_INVALID;
//constexpr int CondDict::ID_X_VAR_NEG_BASE;
//constexpr int CondDict::ID_Y_VAR_POS_BASE;

CondTree build_equation(const CondTree &a, const CondTree &b)
{
    std::unique_ptr<CondTreeNode> root(new CondTreeNode('-'));
    root->left = a.root->clone();
    root->right = b.root->clone();
    return CondTree(std::move(root));
}

CondDict::CondDict(const SymbolList &sl, const std::string &xVarName)
    : xVarName_(xVarName), symList_(sl)
{
    // assert(!xVarName_.empty());
    checkXVarName();
}

CondDict::CondDict(SymbolList &&sl, const std::string &xVarName)
    : xVarName_(xVarName), symList_(sl)
{
    // assert(!xVarName_.empty());
    checkXVarName();
}

void CondDict::checkXVarName() const
{
    if (xVarName_.empty()) {
        throw std::logic_error("CondLexer: variable name can not be empty");
    }
}

int CondDict::symToID(const std::string &name, int index) const
{
    assert(index >= 0);
    if (name == xVarName_) {
        return ID_X_VAR_NEG_BASE - index;
    }
    const int offset = symList_.query(name);
    if (offset >= 0) {
        return static_cast<int>(symList_.size() * index + offset);
    }
    return ID_INVALID;
}

CondLexer::CondLexer(std::istream &s, const CondDict &d)
        : stream_(s), dict_(d)
{
}

CondLexer::CondLexer(std::istream &s, CondDict &&d)
        : stream_(s), dict_(d)
{
}

CondLexer::Token CondLexer::token()
{
    stream_ >> std::ws;
    const int peek = static_cast<int>(stream_.peek());
    if (peek == std::istream::traits_type::eof()) {
        msg_ = "EOF";
        return Token::TK_EOF;
    } else if (std::isalpha(peek)) {
        return peekAlpha();
        } else if (std::isdigit(peek)) {
        stream_ >> num_;
        return Token::TK_NUM;
    } else switch (peek) {
        case '-':
        case '+':
        case '*':
        case '/':
        case '(':
        case ')':
        case '=':
            symbol_ = static_cast<int>(stream_.get());
            msg_ = static_cast<char>(symbol_);
            return Token::TK_OP;
        default:
            symbol_ = static_cast<int>(stream_.get());
            msg_ = "invalid symbol ";
            msg_ += static_cast<char>(symbol_);
            return Token::TK_INVALID;
    }
    assert(!"code should be unreachable");
}

CondLexer::Token CondLexer::peekAlpha()
{
    std::string name;
    std::string numstr;
    while (std::isalpha(static_cast<int>(stream_.peek()))) {
        name.push_back(static_cast<char>(stream_.get()));
    }
    while (std::isdigit(static_cast<int>(stream_.peek()))) {
        numstr.push_back(static_cast<char>(stream_.get()));
    }
    if (numstr.empty()) {
        msg_ = name + " should follow an integer index";
        return Token::TK_INVALID;
    }
    // do not use std::istringstream because it differs from 010 and 10
    int num = 0;
    for (const auto i : numstr) {
        num *= 10;
        num += i - '0';
    }
    msg_.clear();
    symbol_ = dict_.symToID(name, num);
    if (symbol_ == dict_.ID_INVALID)  {
        msg_ += ", invalid symbol ";
        msg_ += name;
        msg_ += numstr;
        return Token::TK_INVALID;
    }
    return Token::TK_ID;
}

CondParser::CondParser(
        std::istream & s,
        const SymbolList &l,
        const std::string &xVarName
) : lexer_(s, CondDict(l, xVarName))
{
    forward_ = lexer_.token();
}

std::vector<CondTree> CondParser::parseCondMiddle()
{
    std::vector<CondTree> trees;
    while (forward_ == CondLexer::Token::TK_OP && lexer_.symbol() == '=') {
        forward_ = lexer_.token();
        trees.push_back(parseExpr());
    }
    return trees;
}

std::unique_ptr<CondTreeNode> CondParser::parseExpr()
{
    std::unique_ptr<CondTreeNode> t = parseTerm();
    while ( forward_ == CondLexer::Token::TK_OP
      && (lexer_.symbol() == '+' || lexer_.symbol() == '-') )
    {
        auto r = CondTreeNode::make(static_cast<char>(lexer_.symbol()));
        forward_ = lexer_.token();
        r->right = parseTerm();
        r->left = std::move(t);
        t = std::move(r);
    }
    return t;
}

std::vector<CondTree> CondParser::parseCond()
{
    const CondTree a(parseExpr());
    auto eqn = parseCondMiddle();
    if (eqn.empty()) {
        throw ParserError(
                0,
                "expect at least one equation",
                ParserError::Type::SEMANTIC_ERROR
        );
    }
    for (auto &x : eqn) {
        x = build_equation(a, x);
    }
    return eqn;
}

std::vector<CondTree> CondParser::parse()
{
    const auto r = parseCond();
    if (forward_ != CondLexer::Token::TK_EOF) {
        throw ParserError(
                0,
                "unrecognized content: " + lexer_.msg(),
                ParserError::Type::UNEXPECTED_CHAR
        );
    }
    return r;
}

std::unique_ptr<CondTreeNode> CondParser::parseTerm()
{
    auto a = parseAtom();
    while (forward_ == CondLexer::Token::TK_OP
        && (lexer_.symbol() == '*' || lexer_.symbol() == '/')
    ) {
        auto r = CondTreeNode::make(static_cast<char>(lexer_.symbol()));
        forward_ = lexer_.token();
        r->right = parseAtom();
        r->left = std::move(a);
        a = std::move(r);
    }
    return a;
}

std::unique_ptr<CondTreeNode> CondParser::parseAtom()
{
    if (forward_ == CondLexer::Token::TK_OP && lexer_.symbol() == '-') {
        forward_ = lexer_.token();
        auto r = CondTreeNode::make('*');
        r->left = CondTreeNode::make(-1.0);
        r->right = parseAtomTail();
        return r;
    }
    return parseAtomTail();
}

std::unique_ptr<CondTreeNode> CondParser::parseAtomTail()
{
    switch (forward_) {
        case CondLexer::Token::TK_ID: {
            const auto sym = lexer_.symbol();
            forward_ = lexer_.token();
            return CondTreeNode::make(sym);
        };
        case CondLexer::Token::TK_NUM: {
            const auto num = lexer_.num();
            forward_ = lexer_.token();
            return CondTreeNode::make(num);
        };
        case CondLexer::Token::TK_INVALID:
            throw ParserError(
                    0,
                    "invalid token " + lexer_.msg(),
                    ParserError::Type::INVALID_TOKEN
            );
        case CondLexer::Token::TK_OP:
            if (lexer_.symbol() == '(') {
                forward_ = lexer_.token();
                auto expr = parseExpr();
                if ( forward_ != CondLexer::Token::TK_OP
                  || lexer_.symbol() !=  ')' ) {
                    throw ParserError(
                            0,
                            "missing or unmatched ')'",
                            ParserError::Type::EXPECT_CHAR
                    );
                }
                forward_ = lexer_.token();
                return expr;
            } else {
                throw ParserError(
                        0,
                        "unexpected operator " + lexer_.msg(),
                        ParserError::Type::EXPECT_CHAR
                );
            }
        //case CondLexer::Token::TK_EOF:
        default:
            throw ParserError(
                    0,
                    "unexpected EOF",
                    ParserError::Type::UNEXPECTED_EOF
            );
    }
}
