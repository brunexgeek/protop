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