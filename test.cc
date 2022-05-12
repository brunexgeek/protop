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

#include "source/parser.hh"
#include <fstream>

using namespace protop;

int main( int argc, char **argv )
{
    if (argc != 2) return 1;

    std::ifstream input(argv[1]);
    if (!input.good()) return 1;

    Proto3 tree;
    Proto3::parse(tree, input, argv[1]);
    tree.print(std::cout);

    return 0;
}