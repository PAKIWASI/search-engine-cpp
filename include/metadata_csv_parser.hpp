#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

/*
 * paper structure:
 
    cord_uid, 
    sha, 
    source_x, 
    title, 
    doi, 
    pmcid, 
    pubmed_id, 
    license,
    abstract, 
    publish_time, 
    authors, 
    journal, 
    Microsoft Academic Paper ID, 
    WHO #Covidence, 
    has_pdf_p, 
    se, 
    has_pmc_xml_parse, 
    full_text_file, 
    url
*/


class metadataParser_CSV {
private:


};


// Parse CSV line handling quotes and commas
void parseCSVLine(const std::string& line, std::vector<std::string>& parsed_line) 
{
    parsed_line.clear();
    std::string field;
    bool inQuotes = false;
    
    for (char c : line) {
        if (c == ';') { continue; }
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ',' && !inQuotes) {
            parsed_line.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    parsed_line.push_back(field);
}

int metadata_parse_run()
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
    
    std::vector<std::string> parsed_lines;
    parseCSVLine(line, parsed_lines);
    std::cout << "headers: ";
    for (const auto& p : parsed_lines) {
        std::cout << p << ", ";
    }
    std::cout << '\n';
    
    std::cout << "Loading metadata..." << '\n';

    getline(file,line);
    parseCSVLine(line, parsed_lines);
    for (const auto& p : parsed_lines) {
        std::cout << p << ", ";
    }
    
    
    /*
    // Read papers
    for (int i = 0; i < 10; i++) {
        if (!getline(file, line)) {
            std::cerr << "error reading line: " << i << '\n';
            return -1;
        }

        std::vector<std::string> parsed_lines;
        parseCSVLine(line, parsed_lines);
        for (const auto& p: parsed_lines) {
            std::cout << p << ", ";
        }
        std::cout << '\n';
    }
         */


    file.close();
    return 0;
}
