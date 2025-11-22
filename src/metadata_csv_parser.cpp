#include "metadata_csv_parser.hpp"

#include <cstdint>  
#include <iostream> 
#include <fstream>  



// Parse CSV line handling quotes and commas
void parseCSVLine(const std::string& line, std::vector<std::string>& parsed_line) 
{
    parsed_line.clear();

    std::string temp;
    bool is_quotes = false;

    for (const char c : line) {
        if (c == '"') {
            is_quotes = !is_quotes;
        } 
        else if (c == ',' && !is_quotes) {
            parsed_line.push_back(temp);
            temp.clear();
        }
        else {
            temp += c;
        }
    }
    parsed_line.push_back(temp);
}

int metadata_parse_stats()
{
    std::ifstream file("data/2020-04-10/metadata.csv"); 
    if (!file.is_open()) {
        std::cout << "can't open metadata.csv\n";
    }
    std::string line;
        
    // Read header
    if (!getline(file, line)) {
        std::cerr << "Error: Empty metadata file" << '\n';
        return -1;
    }
    std::cout << "Line: " << line << '\n';


    std::vector<std::string> parsed_lines;

    parseCSVLine(line, parsed_lines);


    uint32_t i = 1;
    for (const auto& p : parsed_lines) {
        std::cout << i << " -> " << p << '\n';
        i++;
    }

    uint32_t num_fields = i - 1;
    std::cout << "No of fields: " << num_fields << '\n';

    std::cout << "Loading metadata..." << '\n';

    
    uint32_t no_papers = 0;
    uint32_t no_pdf = 0;
    uint32_t no_xml = 0;
    uint32_t no_full_text = 0;

    int freq[20] = {0};

    while (getline(file, line)) 
    {
        parseCSVLine(line, parsed_lines);

        freq[parsed_lines.size()]++;

        no_papers++;

       if (parsed_lines.size() > 14 && !parsed_lines[14].empty()) { no_pdf++; }
       if (parsed_lines.size() > 15 && !parsed_lines[15].empty()) { no_xml++; }
       if (parsed_lines.size() > 16 && !parsed_lines[16].empty()) { no_full_text++; }

    }
    std::cout << "No of papers: " << no_papers << '\n';
    std::cout << "No of pdfs: " << no_pdf << '\n';
    std::cout << "No of xmls: " << no_xml << '\n';
    std::cout << "No of full texts: " << no_full_text << '\n';

    std::cout << "size frequencies: \n";
    for (int i = 0; i < 20; i++) {
        std::cout << freq[i] << ' ';
    }
    std::cout << '\n';

    file.close();
    return 0;
}


