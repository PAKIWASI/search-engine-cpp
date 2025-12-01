#include "inverted_index.hpp"
#include "metadata_parser.hpp"

#include <string>


// WARN: will take aprox 8.3 hours on my cpu to get 50k docs on a single thread


int main()
{
    /*
    // builder phase
    const std::string data_path = "data/2020-04-10"; 

    MetadataParser parser(data_path);

    parser.metadata_parse();
    */

    InvertedIndex ii;

    ii.load_barrel("indices/", 0);

    ii.print_statistics();

    return 0;
}
