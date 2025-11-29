#include "metadata_parser.hpp"

#include <string>


// WARN: will take aprox 8.3 hours on my cpu to get 50k docs on a single thread


int main()
{
    // builder phase
    const std::string data_path = "data/2020-04-10"; 

    MetadataParser parser(data_path);

    parser.metadata_parse();



    return 0;
}
