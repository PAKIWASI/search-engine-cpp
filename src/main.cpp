#include "forward_index.hpp"
#include "metadata_parser.hpp"
#include <string>


int main()
{
    /*
    const std::string data_path = "data/2020-04-10"; 

    MetadataParser parser(data_path);

    parser.metadata_parse();
    */

    ForwardIndex fi;

    std::string input_path = "indices/forward_index_cordR1.bin";
    fi.load_from_file(input_path);

    fi.print_statistics();

    return 0;
}
