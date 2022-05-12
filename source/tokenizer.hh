#ifndef PROTOP_TOKENIZER
#define PROTOP_TOKENIZER

#include <string>
#include "exception.hh"

#define TOKEN_EOF              0
#define TOKEN_MESSAGE          1
#define TOKEN_NAME             2
#define TOKEN_EQUAL            3
#define TOKEN_REPEATED         4
#define TOKEN_T_DOUBLE         6
#define TOKEN_T_FLOAT          7
#define TOKEN_T_INT32          8
#define TOKEN_T_INT64          9
#define TOKEN_T_UINT32         10
#define TOKEN_T_UINT64         11
#define TOKEN_T_SINT32         12
#define TOKEN_T_SINT64         13
#define TOKEN_T_FIXED32        14
#define TOKEN_T_FIXED64        15
#define TOKEN_T_SFIXED32       16
#define TOKEN_T_SFIXED64       17
#define TOKEN_T_BOOL           18
#define TOKEN_T_STRING         19
#define TOKEN_T_BYTES          20
#define TOKEN_T_COMPLEX        21
#define TOKEN_ENUM             22
#define TOKEN_SYNTAX           27
#define TOKEN_QNAME            23
#define TOKEN_STRING           24
#define TOKEN_INTEGER          25
#define TOKEN_COMMENT          26
#define TOKEN_SCOLON           28
#define TOKEN_PACKAGE          29
#define TOKEN_LT               30
#define TOKEN_GT               31
#define TOKEN_MAP              32
#define TOKEN_COMMA            33
#define TOKEN_BEGIN            34
#define TOKEN_END              35
#define TOKEN_OPTION           36
#define TOKEN_TRUE             37
#define TOKEN_FALSE            38
#define TOKEN_LBRACKET         39
#define TOKEN_RBRACKET         40
#define TOKEN_RPC              41
#define TOKEN_SERVICE          42
#define TOKEN_RETURNS          43
#define TOKEN_LPAREN           44
#define TOKEN_RPAREN           45

#define IS_LETTER(x)           ( ((x) >= 'A' && (x) <= 'Z') || ((x) >= 'a' && (x) <= 'z') || (x) == '_' )
#define IS_DIGIT(x)            ( (x) >= '0' && (x) <= '9' )
#define IS_LETTER_OR_DIGIT(x)  ( IS_LETTER(x) || IS_DIGIT(x) )

namespace protop {

template <typename I> class InputStream
{
    protected:
        I cur_, end_;
        int last_, line_, column_;
        bool ungot_;

    public:
        InputStream( const I& first, const I& last ) : cur_(first), end_(last),
            last_(-1), line_(1), column_(0), ungot_(false)
        {
        }

        bool eof()
        {
            return last_ == -2;
        }

        int get()
        {
            if (ungot_)
            {
                ungot_ = false;
                return last_;
            }
            if (last_ == '\n')
            {
                ++line_;
                column_ = 0;
            }
            if (cur_ == end_)
            {
                last_ = -2;
                return -1;
            }

            last_ = *cur_ & 0xFF;
            ++cur_;
            ++column_;
            return last_;
        }

        void unget()
        {
            if (last_ >= 0)
            {
                if (ungot_) throw exception("unable to unget");
                ungot_ = true;
            }
        }

        int cur() const { return *cur_; }

        int line() const { return line_; }

        int column() const { return column_; }

        void skipws()
        {
            while (1) {
                int ch = get();
                if (! (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r'))
                {
                    unget();
                    break;
                }
            }
        }

        bool expect(int expect)
        {
            if (get() != expect)
            {
                unget();
                return false;
            }
            return true;
        }
};


struct Token
{
    int code;
    std::string value;
    int line, column;

    Token( int code = TOKEN_EOF, const std::string &value = "", int line = 1, int column = 1) :
        code(code), value(value), line(line), column(column)
    {
    }

    template<typename I>
    Token( int code, const std::string &value, InputStream<I> &is ) : code(code), value(value)
    {
        line = is.line();
        column = is.column() - value.length();
    }

};

int findKeyword( const std::string &name );

template <typename I> class Tokenizer
{
    public:
        Token current;
        bool ungot;

        Tokenizer( InputStream<I> &is ) : ungot(false), is(is)
        {
        }

        void unget()
        {
            if (ungot)
                throw exception("Already ungot", current.line, current.column);
            ungot = true;
        }

        // TODO: create function to consume token and throw error is not from indicated type

        Token next()
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

    private:
        InputStream<I> &is;

        Token comment()
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

        Token qname( int line = 1, int column = 1 )
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

        std::string name()
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

        Token integer( int first = 0, int line = 1, int column = 1 )
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

        Token literalString(int line = 1, int column = 1 )
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
};

} // protop

#endif // PROTOP_TOKENIZER
