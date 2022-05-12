#include "tokenizer.hh"

namespace protop {

static const struct
{
    int code;
    const char *keyword;
} KEYWORDS[] =
{
    { TOKEN_MESSAGE     , "message" },
    { TOKEN_REPEATED    , "repeated" },
    { TOKEN_T_STRING    , "string" },
    { TOKEN_ENUM        , "enum" },
    { TOKEN_T_DOUBLE    , "double" },
    { TOKEN_T_FLOAT     , "float" },
    { TOKEN_T_BOOL      , "bool" },
    { TOKEN_T_INT32     , "int32" },
    { TOKEN_T_INT64     , "int64" },
    { TOKEN_T_UINT32    , "uint32" },
    { TOKEN_T_UINT64    , "uint64" },
    { TOKEN_T_SINT32    , "sint32" },
    { TOKEN_T_SINT64    , "sint64" },
    { TOKEN_T_FIXED32   , "fixed32" },
    { TOKEN_T_FIXED64   , "fixed64" },
    { TOKEN_T_SFIXED32  , "sfixed32" },
    { TOKEN_T_SFIXED64  , "sfixed64" },
    { TOKEN_T_BYTES     , "bytes" },
    { TOKEN_PACKAGE     , "package" },
    { TOKEN_SYNTAX      , "syntax" },
    { TOKEN_MAP         , "map" },
    { TOKEN_OPTION      , "option" },
    { TOKEN_TRUE        , "true" },
    { TOKEN_FALSE       , "false" },
    { TOKEN_RPC         , "rpc" },
    { TOKEN_SERVICE     , "service" },
    { TOKEN_RETURNS     , "returns" },
    { 0, nullptr },
};

int findKeyword( const std::string &name )
{
    for (int i = 0; KEYWORDS[i].keyword != nullptr; ++i)
        if (name == KEYWORDS[i].keyword)
            return KEYWORDS[i].code;
    return TOKEN_NAME;
}

Token::Token( int code, const std::string &value, int line, int column )
    : code(code), value(value), line(line), column(column)
{
}

/*Token::Token( int code, const std::string &value, InputStream &is )
    : code(code), value(value)
{
    line = is.line();
    column = is.column() - value.length();
}*/

Tokenizer::Tokenizer( InputStream &is ) : ungot(false), is(is)
{
}

void Tokenizer::unget()
{
    if (ungot)
        throw exception("Already ungot", current.line, current.column);
    ungot = true;
}

// TODO: create function to consume token and throw error is not from indicated type

Token Tokenizer::next()
{
    int line = 1;
    int column = 1;

    while (true)
    {
        if (ungot)
        {
            ungot = false;
            return current;
        }
        is.skipws();

        line = is.line();
        column = is.column();

        int cur = is.get();
        if (cur < 0) break;

        if (IS_LETTER(cur))
        {
            is.unget();
            current = qname(line, column);
        }
        else
        if (IS_DIGIT(cur))
            current = integer(cur, line, column);
        else
        if (cur == '/')
        {
            comment(); //discarding
            continue;
        }
        else
        if (cur == '\n' || cur == '\r')
            continue;
        else
        if (cur == '"')
            current = literalString(line, column);
        else
        if (cur == '=')
            current = Token(TOKEN_EQUAL, "", line, column);
        else
        if (cur == '{')
            current = Token(TOKEN_BEGIN, "", line, column);
        else
        if (cur == '}')
            current = Token(TOKEN_END, "", line, column);
        else
        if (cur == '(')
            current = Token(TOKEN_LPAREN, "", line, column);
        else
        if (cur == ')')
            current = Token(TOKEN_RPAREN, "", line, column);
        else
        if (cur == ';')
            current = Token(TOKEN_SCOLON, "", line, column);
        else
        if (cur == ',')
            current = Token(TOKEN_COMMA, "", line, column);
        else
        if (cur == '<')
            current = Token(TOKEN_LT, "", line, column);
        else
        if (cur == '>')
            current = Token(TOKEN_GT, "", line, column);
        else
        if (cur == '[')
            current = Token(TOKEN_LBRACKET, "", line, column);
        else
        if (cur == ']')
            current = Token(TOKEN_RBRACKET, "", line, column);
        else
            throw exception("Invalid symbol", line, column);

        return current;
    }

    return current = Token(TOKEN_EOF, "", line, column);
}

Token Tokenizer::comment()
{
    Token temp(TOKEN_COMMENT, "");
    int cur = is.get();

    if (cur == '/')
    {
        while ((cur = is.get()) != '\n') temp.value += (char) cur;
        return temp;
    }
    else
    if (cur == '*')
    {
        while (true)
        {
            cur = is.get();
            if (cur == '*' && is.expect('/')) return temp;
            if (cur < 0) return Token();
            temp.value += (char) cur;
        }
    }

    return Token();
}

Token Tokenizer::qname( int line, int column )
{
    // capture the identifier
    int type = TOKEN_NAME;
    std::string name = this->name();
    while (is.get() == '.')
    {
        type = TOKEN_QNAME;
        std::string cur = this->name();
        if (cur.length() == 0)
            throw exception("Invalid identifier", line, column);
        name += '.';
        name += cur;
    }
    is.unget();

    // we found a keyword?
    if (type == TOKEN_NAME)
        type = findKeyword(name);

    return Token(type, name, line, column);
}

std::string Tokenizer::name()
{
    std::string temp;
    bool first = true;
    int cur;
    while ((cur = is.get()) >= 0)
    {
        if ((first && !IS_LETTER(cur)) || !IS_LETTER_OR_DIGIT(cur)) break;
        first = false;
        temp += (char)cur;
    }
    is.unget();
    return temp;
}

Token Tokenizer::integer( int first, int line, int column )
{
    Token tt(TOKEN_INTEGER, "", line, column);
    if (first != 0) tt.value += (char) first;

    do
    {
        int cur = is.get();
        if (!IS_DIGIT(cur))
        {
            is.unget();
            return tt;
        }
        tt.value += (char) cur;
    } while (true);

    return tt;
}

Token Tokenizer::literalString(int line, int column )
{
    Token tt(TOKEN_STRING, "", line, column);

    do
    {
        int cur = is.get();
        if (cur == '\n' || cur == 0) return Token();
        if (cur == '"') return tt;
        tt.value += (char) cur;
    } while (true);

    return tt;
}

} // protop
