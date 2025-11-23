#include "metadata_parser.hpp"
#include "nlohmann_json.hpp"


#include <cstdio>
#include <iostream> 
#include <fstream>  
#include <filesystem>


namespace fs = std::filesystem;
using json = nlohmann::json;


#define HEADER_SIZE 18

// field name to index mapping
#define CORD_UID    0
#define SHA         1
#define TITLE       3
#define ABSTRACT    8




// Public funcs


int MetadataParser::metadata_stats()
{
    std::ifstream file(data_path + "/metadata.csv"); 
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


    std::vector<std::string> parsed_line;

    parse_csv_line(line, parsed_line);


    uint32_t i = 1;
    for (const auto& p : parsed_line) {
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


    uint32_t found_pdf = 0;
    uint32_t not_found = 0;


    int a = 1;
    while (getline(file, line)) 
    {
        parse_csv_line(line, parsed_line);

        freq[parsed_line.size()]++;

        no_papers++;

       if (parsed_line.size() > 14 && !parsed_line[14].empty()) { no_pdf++; }
       if (parsed_line.size() > 15 && !parsed_line[15].empty()) { no_xml++; }
       if (parsed_line.size() > 16 && !parsed_line[16].empty()) { no_full_text++; }

        // finding papers
        if (parsed_line.size() == HEADER_SIZE) {
            std::string path = find_fulltext(parsed_line[SHA]);
            if (path.empty()) {
                not_found++;
            } else {
                found_pdf++;
                if (a == 1000) {
                    std::string body_text;
                    extract_body_text(path, body_text);
                    std::cout << "BODY TEST: \n\n" << body_text << "\n\n";
                }
            }
        }


        std::cout << "ABSTRACT TEST: \n";
        if (a % 10000 == 0) {
            std::cout << parsed_line[ABSTRACT] << "\n\n";
        }
        a++;

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

    std::cout << "Found pdf: " << found_pdf << '\n';
    std::cout << "Not Found: " << not_found << '\n';


    file.close();
    return 0;
}


// main parser

int MetadataParser::metadata_parse() 
{
    std::ifstream file(data_path + "/metadata.csv"); 
    if (!file.is_open()) {
        std::cout << "can't open metadata.csv\n";
    }
    std::string line;
        
    // Read header
    if (!getline(file, line)) {
        std::cerr << "Error: Empty metadata file" << '\n';
        return -1;
    }

    std::vector<std::string> parsed_line;

    parse_csv_line(line, parsed_line);

    std::cout << "Parsed Header: ";
    for(const auto& p : parsed_line) {
        std::cout << p << ' ';
    }
    std::cout << '\n';


    std::cout << "Reading metadata..." << '\n';
    
    // read all the csv lines
    int a = 1;
    while (getline(file, line)) 
    {
        // parse them into vec of strs
        parse_csv_line(line, parsed_line);

        // find the file related to the sha id

        if (a % 10000 == 0) {
            std::string path = find_fulltext(parsed_line[SHA]);

            // build file for Text processing
            std::string body_text = "ABSTRACT:\n";
            body_text += parsed_line[ABSTRACT];
            body_text += "\nBODY:\n";
            extract_body_text(path, body_text); 
            
            std::cout << body_text << '\n';
        }
        a++;
    }


    file.close();
    return 0;
}



// Private funcs

// Parse CSV line handling quotes and commas
void MetadataParser::parse_csv_line(const std::string& line, std::vector<std::string>& parsed_line) 
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


std::string MetadataParser::find_fulltext(std::string& sha)
{
    if (sha.empty()) { return ""; }
    
    // SHA might have multiple values separated by "; "
    std::string first_sha = sha;
    size_t semi_pos = sha.find(';');
    if (semi_pos != std::string::npos) {
        first_sha = sha.substr(0, semi_pos);
        // Trim whitespace
        first_sha.erase(0, first_sha.find_first_not_of(" \t"));
        first_sha.erase(first_sha.find_last_not_of(" \t") + 1);
    }
    
    // Search PDF JSONs
    std::vector<std::string> pdf_search_paths = {
        data_path + "/comm_use_subset/pdf_json/" + first_sha + ".json",
        data_path + "/noncomm_use_subset/pdf_json/" + first_sha + ".json",
        data_path + "/custom_license/pdf_json/" + first_sha + ".json",
        data_path + "/biorxiv_medrxiv/pdf_json/" + first_sha + ".json"
    };
    
    for (const auto& path : pdf_search_paths) {
        if (fs::exists(path)) {
            return path;
        }
    }
    
    return "";
}

void MetadataParser::extract_body_text(const std::string& file_path, std::string& body_text) 
{
    try {
        std::ifstream file(file_path);
        json data = json::parse(file);
        
        
        // Extract only from body_text
        if (data.contains("body_text") && data["body_text"].is_array()) {
            for (const auto& section : data["body_text"]) {
                if (section.contains("text") && section["text"].is_string()) {
                    body_text += section["text"].get<std::string>() + " ";
                }
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error reading JSON file: " << e.what() << '\n';
        return;
    }
}


