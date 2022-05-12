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

}
