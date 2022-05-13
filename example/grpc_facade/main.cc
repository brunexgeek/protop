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
#include <fstream>
#include <vector>

using namespace protop;

struct Context
{
    std::ostream &header;
    std::ostream &source;
    std::string ifname;
    std::string phname;
    std::string grpcns;
    std::vector<std::string> nspace;
};

static const char *AUTO_NEW = "\
template<class T> \n\
class auto_new \n\
{ \n\
    public: \n\
        T &operator=( const T &that ) { if (!o_) o_ = new T(); *o_ = that; return *o_; } \n\
        T *operator->() { if (!o_) o_ = new T(); return o_; } \n\
        T *operator->() const { if (!o_) o_ = new T(); return o_; } \n\
        T &operator*() { if (!o_) o_ = new T(); return *o_; } \n\
        T &operator*() const { if (!o_) o_ = new T(); return *o_; } \n\
        bool operator==( std::nullptr_t ) const { return o_ == nullptr; } \n\
        operator bool() const { return o_ == nullptr; } \n\
    protected: \n\
        mutable T *o_ = nullptr; \n\
};\n";

static const char *TYPES[] =
{
    "double",
    "float",
    "int32_t",
    "int64_t",
    "uint32_t",
    "uint64_t",
    "sint32_t",
    "sint64_t",
    "uint32_t",
    "uint64_t",
    "sint32_t",
    "sint64_t",
    "bool",
    "std::string",
    "std::string",
    nullptr,
};

static std::vector<std::string> split_package( const std::string &package )
{
    std::vector<std::string> out;
    std::string current;

    const char *ptr = package.c_str();
    while (true)
    {
        if (*ptr == '.' || *ptr == 0)
        {
            if (!current.empty())
            {
                out.push_back(current);
                current.clear();
            }
            if (*ptr == 0) break;
        }
        else
            current += *ptr;
        ++ptr;
    }
    return out;
}

static std::string get_native_type( int type, const std::string &name, bool is_enum, bool is_repeated, bool is_ptr )
{
    std::string result;
    if (is_repeated)
        result = "std::list<";

    if (!is_enum && type >= TYPE_DOUBLE && type <= TYPE_BYTES)
        result += TYPES[type - TYPE_DOUBLE];
    else
    if (is_enum)
        result += "int32_t";
    else
    {
        if (is_ptr)
            result += "std::shared_ptr<" + name + ">";
        else
            result += name;
    }

    if (is_repeated)
        result += ">";

    return result;
}

static void generate_field( Context &ctx, std::shared_ptr<Field> field )
{
    ctx.header << "    ";
    if (field->type.repeated)
        ctx.header << "std::list<";

    if (field->type.id >= TYPE_DOUBLE && field->type.id <= TYPE_BYTES)
        ctx.header << TYPES[field->type.id - TYPE_DOUBLE];
    else
    if (field->type.eref != nullptr)
        ctx.header << "int32_t";
    else
    {
        if (!field->type.repeated)
            ctx.header << "auto_new<";
        ctx.header << field->type.name;
        if (!field->type.repeated)
            ctx.header << ">";
    }
        //out << "std::shared_ptr<" << field->type.name << ">";
    if (field->type.repeated)
        ctx.header << ">";
    ctx.header << ' ' << field->name;

    if (!field->type.repeated)
    {
        if (field->type.id >= TYPE_DOUBLE && field->type.id <= TYPE_SINT64)
            ctx.header << " = 0;\n";
            else
        if (field->type.id == TYPE_BOOL)
            ctx.header << " = false;\n";
        else
            ctx.header << ";\n";
    }
    else
        ctx.header << ";\n";
}
/*
static void print( Context &ctx, std::shared_ptr<Constant> entity )
{
    out << "    " << entity->name << " = " << entity->value << ";\n";
}

static void print( Context &ctx, std::shared_ptr<Enum> entity )
{
    out << "enum " << entity->name << "\n{" << '\n';

    for (auto it : entity->constants)
        print(out, it);
    out << '}' << '\n';
}*/

static void print_forward( Context &ctx, std::shared_ptr<Message> message )
{
    ctx.header << "class " << message->name << ";\n";
}

static void generate_to_grpc( Context &ctx, std::shared_ptr<Message> message )
{
    //out << "    void to_grpc( std::shared_ptr<" << grpcns << "::" << message->name << "> that ) const {\n";
    ctx.source << "void " << message->name << "::to_grpc( " << ctx.grpcns << "::" << message->name << "& that ) const\n{\n";
    for (auto it : message->fields)
    {
        if (it->type.repeated)
        {
            ctx.source << "\tfor (auto item : " << it->name << ")";
            if (it->type.id == TYPE_COMPLEX)
                ctx.source << " item.to_grpc(*that.add_" << it->name << "());\n";
            else
                ctx.source << " that.add_" << it->name << "(item);\n";
        }
        else
        if (it->type.id == TYPE_COMPLEX)
        {
            if (it->type.mref != nullptr)
                ctx.source << "\tif (" << it->name << ") " << it->name << "->to_grpc(*that.mutable_" << it->name << "());\n";
            else
                ctx.source << "\tthat.set_" << it->name << "( static_cast<" << ctx.grpcns << "::" << it->type.name << ">(" << it->name << "));\n";
        }
        else
            ctx.source << "\tthat.set_" << it->name << "(" << it->name << ");\n";
    }
    if (message->fields.size() == 0) ctx.source << "\t(void) that;\n";
    ctx.source << "}\n";
}

static void generate_from_grpc( Context &ctx, std::shared_ptr<Message> message )
{
    ctx.source << "void " << message->name << "::from_grpc( const " << ctx.grpcns << "::" << message->name << "& that )\n{\n";
    for (auto it : message->fields)
    {
        if (it->type.repeated)
        {
            ctx.source << "\t{\n\t\t" << it->name << ".resize(that." << it->name << "_size());\n\t\tauto it = " << it->name << ".begin();\n";
            ctx.source << "\t\tfor (auto item : that." << it->name << "())";
            if (it->type.id == TYPE_COMPLEX)
                ctx.source << " { it->from_grpc(item); ++it; };\n\t}\n";
            else
                ctx.source << " { *it = item; ++it; };\n\t}\n";
        }
        else
        if (it->type.id == TYPE_COMPLEX)
        {
            if (it->type.mref != nullptr)
            {
                auto native_type = get_native_type(it->type.id, it->type.name, it->type.eref != nullptr, false, false);
                ctx.source << "\t"<< it->name << "->from_grpc( that." << it->name << "() );\n";
            }
            else
                ctx.source << "\t" << it->name << " = static_cast<int32_t>(that." << it->name << "());\n";
        }
        else
            ctx.source << "\t" << it->name << " = that." << it->name << "();\n";
    }
    if (message->fields.size() == 0) ctx.source << "\t(void) that;\n";
    ctx.source << "}\n";
}

static void generate_message_decl( Context &ctx, std::shared_ptr<Message> message )
{
    ctx.header << "struct " << message->name << "\n{" << '\n';

    // fields
    for (auto it : message->fields) generate_field(ctx, it);
    // functions
    ctx.header << "\t" << message->name << "() = default;\n";
    ctx.header << "\t" << message->name << "( " << message->name << "&& ) = default;\n";
    ctx.header << "\t" << message->name << "( const " << message->name << "& ) = default;\n";
    ctx.header << "\t" << message->name << "( const " << ctx.grpcns << "::" << message->name << "& that ) { this->from_grpc(that); };\n";
    ctx.header << "\t" << message->name << " &operator=( const " << message->name << "& that ) = default;\n";
    ctx.header << "\t" << message->name << " &operator=( const " << ctx.grpcns << "::" << message->name << "& that ) { this->from_grpc(that); return *this; };\n";
    ctx.header << "\tvoid to_grpc( " << ctx.grpcns << "::" << message->name << "& that ) const;\n";
    ctx.header << "\tvoid from_grpc( const " << ctx.grpcns << "::" << message->name << "& that );\n";

    ctx.header << "};" << '\n';
}

static void generate_source( Context &ctx, Proto &proto )
{
    ctx.source << "#include \"" << ctx.ifname << "\"\n";

    // begin prettify namespace
    for (auto item : ctx.nspace)
        ctx.source << "namespace " << item << "{\n";
    // functions
    for (auto it : proto.messages)
    {
        generate_from_grpc(ctx, it);
        generate_to_grpc(ctx, it);
    }
    // end prettify namespace
    for (auto item : ctx.nspace)
        ctx.source << "} // namespace " << item << "\n";
}

static void generate_header( Context &ctx, Proto &proto )
{
    auto sentinel = proto.package;
    for (auto &c : sentinel) if (c == '.') c = '_';

    ctx.header << "#ifndef " << sentinel << "_header\n";
    ctx.header << "#define " << sentinel << "_header\n";

    ctx.header << "#include <stdint.h>\n";
    ctx.header << "#include <string>\n";
    ctx.header << "#include <list>\n";
    ctx.header << "#include <memory>\n";
    ctx.header << "#include \"" << ctx.phname << "\"\n";

    ctx.nspace = split_package(proto.package);
    for (auto item : ctx.nspace)
        ctx.grpcns += "::" + item;
#if 0
    // begin GRPC namespace
    for (auto item : ctx.nspace)
        ctx.header << "namespace " << item << "{\n";
    // forward declarations
    for (auto it : proto.messages) print_forward(ctx, it);
    // end GRPC namespace
    for (auto item : ctx.nspace)
        ctx.header << "} // namespace " << item << "\n";
#endif
    // begin prettify namespace
    ctx.nspace.back().append("_");
    for (auto item : ctx.nspace)
        ctx.header << "namespace " << item << "{\n";
    // auto_new
    ctx.header << AUTO_NEW << '\n';
    // forward declarations
    for (auto it : proto.messages) print_forward(ctx, it);
    // messages
    for (auto it : proto.messages) generate_message_decl(ctx, it);
    // end prettify namespace
    for (auto item : ctx.nspace)
        ctx.header << "} // namespace " << item << "\n";

    ctx.header << "#endif // " << sentinel << "_header\n";
}

static std::string replace_ext( const std::string &name, const std::string &ext )
{
    auto spos = name.rfind("/");
    auto dpos = name.rfind(".");
    if (dpos != std::string::npos && (spos == std::string::npos || spos < dpos))
        return name.substr(0, dpos) + ext;
    else
        return name + ext;
}

static std::string filename( const std::string &name, bool ext = true )
{
    std::string out;
    auto pos = name.rfind("/");
    if (pos == std::string::npos)
        out = name;
    else
        out = name.substr(pos+1);

    if (!ext && (pos = out.rfind(".")) != std::string::npos)
        out = out.substr(0, pos);
    return out;
}

int main( int argc, char **argv )
{
    if (argc != 4)
    {
        std::cerr << "Usage: example_grpc_facade <proto file> <header> <source>\n";
        return 1;
    }

    std::string hfname = argv[2];
    std::string sfname = hfname;
    if (hfname.empty() || hfname.back() == '/') hfname += "out.hh";
    std::string ifname = filename(hfname);
    sfname = replace_ext(hfname, ".cc");
    std::string phname = filename(argv[1], false) + ".pb.h";

    std::cout << " Proto: " << argv[1] << " (" << phname << ")\n";
    std::cout << "Header: " << hfname << " (" << ifname << ")\n";
    std::cout << "Source: " << sfname << '\n';

    std::ifstream input(argv[1]);
    if (!input.good()) return 1;

    std::ofstream header(hfname);
    if (!header.good()) return 1;
    std::ofstream source(sfname);
    if (!source.good()) return 1;

    Context context{header, source, ifname, phname, "", {} };

    Proto tree;
    Proto::parse(tree, input, argv[1]);
    generate_header(context, tree);
    generate_source(context, tree);

    return 0;
}