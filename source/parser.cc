/*
 * Copyright 2020-2022 Bruno Ribeiro <https://github.com/brunexgeek>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <protop/protop.hh>
#include "tokenizer.hh"
#include <iterator>
#include <sstream>
#include <list>
#include <set>

#define IS_VALID_TYPE(x)       ( (x) >= protogen::TYPE_DOUBLE && (x) <= protogen::TYPE_MESSAGE )
#define TOKEN_POSITION(t)      (t).line, (t).column
#define CURRENT_TOKEN_POSITION ctx.tokens.current.line, ctx.tokens.current.column

#ifdef BUILD_DEBUG

static const char *TOKENS[] =
{
    "TOKEN_EOF",
    "TOKEN_MESSAGE",
    "TOKEN_NAME",
    "TOKEN_EQUAL",
    "TOKEN_REPEATED",
    "TOKEN_T_DOUBLE",
    "TOKEN_T_FLOAT",
    "TOKEN_T_INT32",
    "TOKEN_T_INT64",
    "TOKEN_T_UINT32",
    "TOKEN_T_UINT64",
    "TOKEN_T_SINT32",
    "TOKEN_T_SINT64",
    "TOKEN_T_FIXED32",
    "TOKEN_T_FIXED64",
    "TOKEN_T_SFIXED32",
    "TOKEN_T_SFIXED64",
    "TOKEN_T_BOOL",
    "TOKEN_T_STRING",
    "TOKEN_T_BYTES",
    "TOKEN_T_MESSAGE",
    "TOKEN_SYNTAX",
    "TOKEN_QNAME",
    "TOKEN_STRING",
    "TOKEN_INTEGER",
    "TOKEN_COMMENT",
    "TOKEN_ENUM",
    "TOKEN_SCOLON",
    "TOKEN_PACKAGE",
    "TOKEN_LT",
    "TOKEN_GT",
    "TOKEN_MAP",
    "TOKEN_COMMA",
    "TOKEN_BEGIN",
    "TOKEN_END",
    "TOKEN_OPTION",
    "TOKEN_TRUE",
    "TOKEN_FALSE",
    "TOKEN_LBRACKET",
    "TOKEN_RBRACKET",
    "RPC",
    "SERVICE",
    "RETURNS",
    "TOKEN_LPAREN",
    "TOKEN_RPAREN",
};

#endif

namespace protop {

struct Context
{
    Tokenizer &tokens;
    Proto &tree;
    InputStream &is;
    std::string package;

    Context( Tokenizer &tokenizer, Proto &tree, InputStream &is ) :
        tokens(tokenizer), tree(tree), is(is)
    {
    }
};

static std::string qualifiedName( Context &ctx, const std::string &name )
{
    if (ctx.package.empty()) return name;
    if (ctx.package.back() == '.')
        return ctx.package + name;
    else
        return ctx.package + '.' + name;
}

static std::string parseName( Context &ctx, bool qualified = false )
{
    if (ctx.tokens.current.code != TOKEN_NAME && ctx.tokens.current.code != TOKEN_QNAME)
    {
        if (findKeyword(ctx.tokens.current.value) == TOKEN_NAME)
            throw exception("Missing field name", TOKEN_POSITION(ctx.tokens.current));
        return ctx.tokens.current.value;
    }
    else
    if (ctx.tokens.current.code == TOKEN_QNAME && !qualified)
        throw exception("Cannot use a qualified name", TOKEN_POSITION(ctx.tokens.current));
    return ctx.tokens.current.value;
}

static OptionEntry parseOption( Context &ctx )
{
    // the token 'option' is already consumed at this point
    OptionEntry temp;

    temp.line = ctx.tokens.current.line;

    // option name
    ctx.tokens.next();
    temp.name = parseName(ctx, true);
    // equal symbol
    if (ctx.tokens.next().code != TOKEN_EQUAL)
        throw exception("Expected '='", TOKEN_POSITION(ctx.tokens.current));
    // option value
    ctx.tokens.next();
    switch (ctx.tokens.current.code)
    {
        case TOKEN_TRUE:
        case TOKEN_FALSE:
            temp.type = OptionType::BOOLEAN; break;
        case TOKEN_NAME:
        case TOKEN_QNAME:
            temp.type = OptionType::IDENTIFIER; break;
        case TOKEN_INTEGER:
            temp.type = OptionType::INTEGER; break;
        case TOKEN_STRING:
            temp.type = OptionType::STRING; break;
        default:
            throw exception("Invalid option value", TOKEN_POSITION(ctx.tokens.current));
    }
    temp.value = ctx.tokens.current.value;
    return temp;
}

static void parseFieldOptions( Context &ctx, OptionMap &entries )
{
    while (true)
    {
        if (ctx.tokens.next().code == TOKEN_RBRACKET) break;
        ctx.tokens.unget();
        OptionEntry option = parseOption(ctx);
        if (ctx.tokens.next().code != TOKEN_COMMA) ctx.tokens.unget();
        entries[option.name] = option;
    }
    // give back the TOKEN_RBRACKET
    ctx.tokens.unget();
}

static void parseStandardOption( Context &ctx, OptionMap &entries )
{
    // the token 'option' is already consumed at this point

    OptionEntry option = parseOption(ctx);

    // semicolon symbol
    if (ctx.tokens.next().code != TOKEN_SCOLON)
        throw exception("Expected '='", TOKEN_POSITION(ctx.tokens.current));

    entries[option.name] = option;
}

static std::shared_ptr<Enum> findEnum( Context &ctx, const std::string &name )
{
    for (auto it = ctx.tree.enums.begin(); it != ctx.tree.enums.end(); ++it)
        if ((*it)->qname == name) return *it;
    return nullptr;
}

static std::shared_ptr<Message> findMessage( Context &ctx, const std::string &name )
{
    for (auto it = ctx.tree.messages.begin(); it != ctx.tree.messages.end(); ++it)
        if ((*it)->qname == name) return *it;
    return nullptr;
}

static void parseTypeInfo( Context &ctx, TypeInfo &type )
{
    if (ctx.tokens.current.code >= TOKEN_T_DOUBLE && ctx.tokens.current.code <= TOKEN_T_BYTES)
        type.id = (FieldType) ctx.tokens.current.code;
    else
    if (ctx.tokens.current.code == TOKEN_NAME || ctx.tokens.current.code == TOKEN_QNAME) // TODO: use 'parseName'
    {
        type.id = TYPE_COMPLEX;
        type.name = ctx.tokens.current.value; // TODO: check whether is originally qualified
        type.package = ctx.package;
        type.mref = nullptr;
        type.eref = nullptr;
    }
# if 0
    else
    if (ctx.tokens.current.code == TOKEN_MAP)
    {
        if (ctx.tokens.next().code != TOKEN_LT) throw exception("Missing <");

        // key type
        if (ctx.tokens.next().code != TOKEN_T_STRING) throw exception("Key type must be 'string'");
        field.type.id = (FieldType) ctx.tokens.current.code;
        // comma
        if (ctx.tokens.next().code != TOKEN_COMMA) throw exception("Missing comma");
        //value type
        if (ctx.tokens.next().code != TOKEN_T_STRING) throw exception("Key type must be 'string'");
        field.type.id = (FieldType) ctx.tokens.current.code;

        if (ctx.tokens.next().code != TOKEN_GT) throw exception("Missing >");
    }
#endif
    else
        throw exception("Missing type", TOKEN_POSITION(ctx.tokens.current));
}

static void parseField( Context &ctx, Message &message )
{
    std::shared_ptr<Field> field = std::make_shared<Field>();

    if (ctx.tokens.current.code == TOKEN_REPEATED)
    {
        field->type.repeated = true;
        ctx.tokens.next();
    }
    else
        field->type.repeated = false;

    // type
    parseTypeInfo(ctx, field->type);

    // name
    ctx.tokens.next();
    field->name = parseName(ctx);
    // equal symbol
    if (ctx.tokens.next().code != TOKEN_EQUAL) throw exception("Expected '='", TOKEN_POSITION(ctx.tokens.current));
    // index
    if (ctx.tokens.next().code != TOKEN_INTEGER) throw exception("Missing field index", TOKEN_POSITION(ctx.tokens.current));
    field->index = (int) strtol(ctx.tokens.current.value.c_str(), nullptr, 10);

    ctx.tokens.next();

    // options
    if (ctx.tokens.current.code == TOKEN_LBRACKET)
    {
        parseFieldOptions(ctx, field->options);
        if (ctx.tokens.next().code != TOKEN_RBRACKET)
            throw exception("Expected ']'", TOKEN_POSITION(ctx.tokens.current));
        ctx.tokens.next();
    }

    // semi-colon
    if (ctx.tokens.current.code != TOKEN_SCOLON)
        throw exception("Expected ';'", TOKEN_POSITION(ctx.tokens.current));

    // check for repeated field indices
    for (auto item : message.fields)
        if (item->index == field->index)
            throw exception("Field '" + item->name + "' has the same index as '" + field->name + "'", CURRENT_TOKEN_POSITION);

    message.fields.push_back(field);
}

static void parseContant( Context &ctx, Enum &entity )
{
    std::shared_ptr<Constant> value = std::make_shared<Constant>();

    // name
    value->name = parseName(ctx);
    if (ctx.tokens.next().code != TOKEN_EQUAL)
        throw exception("Missing equal sign", CURRENT_TOKEN_POSITION);
    // value
    if (ctx.tokens.next().code != TOKEN_INTEGER)
        throw exception("Missing constant value", CURRENT_TOKEN_POSITION);
    value->value = (int) strtol(ctx.tokens.current.value.c_str(), nullptr, 10);
    // semicolon
    if (ctx.tokens.next().code != TOKEN_SCOLON)
        throw exception("Missing semicolon", CURRENT_TOKEN_POSITION);

    entity.constants.push_back(value);
}

static void parseEnum( Context &ctx )
{
    if (ctx.tokens.current.code == TOKEN_ENUM)
    {
        std::shared_ptr<Enum> entity = std::make_shared<Enum>();

        ctx.tokens.next();
        entity->name = parseName(ctx);;
        entity->qname = qualifiedName(ctx, entity->name);
        if (ctx.tokens.next().code != TOKEN_BEGIN)
            throw exception("Missing enum body", CURRENT_TOKEN_POSITION);

        while (ctx.tokens.next().code != TOKEN_END)
        {
            if (ctx.tokens.current.code == TOKEN_OPTION)
                parseStandardOption(ctx, entity->options);
            else
                parseContant(ctx, *entity);
        }
        ctx.tree.enums.push_back(entity);
    }
    else
        throw exception("Expected enum", CURRENT_TOKEN_POSITION);
}

static void parseMessage( Context &ctx )
{
    if (ctx.tokens.current.code == TOKEN_MESSAGE)
    {
        std::shared_ptr<Message> message = std::make_shared<Message>();

        ctx.tokens.next();
        message->name = parseName(ctx);
        message->qname = qualifiedName(ctx, message->name);
        if (ctx.tokens.next().code != TOKEN_BEGIN)
            throw exception("Missing message body", CURRENT_TOKEN_POSITION);

        while (ctx.tokens.next().code != TOKEN_END)
        {
            if (ctx.tokens.current.code == TOKEN_OPTION)
                parseStandardOption(ctx, message->options);
            else
                parseField(ctx, *message);
        }
        ctx.tree.messages.push_back(message);
    }
    else
        throw exception("Invalid message", CURRENT_TOKEN_POSITION);
}


static void parsePackage( Context &ctx )
{
    Token tt = ctx.tokens.next();
    if ((tt.code == TOKEN_NAME || tt.code == TOKEN_QNAME) && ctx.tokens.next().code == TOKEN_SCOLON)
    {
        ctx.package = tt.value;
    }
    else
        throw exception("Invalid package", CURRENT_TOKEN_POSITION);
}


static void parseSyntax( Context &ctx )
{
    // the token 'syntax' is already consumed at this point

    if (ctx.tokens.next().code != TOKEN_EQUAL)
        throw exception("Expected '='", CURRENT_TOKEN_POSITION);
    Token tt = ctx.tokens.next();
    if (tt.code == TOKEN_STRING && ctx.tokens.next().code == TOKEN_SCOLON)
    {
        if (tt.value != "proto3") throw exception("Invalid language version", CURRENT_TOKEN_POSITION);
    }
    else
        throw exception("Invalid syntax", CURRENT_TOKEN_POSITION);
}

static void parseProcedure( Context &ctx, std::shared_ptr<Service> service )
{
    auto proc = std::make_shared<Procedure>();

    // name
    ctx.tokens.next();
    proc->name = parseName(ctx);
    // request
    if (ctx.tokens.next().code != TOKEN_LPAREN)
        throw exception("Missing left parenthesis", CURRENT_TOKEN_POSITION);
    ctx.tokens.next();
    parseTypeInfo(ctx, proc->request);
    if (ctx.tokens.next().code != TOKEN_RPAREN)
        throw exception("Missing right parenthesis", CURRENT_TOKEN_POSITION);
    // response
    if (ctx.tokens.next().code != TOKEN_RETURNS)
        throw exception("Missing returns", CURRENT_TOKEN_POSITION);
    if (ctx.tokens.next().code != TOKEN_LPAREN)
        throw exception("Missing left parenthesis", CURRENT_TOKEN_POSITION);
    ctx.tokens.next();
    parseTypeInfo(ctx, proc->response);
    if (ctx.tokens.next().code != TOKEN_RPAREN)
        throw exception("Missing right parenthesis", CURRENT_TOKEN_POSITION);

    ctx.tokens.next();
    if (ctx.tokens.current.code != TOKEN_BEGIN && ctx.tokens.current.code != TOKEN_SCOLON)
        throw exception("Unexpected token", CURRENT_TOKEN_POSITION);
    if (ctx.tokens.current.code == TOKEN_BEGIN && ctx.tokens.next().code != TOKEN_END)
        throw exception("Missing right braces", CURRENT_TOKEN_POSITION);

    service->procs.push_back(proc);
}

static void parseService( Context &ctx )
{
    auto service = std::make_shared<Service>();

    ctx.tokens.next();
    service->name = parseName(ctx);

    if (ctx.tokens.next().code != TOKEN_BEGIN)
        throw exception("Missing service body", CURRENT_TOKEN_POSITION);

    while (ctx.tokens.next().code != TOKEN_END)
    {
        if (ctx.tokens.current.code == TOKEN_RPC)
            parseProcedure(ctx, service);
        else
            throw exception("Unexpected token" + ctx.tokens.current.value, TOKEN_POSITION(ctx.tokens.current));
    }

    ctx.tree.services.push_back(service);
}

static void parseProto( Context &ctx )
{
    do
    {
        ctx.tokens.next();
        if (ctx.tokens.current.code == TOKEN_MESSAGE)
            parseMessage(ctx);
        else
        if (ctx.tokens.current.code == TOKEN_PACKAGE)
            parsePackage(ctx);
        else
        if (ctx.tokens.current.code == TOKEN_COMMENT)
            continue;
        else
        if (ctx.tokens.current.code == TOKEN_SYNTAX)
            parseSyntax(ctx);
        else
        if (ctx.tokens.current.code == TOKEN_OPTION)
            parseStandardOption(ctx, ctx.tree.options);
        else
        if (ctx.tokens.current.code == TOKEN_ENUM)
            parseEnum(ctx);
        else
        if (ctx.tokens.current.code == TOKEN_SERVICE)
            parseService(ctx);
        else
        if (ctx.tokens.current.code == TOKEN_EOF)
            break;
        else
        {
            throw exception("Unexpected token", TOKEN_POSITION(ctx.tokens.current));
        }
    } while (ctx.tokens.current.code != 0);
}

typedef std::list<std::shared_ptr<Message>> MessageList;
typedef std::set<std::shared_ptr<Message>> MessageSet;

static bool contains( const MessageList &items, const std::shared_ptr<Message> message )
{
    for (auto mi = items.begin(); mi != items.end(); ++mi)
        if (*mi == message) return true;
    return false;
}

static void sort( MessageList &items, MessageSet &pending, std::shared_ptr<Message> message )
{
    if (pending.find(message) != pending.end())
        throw exception("Circular reference with " + message->name);
    //if (contains(items, message)) return; // already processed

    pending.insert(message);
    for (auto fi : message->fields)
    {
        if (fi->type.mref == nullptr || fi->type.mref == message)
            continue;
        if (!contains(items, fi->type.mref))
            sort(items, pending, fi->type.mref);
    }
    items.push_back(message);
    pending.erase(message);
}

static void sort_messages( Context &ctx )
{
    MessageList items;
    MessageSet pending;
    for (auto mi : ctx.tree.messages)
        sort(items, pending, mi);
    ctx.tree.messages.swap(items);
}

void Proto::parse( Proto &tree, std::istream &input, const std::string &fileName )
{
    std::ios_base::fmtflags flags = input.flags();
    std::noskipws(input);
    std::istream_iterator<char> end;
    std::istream_iterator<char> begin(input);

    IteratorInputStream< std::istream_iterator<char> > is(begin, end);
    Tokenizer tok(is);

    Context ctx(tok, tree, is);
    tree.fileName = fileName;

    try
    {
        parseProto(ctx);
        tree.package = ctx.package;
        if (flags & std::ios::skipws) std::skipws(input);
    } catch (exception &ex)
    {
        if (flags & std::ios::skipws) std::skipws(input);
        throw ex;
    }

    // check if we have unresolved types
    for (auto mit : tree.messages)
    {
        for (auto fit : mit->fields)
        {
            if (fit->type.id != TYPE_COMPLEX) continue;

            auto qname = fit->type.package + "." + fit->type.name;

            fit->type.mref = findMessage(ctx, qname);
            if (fit->type.mref == nullptr)
                fit->type.eref = findEnum(ctx, qname);
            if (fit->type.mref == nullptr && fit->type.eref == nullptr)
                    throw exception("Unable to find type '" + qname + "'");
        }
    }
    // sort messages and check for circular references
    sort_messages(ctx);
}

} // protogen

