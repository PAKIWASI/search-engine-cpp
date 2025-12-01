#include "metadata_parser.hpp"
#include "inverted_index.hpp"
#include "nlohmann_json.hpp"
#include "text_processor.hpp"
#include "lexicon.hpp"
#include "forward_index.hpp"


#include <iostream> 
#include <fstream>  


namespace fs = std::filesystem;
using json = nlohmann::json;


#define HEADER_SIZE 18

// field name to index mapping
#define CORD_UID    0
#define SHA         1   // for pdfs
#define TITLE       3
#define PMC_ID      5   // for xmls
#define ABSTRACT    8


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
    if (!std::getline(file, line)) {
        std::cerr << "Error: Empty metadata file" << '\n';
        return -1;
    }

    // main vector for storing each parsed csv line
    std::vector<std::string> parsed_line;

    // parse the header (not needed but do it for vibes)
    parse_csv_line(line, parsed_line);

    // initialize Lexicon
    Lexicon lexicon;

    // store data of curr doc in temp_lex
    std::unordered_map<std::string, WordData> temp_lex; 
    

    // initialize TextProcessor with reference to lexicon
    TextProcessor text_processor(
        lexicon,
        "python/.venv/bin/python3", 
        "python/lemmatizer_daemon.py"
    );
    
    // initialize Forward Index
    ForwardIndex forward_index;

    // initialize Inverted Index
    InvertedIndex inverted_index;
    

    std::cout << "\nStarting paper processing...\n";
    
    uint32_t paper_count = 0;
    uint32_t lemmatize_count = 0;
    uint32_t failed_count = 0;
    uint32_t skipped_count = 0;
    uint32_t pdf_count = 0;
    uint32_t xml_count = 0;
    uint32_t not_found = 0;
    

    // read all the csv lines
    while (std::getline(file, line)) 
    {
        paper_count++;
        
        // parse csv line
        parse_csv_line(line, parsed_line);

        // for testing ranges
        //if (paper_count < 495) { continue; }
       

        // build text as a string
        std::string full_text;

        // add title
        if (parsed_line.size() > TITLE && !parsed_line[TITLE].empty())
        {
            full_text += parsed_line[TITLE];
            full_text += ' ';
        }

        // add abstract
        if (parsed_line.size() > ABSTRACT && !parsed_line[ABSTRACT].empty()) 
        {
            full_text += parsed_line[ABSTRACT];
            full_text += ' ';
        }
        
        // try to find pdfs first (about 38k of em)
        std::string path_pdf = find_fulltext_pdf(parsed_line[SHA]);

        // if no pdf, try xml (only finding about 800 of em, something wrong with pmcid?)
        if (path_pdf.empty() && parsed_line.size() > PMC_ID) {
            std::string path_xml = find_fulltext_xml(parsed_line[PMC_ID]);
            if (!path_xml.empty()) {
                extract_body_text(path_xml, full_text);
                xml_count++;
            }
            else {  // no pdf no xml
                not_found++;
                std::cout << "Paper No: " << paper_count << " full_text not found\n";
            }
        } 
        else if (!path_pdf.empty()) {
            extract_body_text(path_pdf, full_text);
            pdf_count++;
        }


        if (!full_text.empty()) { // valid papers
            
            // lemmatize text
            bool success_lemma = text_processor.lemmatize_text(full_text, temp_lex);

            if (success_lemma) {

                lemmatize_count++;

                // get cord_uid for this paper
                std::string cord_uid = parsed_line.size() > CORD_UID ? parsed_line[CORD_UID] : "";
                // Build forward index using temp_lex
                uint32_t doc_id = forward_index.add_document(cord_uid, temp_lex);

                // Build inverted index
                // get word_id, freq from temp lex for each doc
                // get docid from forward_index
                inverted_index.add_document(doc_id, temp_lex);

            } 
            else {
                failed_count++;
                std::cout << "Paper No: " << paper_count << " Lemmatization Failed\n";
            }
        } 
        else {
            skipped_count++;
            std::cout << "Paper No: " << paper_count << " Full text empty\n";
        }

        // progress
        if (paper_count % 10 == 0) {
            std::cout << "Progress: " << paper_count << " papers, "
                      << lemmatize_count << " processed, "
                      << text_processor.get_lexicon_size() << " unique terms\n";
        }
        
        // limit for testing 
        if (paper_count >= 1000) { break; }
    }


    std::cout << "\nPROCESSING SUMMARY\n";
    std::cout << "Total papers read:       " << paper_count << "\n";
    std::cout << "Successfully Lemmatized: " << lemmatize_count << "\n";
    std::cout << "Failed:                  " << failed_count << "\n";
    std::cout << "Skipped (no text):       " << skipped_count << "\n";
    std::cout << "Unprocessable:           " << skipped_count << "\n";
    std::cout << "PDF's Found:             " << pdf_count << "\n";
    std::cout << "XML's Found:             " << xml_count << "\n";
    std::cout << "Not Found (No PDF, XML): " << not_found << "\n";
    std::cout << "Unique terms in lexicon: " << lexicon.size() << "\n";

    // Print top terms
    lexicon.print_top_words(50);
    
    // Print forward index stats
    forward_index.print_statistics();

    // print inverted_index stats
    inverted_index.print_statistics();
    

    // Save lexicon to file
    std::string lexicon_path = "indices/lexicon_cordR1.bin";
    lexicon.save_to_file_binary(lexicon_path);
    std::string lexicon_text = "indices/lexicon_text.csv";
    lexicon.save_to_file_csv(lexicon_text);
    
    // Save forward index to file (binary form)
    std::string forward_index_path = "indices/forward_index_cordR1.bin";
    forward_index.save_to_file(forward_index_path);
    std::string forward_text = "indices/forward_index_text.txt";
    forward_index.save_as_text(forward_text, lexicon.get_reverse_lexicon());

    // save inverted_index to file (binary)
    std::string inverted_index_path = "indices/inverted_index_cordR1.bin";
    inverted_index.save_to_file(inverted_index_path);
    std::string inverted_text = "indices/inverted_index_text.txt";
    inverted_index.save_as_text(inverted_text,lexicon.get_reverse_lexicon());


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


void MetadataParser::copy_file(const std::string& in_path)
{

    std::string out_path = "sample/data/";

    std::string temp;
    for (size_t i = in_path.size() - 1; in_path[i] != '/'; i--) {
        temp += in_path[i]; 
    }
    std::reverse(temp.begin(), temp.end());

    out_path += temp;

    fs::copy_file(in_path, out_path); 
}

int MetadataParser::metadata_stats()
{
    std::ifstream file(data_path + "/metadata.csv"); 
    if (!file.is_open()) {
        std::cout << "Error: can't open metadata.csv\n";
        return -1;
    }

    std::string line;
        
    // Read header
    if (!std::getline(file, line)) {
        std::cerr << "Error: Empty metadata file" << '\n';
        return -1;
    }

    std::vector<std::string> parsed_line;
    parse_csv_line(line, parsed_line);

    std::cout << "\n=== HEADER FIELDS ===\n";
    for (size_t i = 0; i < parsed_line.size(); i++) {
        std::cout << i << " -> " << parsed_line[i] << '\n';
    }
    std::cout << "Total fields: " << parsed_line.size() << "\n\n";

    // Statistics counters
    uint32_t total_papers = 0;
    uint32_t has_title = 0;
    uint32_t has_abstract = 0;
    uint32_t has_sha = 0;
    uint32_t has_pmcid = 0;
    
    uint32_t found_pdf = 0;
    uint32_t found_xml = 0;
    uint32_t found_both = 0;
    uint32_t found_neither = 0;
    
    uint32_t has_fulltext = 0;  // title + abstract + (pdf or xml)
    uint32_t has_partial = 0;   // title + abstract only
    uint32_t has_minimal = 0;   // title or abstract only
    uint32_t has_nothing = 0;   // no title, no abstract, no full text
    
    // Text length statistics
    uint64_t total_title_chars = 0;
    uint64_t total_abstract_chars = 0;
    size_t min_title_len = SIZE_MAX;
    size_t max_title_len = 0;
    size_t min_abstract_len = SIZE_MAX;
    size_t max_abstract_len = 0;
    
    // Field size frequency (for debugging)
    std::map<size_t, uint32_t> field_count_freq;
    
    std::cout << "Processing papers...\n";
    
    while (std::getline(file, line)) 
    {
        parse_csv_line(line, parsed_line);
        total_papers++;

        
        
        // Track field count distribution
        field_count_freq[parsed_line.size()]++;
        
        // Check what's available
        bool has_title_text = (parsed_line.size() > TITLE && !parsed_line[TITLE].empty());
        bool has_abstract_text = (parsed_line.size() > ABSTRACT && !parsed_line[ABSTRACT].empty());
        bool has_sha_field = (parsed_line.size() > SHA && !parsed_line[SHA].empty());
        bool has_pmcid_field = (parsed_line.size() > PMC_ID && !parsed_line[PMC_ID].empty());
        
        if (has_title_text) {
            has_title++;
            size_t len = parsed_line[TITLE].size();
            total_title_chars += len;
            min_title_len = std::min(min_title_len, len);
            max_title_len = std::max(max_title_len, len);
        }
        
        if (has_abstract_text) {
            has_abstract++;
            size_t len = parsed_line[ABSTRACT].size();
            total_abstract_chars += len;
            min_abstract_len = std::min(min_abstract_len, len);
            max_abstract_len = std::max(max_abstract_len, len);
        }
        
        if (has_sha_field) has_sha++;
        if (has_pmcid_field) has_pmcid++;
        
        // Check for PDF and XML files
        bool pdf_exists = false;
        bool xml_exists = false;
        
        if (has_sha_field) {
            std::string path_pdf = find_fulltext_pdf(parsed_line[SHA]);
            if (!path_pdf.empty()) {
                pdf_exists = true;
                found_pdf++;
            }
        }
        
        if (has_pmcid_field) {
            std::string path_xml = find_fulltext_xml(parsed_line[PMC_ID]);
            if (!path_xml.empty()) {
                xml_exists = true;
                found_xml++;
            }
        }
        
        // Count combinations
        if (pdf_exists && xml_exists) found_both++;
        if (!pdf_exists && !xml_exists) found_neither++;
        
        // Categorize paper completeness
        bool has_body = pdf_exists || xml_exists;
        
        if (has_title_text && has_abstract_text && has_body) {
            has_fulltext++;
        } else if (has_title_text && has_abstract_text) {
            has_partial++;
        } else if (has_title_text || has_abstract_text) {
            has_minimal++;
        } else {
            has_nothing++;
        }
        
        // Progress indicator
        if (total_papers % 5000 == 0) {
            std::cout << "  Processed " << total_papers << " papers...\n";
        }
    }
    
    file.close();
    
    // Print comprehensive statistics
    std::cout << "\n";
    std::cout << "       METADATA STATISTICS REPORT      \n";
    
    std::cout << "=== BASIC COUNTS ===\n";
    std::cout << "Total papers:           " << total_papers << "\n";
    std::cout << "Papers with title:      " << has_title 
              << " (" << (100.0 * has_title / total_papers) << "%)\n";
    std::cout << "Papers with abstract:   " << has_abstract 
              << " (" << (100.0 * has_abstract / total_papers) << "%)\n";
    std::cout << "Papers with SHA:        " << has_sha 
              << " (" << (100.0 * has_sha / total_papers) << "%)\n";
    std::cout << "Papers with PMC_ID:     " << has_pmcid 
              << " (" << (100.0 * has_pmcid / total_papers) << "%)\n\n";
    
    std::cout << "=== FULL TEXT AVAILABILITY ===\n";
    std::cout << "Papers with PDF:        " << found_pdf 
              << " (" << (100.0 * found_pdf / total_papers) << "%)\n";
    std::cout << "Papers with XML:        " << found_xml 
              << " (" << (100.0 * found_xml / total_papers) << "%)\n";
    std::cout << "Papers with both:       " << found_both 
              << " (" << (100.0 * found_both / total_papers) << "%)\n";
    std::cout << "Papers with neither:    " << found_neither 
              << " (" << (100.0 * found_neither / total_papers) << "%)\n";
    std::cout << "Papers with any body:   " << (found_pdf + found_xml - found_both)
              << " (" << (100.0 * (found_pdf + found_xml - found_both) / total_papers) << "%)\n\n";
    
    std::cout << "=== COMPLETENESS CATEGORIES ===\n";
    std::cout << "Full text (T+A+Body):   " << has_fulltext 
              << " (" << (100.0 * has_fulltext / total_papers) << "%)\n";
    std::cout << "Partial (T+A only):     " << has_partial 
              << " (" << (100.0 * has_partial / total_papers) << "%)\n";
    std::cout << "Minimal (T or A only):  " << has_minimal 
              << " (" << (100.0 * has_minimal / total_papers) << "%)\n";
    std::cout << "Nothing (no T,A,Body):  " << has_nothing 
              << " (" << (100.0 * has_nothing / total_papers) << "%)\n\n";

    
    std::cout << "=== TEXT LENGTH STATISTICS ===\n";
    if (has_title > 0) {
        std::cout << "Title lengths:\n";
        std::cout << "  Average:  " << (total_title_chars / has_title) << " chars\n";
        std::cout << "  Min:      " << min_title_len << " chars\n";
        std::cout << "  Max:      " << max_title_len << " chars\n";
    }
    if (has_abstract > 0) {
        std::cout << "Abstract lengths:\n";
        std::cout << "  Average:  " << (total_abstract_chars / has_abstract) << " chars\n";
        std::cout << "  Min:      " << min_abstract_len << " chars\n";
        std::cout << "  Max:      " << max_abstract_len << " chars\n";
    }
    std::cout << "\n";
    
    std::cout << "=== FIELD COUNT DISTRIBUTION ===\n";
    for (const auto& [field_count, freq] : field_count_freq) {
        std::cout << field_count << " fields: " << freq << " papers";
        if (field_count != HEADER_SIZE) {
            std::cout << " (UNEXPECTED!)";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
    
    std::cout << "=== PROCESSABLE PAPERS ===\n";
    uint32_t processable = has_fulltext + has_partial;
    std::cout << "Papers with usable text: " << processable 
              << " (" << (100.0 * processable / total_papers) << "%)\n";
    std::cout << "  (Title+Abstract+Body or Title+Abstract)\n\n";
    
    std::cout << "=== RECOMMENDATIONS ===\n";
    if (found_neither > total_papers * 0.1) {
        std::cout << " Warning: " << (100.0 * found_neither / total_papers) 
                  << "% of papers have no full text files\n";
    }
    if (has_nothing > 0) {
        std::cout << " Warning: " << has_nothing 
                  << " papers have no useful text at all\n";
    }
    if (processable < total_papers * 0.5) {
        std::cout << " Warning: Less than 50% of papers are processable\n";
    } else {
        std::cout << " Good: " << (100.0 * processable / total_papers) 
                  << "% of papers are processable\n";
    }
    
    
    return 0;
}

