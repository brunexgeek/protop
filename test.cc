#include "protop.hh"
#include <fstream>

using namespace protogen;

int main( int argc, char **argv )
{
    if (argc != 2) return 1;

    std::ifstream input(argv[1]);
    if (!input.good()) return 1;

    Proto3 tree;
    Proto3::parse(tree, input, argv[1]);
    std::cout << tree;

    return 0;
}