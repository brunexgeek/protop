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

#ifndef PROTOP_API
#define PROTOP_API

#include <string>
#include <list>
#include <unordered_map>
#include <iostream>
#include <memory>

namespace protop {

enum FieldType
{
    TYPE_DOUBLE   = 6,
    TYPE_FLOAT    = 7,
    TYPE_INT32    = 8,
    TYPE_INT64    = 9,
    TYPE_UINT32   = 10,
    TYPE_UINT64   = 11,
    TYPE_SINT32   = 12,
    TYPE_SINT64   = 13,
    TYPE_FIXED32  = 14,
    TYPE_FIXED64  = 15,
    TYPE_SFIXED32 = 16,
    TYPE_SFIXED64 = 17,
    TYPE_BOOL     = 18,
    TYPE_STRING   = 19,
    TYPE_BYTES    = 20,
    TYPE_COMPLEX  = 21,
};

class Message;
class Enum;

struct TypeInfo
{
    FieldType id;
    std::string name;
    std::string package;
    std::shared_ptr<Message> mref;
    std::shared_ptr<Enum> eref;
    bool repeated = false;
};

enum class OptionType
{
    IDENTIFIER,
    STRING,
    INTEGER,
    BOOLEAN
};

struct OptionEntry
{
    std::string name;
    OptionType type;
    std::string value;
    int line;
};

typedef std::unordered_map<std::string, OptionEntry> OptionMap;

struct Field
{
    TypeInfo type;
    std::string name;
    int index = 0;
    OptionMap options;
};

struct Constant
{
    std::string name;
    int value = 0;
    OptionMap options;
};

struct Enum
{
    std::list<std::shared_ptr<Constant>> constants;
    std::string name;
    std::string qname;
    OptionMap options;
};

struct Message
{
    std::list<std::shared_ptr<Field>> fields;
    std::string name;
    std::string qname;
    OptionMap options;
};

struct Procedure
{
    std::string name;
    TypeInfo request;
    TypeInfo response;
    OptionMap options;
};

struct Service
{
    std::string name;
    std::list<std::shared_ptr<Procedure>> procs;
    OptionMap options;
};

class Proto
{
    public:
        std::list<std::shared_ptr<Message>> messages;
        std::list<std::shared_ptr<Service>> services;
        std::list<std::shared_ptr<Enum>> enums;
        OptionMap options;
        std::string fileName;
        std::string package;
        std::string syntax;

        static void parse( Proto &tree, std::istream &input, const std::string &fileName = "");
};

} // protop

#endif // PROTOP_API