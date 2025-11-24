#include "metadata_parser.hpp"
#include "nlohmann_json.hpp"
#include "text_processor.hpp"


#include <cstdio>
#include <iostream> 
#include <fstream>  
#include <filesystem>
#include <string>


namespace fs = std::filesystem;
using json = nlohmann::json;


#define HEADER_SIZE 18

// field name to index mapping
#define CORD_UID    0
#define SHA         1   // for pdfs
#define TITLE       3
#define PMC_ID      5   // for xmls
#define ABSTRACT    8



// Public funcs


// main parser
int MetadataParser::metadata_parse() 
{
    std::ifstream file(data_path + "/metadata.csv"); 
    if (!file.is_open()) {
        std::cout << "Error: can't open metadata.csv\n";
        return -1;
    }

    std::string line;
        
    // Read header
    if (!getline(file, line)) {
        std::cerr << "Error: Empty metadata file" << '\n';
        return -1;
    }

    std::vector<std::string> parsed_line;
    parse_csv_line(line, parsed_line);

    // Initialize TextProcessor (python script)
    TextProcessor text_processor(
        "python/.venv/bin/python3", 
        "python/lemmatizer_2.py"
    );
    
    std::cout << "\nStarting paper processing...\n";
    
    int paper_count = 0;
    int processed_count = 0;
    int failed_count = 0;
    int skipped_count = 0;
    
    // Read all the csv lines
    while (getline(file, line)) 
    {
        paper_count++;
        
        // Parse CSV line
        parse_csv_line(line, parsed_line);

        // Build text for processing
        std::string body_text;

        // add title
        if (parsed_line.size() > TITLE && !parsed_line[TITLE].empty())
        {
            body_text += parsed_line[TITLE];
            body_text += "\n\n";
        }
        
        // add abstract
        if (parsed_line.size() > ABSTRACT && !parsed_line[ABSTRACT].empty()) 
        {
            body_text += parsed_line[ABSTRACT];
            body_text += "\n\n";
        }
        
        // try to find PDFs first (about 38k of em)
        std::string path_pdf = find_fulltext_pdf(parsed_line[SHA]);
        
        // if no PDF, try XML (only finding about 800 of em --something wrong)
        if (path_pdf.empty() && parsed_line.size() > PMC_ID) {
            std::string path_xml = find_fulltext_xml(parsed_line[PMC_ID]);
            if (!path_xml.empty()) {
                extract_body_text(path_xml, body_text);
            }
        } else if (!path_pdf.empty()) {
            extract_body_text(path_pdf, body_text);
        }
        
        // process with python lemmatizer
        if (!body_text.empty()) {
            bool success = text_processor.process_text(body_text);
            if (success) {
                processed_count++;
            } else {
                failed_count++;
                if (failed_count <= 5) {  // DEBUG: show first few failures
                    std::cerr << "Warning: Failed to process paper " << paper_count << "\n";
                }
            }
        } else {
            skipped_count++;
        }
        
        // progress
        if (paper_count % 1 == 0) {
            std::cout << "Progress: " << paper_count << " papers, "
                      << processed_count << " processed, "
                      << text_processor.get_lexicon_size() << " unique terms"
                      << '\n';
        }
        
        // limit for testing 
        if (paper_count >= 20) { break; }
    }

    std::cout << "Total papers read:       " << paper_count << "\n";
    std::cout << "Successfully processed:  " << processed_count << "\n";
    std::cout << "Failed:                  " << failed_count << "\n";
    std::cout << "Skipped (no text):       " << skipped_count << "\n";
    std::cout << "Unique terms in lexicon: " << text_processor.get_lexicon_size() << "\n";
    
    // Print top terms
    text_processor.print_top_words(50);
    
    // Save lexicon to file
    std::string output_path = "indices/lexicon_cordR1.csv";
    text_processor.save_lexicon(output_path);

    file.close();
    return 0;
}


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
    uint32_t found_xml = 0;
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
            std::string path = find_fulltext_pdf(parsed_line[SHA]);
            if (path.empty()) {
                
                // pdf not available, look for xml
                std::string path_xml = find_fulltext_xml(parsed_line[PMC_ID]);
                if (!path_xml.empty()) {
                    found_xml++;            
                }
                else {
                    not_found++;
                }

            } 
            else {
                found_pdf++;
                if (a == 1000) {
                    std::string body_text;
                    extract_body_text(path, body_text);
                    std::cout << "BODY TEST: \n\n" << body_text << "\n\n";
                }
            }
        }


        if (a % 10000 == 0) {
            std::cout << "ABSTRACT TEST: \n";
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
    std::cout << "Found xml: " << found_xml << '\n';
    std::cout << "Not Found: " << not_found << '\n';


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


std::string MetadataParser::find_fulltext_pdf(std::string& sha)
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

std::string MetadataParser::find_fulltext_xml(std::string& pmcid)
{
    if (pmcid.empty()) { return ""; }
    
    // Search PDF JSONs
    std::vector<std::string> xml_search_paths = {
        data_path + "/comm_use_subset/pmc_json/" + pmcid + ".xml.json",
        data_path + "/noncomm_use_subset/pmc_json/" + pmcid + ".xml.json",
        data_path + "/custom_license/pmc_json/" + pmcid + ".xml.json",
        //data_path + "/biorxiv_medrxiv/pmc_json/" + pmcid + ".json"
    };
    
    for (const auto& path : xml_search_paths) {
        if (fs::exists(path)) {
            return path;
        }
    }
    
    return "";
}

void MetadataParser::extract_body_text(const std::string& file_path, std::string& body_text) 
{
    if (file_path.empty()) { return; }
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





