#include "metadata_parser.hpp"


int main()
{
    const std::string data_path = "data/2020-04-10"; 

    MetadataParser parser(data_path);


    parser.metadata_parse();

    return 0;
}
