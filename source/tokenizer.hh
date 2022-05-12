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

class InputStream
{
    public:
        virtual ~InputStream() = default;
        virtual bool eof() = 0;
        virtual int get() = 0;
        virtual void unget() = 0;
        virtual int cur() const = 0;
        virtual int line() const = 0;
        virtual int column() const = 0;
        virtual void skipws() = 0;
        virtual bool expect(int expect) = 0;
};

template <typename I> class IteratorInputStream : public InputStream
{
    protected:
        I cur_, end_;
        int last_, line_, column_;
        bool ungot_;

    public:
        IteratorInputStream( const I& first, const I& last ) : cur_(first), end_(last),
            last_(-1), line_(1), column_(0), ungot_(false)
        {
        }

        ~IteratorInputStream() = default;

        bool eof() override
        {
            return last_ == -2;
        }

        int get() override
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

        void unget() override
        {
            if (last_ >= 0)
            {
                if (ungot_) throw exception("unable to unget");
                ungot_ = true;
            }
        }

        int cur() const override { return *cur_; }

        int line() const override { return line_; }

        int column() const override { return column_; }

        void skipws() override
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

        bool expect(int expect) override
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

    Token( int code = TOKEN_EOF, const std::string &value = "", int line = 1, int column = 1);
    //Token( int code, const std::string &value, InputStream &is );
};

int findKeyword( const std::string &name );

class Tokenizer
{
    public:
        Token current;
        bool ungot;

        Tokenizer( InputStream &is );
        void unget();
        // TODO: create function to consume token and throw error is not from indicated type
        Token next();

    private:
        InputStream &is;

        Token comment();
        Token qname( int line = 1, int column = 1 );
        std::string name();
        Token integer( int first = 0, int line = 1, int column = 1 );
        Token literalString(int line = 1, int column = 1 );
};

} // protop

#endif // PROTOP_TOKENIZER
