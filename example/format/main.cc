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

using namespace protop;

static const char *TYPES[] =
{
    "double",
    "float",
    "int32",
    "int64",
    "uint32",
    "uint64",
    "sint32",
    "sint64",
    "fixed32",
    "fixed64",
    "sfixed32",
    "sfixed64",
    "bool",
    "string",
    "bytes",
    nullptr,
};

static void print( std::ostream &out, std::shared_ptr<Field> field )
{
    out << "    ";
    if (field->type.repeated)
        out << "repeated ";
    if (field->type.id >= TYPE_DOUBLE && field->type.id <= TYPE_BYTES)
        out << TYPES[field->type.id - TYPE_DOUBLE];
    else
        out << field->type.name;
    out << ' ' << field->name << " = " << field->index << ";\n";
}

static void print( std::ostream &out, std::shared_ptr<Constant> entity )
{
    out << "    " << entity->name << " = " << entity->value << ";\n";
}

static void print( std::ostream &out, std::shared_ptr<Enum> entity )
{
    out << "enum " << entity->name << "\n{" << '\n';

    for (auto it : entity->constants)
        print(out, it);
    out << '}' << '\n';
}

static void print( std::ostream &out, std::shared_ptr<Message> message )
{
    out << "message " << message->name << "\n{" << '\n';
    for (auto it : message->fields) print(out, it);
    out << '}' << '\n';
}

static void print( std::ostream &out, std::shared_ptr<Procedure> entity )
{
    out << "    rpc " << entity->name << "(" << entity->request.name << ")"
        << " returns (" << entity->response.name << ");\n";
}

static void print( std::ostream &out,  std::shared_ptr<Service> entity )
{
    out << "service " << entity->name << "\n{" << '\n';
    for (auto it : entity->procs) print(out, it);
    out << '}' << '\n';
}

static void print( std::ostream &out, Proto &proto )
{
    out << "syntax = \"proto3\";\n";
    out << "package " << proto.package << ";\n";
    for (auto it : proto.messages) print(out, it);
    for (auto it : proto.enums) print(out, it);
    for (auto it : proto.services) print(out, it);
}

int main( int argc, char **argv )
{
    if (argc != 2) return 1;

    std::ifstream input(argv[1]);
    if (!input.good()) return 1;

    Proto tree;
    Proto::parse(tree, input, argv[1]);
    print(std::cout, tree);

    return 0;
}