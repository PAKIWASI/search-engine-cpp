#include "inverted_index.hpp"
#include "metadata_parser.hpp"


// WARN: will take aprox 8.3 hours on my cpu to get 50k docs on a single thread


int main()
{
    /*
    // builder phase
    const std::string data_path = "data/2020-04-10"; 

    MetadataParser parser(data_path);

    parser.metadata_parse();
    */

    std::string path = "indices/";
    InvertedIndex i(path);

    i.load_barrel(0);
    i.print_statistics();

    i.load_barrel(11000);
    i.print_statistics();

    i.load_barrel(50000);
    i.print_statistics();


    return 0;
}
