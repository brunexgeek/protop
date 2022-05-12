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

#ifndef PROTOGEN_PROTO3_HH
#define PROTOGEN_PROTO3_HH


#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>


namespace protogen {


#ifndef PROTOGEN_FIELD_TYPES
#define PROTOGEN_FIELD_TYPES

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

#endif // PROTOGEN_FIELD_TYPES


class Message;
class Enum;

struct TypeInfo
{
    FieldType id;
    std::string qname;
    Message *mref = nullptr;
    Enum *eref = nullptr;
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
    int index;
    OptionMap options;

    Field();
};

struct Constant
{
    std::string name;
    int value = 0;
    OptionMap options;
};

struct Enum
{
    std::vector<Constant*> constants;
    std::string name;
    std::string qname;
    OptionMap options;
};

struct Message
{
    std::vector<Field> fields;
    std::string name;
    std::string qname;
    OptionMap options;

    //std::string qualifiedName() const;
    //void splitPackage( std::vector<std::string> &out );
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
    std::vector<Procedure*> procs;
    OptionMap options;
};

class Proto3
{
    public:
        std::vector<Message*> messages;
        std::vector<Service*> services;
        std::vector<Enum*> enums;
        OptionMap options;
        std::string fileName;
        std::string package;

        ~Proto3();
        void print( std::ostream &out ) const;
        static void parse( Proto3 &tree, std::istream &input, const std::string &fileName = "");
};

class Token;

class exception : public std::exception
{
    public:
        int line, column;

        exception( const std::string &message, int line = 1, int column = 1 );
        virtual ~exception();
        const char *what() const throw();
        const std::string cause() const;
    private:
        std::string message;
};

} // protogen

std::ostream &operator<<( std::ostream &out, protogen::Proto3 &proto );
std::ostream &operator<<( std::ostream &out, protogen::Message &message );
std::ostream &operator<<( std::ostream &out, protogen::Field &field );


#endif // PROTOGEN_PROTO3_HH