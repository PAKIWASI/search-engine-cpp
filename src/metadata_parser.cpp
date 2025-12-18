
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

// Field name to index mapping
#define CORD_UID    0
#define SHA         1   // for pdfs
#define TITLE       3
#define PMC_ID      5   // for xmls
#define ABSTRACT    8

// Helper function to clean and truncate text
std::string clean_and_truncate(const std::string& text, size_t max_length) 
{
    std::string cleaned = text;
    
    // Remove extra whitespace
    size_t start = cleaned.find_first_not_of(" \t\n\r");
    size_t end = cleaned.find_last_not_of(" \t\n\r");
    
    if (start == std::string::npos) {
        return "";
    }
    
    cleaned = cleaned.substr(start, end - start + 1);
    
    // Replace newlines with spaces
    std::replace(cleaned.begin(), cleaned.end(), '\n', ' ');
    std::replace(cleaned.begin(), cleaned.end(), '\r', ' ');
    std::replace(cleaned.begin(), cleaned.end(), '\t', ' ');
    
    // Remove multiple consecutive spaces
    auto new_end = std::unique(cleaned.begin(), cleaned.end(), 
        [](char a, char b) { return a == ' ' && b == ' '; });
    cleaned.erase(new_end, cleaned.end());
    
    // Truncate if needed
    if (cleaned.length() > max_length) {
        cleaned = cleaned.substr(0, max_length - 3) + "...";
    }
    
    return cleaned;
}

// Main parser
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

    // Main vector for storing each parsed csv line
    std::vector<std::string> parsed_line;

    // Parse the header
    parse_csv_line(line, parsed_line);
    
    // Initialize Lexicon
    Lexicon lexicon;

    // Store data of current doc in temp_lex
    std::unordered_map<std::string, WordData> temp_lex; 
    
    // Initialize TextProcessor
    TextProcessor text_processor(lexicon);
    
    // Initialize Forward Index
    ForwardIndex forward_index;

    // Initialize Inverted Index (with barrel support)
    InvertedIndex inverted_index("indices/");
    
    std::cout << "Starting paper processing...\n";
    
    u32 paper_count = 0;
    u32 lemmatize_count = 0;
    u32 failed_count = 0;
    u32 skipped_count = 0;
    u32 pdf_count = 0;
    u32 xml_count = 0;
    u32 not_found = 0;
    u32 missing_metadata = 0;
    
    // Read all the csv lines
    while (std::getline(file, line)) 
    {
        paper_count++;
        
        // Parse csv line
        parse_csv_line(line, parsed_line);

        // Safety check for field count
        if (parsed_line.size() < HEADER_SIZE) {
            std::cerr << "Warning: Paper " << paper_count 
                      << " has only " << parsed_line.size() 
                      << " fields (expected " << HEADER_SIZE << ")\n";
            skipped_count++;
            continue;
        }

        // Extract metadata FIRST (before processing full text)
        DocumentMetadata metadata;
        
        // Get CORD UID 
        metadata.cord_uid = parsed_line[CORD_UID];
        if (metadata.cord_uid.empty()) {
            std::cerr << "Warning: Paper " << paper_count << " has no CORD UID, skipping\n";
            skipped_count++;
            continue;
        }
        
        // Get title (clean it)
        if (!parsed_line[TITLE].empty()) {
            metadata.title = clean_and_truncate(parsed_line[TITLE], 500);
        }
        
        // Get abstract (clean it)
        if (!parsed_line[ABSTRACT].empty()) {
            metadata.abstract = clean_and_truncate(parsed_line[ABSTRACT], 1000);
        }
        
        // Get pmcid for url 
        if (parsed_line.size() > PMC_ID) {
            metadata.pmcid = parsed_line[PMC_ID];
        }

        
        // Check if we have at least some metadata
        if (metadata.title.empty() && metadata.abstract.empty()) {
            missing_metadata++;
            std::cout << "Paper No: " << paper_count 
                      << " (CORD: " << metadata.cord_uid << ") has no title or abstract\n";
        }

        // Build text for lemmatization
        std::string full_text;

        // Add title
        if (!metadata.title.empty()) {
            full_text += metadata.title;
            full_text += ' ';
        }

        // Add abstract
        if (!metadata.abstract.empty()) {
            full_text += metadata.abstract;
            full_text += ' ';
        }
        
        // Try to find pdfs first
        std::string path_pdf = find_fulltext_pdf(parsed_line[SHA]);

        // If no pdf, try xml
        if (path_pdf.empty() && parsed_line.size() > PMC_ID) {
            std::string path_xml = find_fulltext_xml(parsed_line[PMC_ID]);
            if (!path_xml.empty()) {
                extract_body_text(path_xml, full_text);
                xml_count++;
            }
            else {
                not_found++;
            }
        } 
        else if (!path_pdf.empty()) {
            extract_body_text(path_pdf, full_text);
            pdf_count++;
        }

        if (!full_text.empty()) {
            // Lemmatize text using LibStemmer 
            bool success_lemma = text_processor.lemmatize_libstemmer(full_text, temp_lex);

            if (success_lemma && !temp_lex.empty()) {
                lemmatize_count++;

                // Add document to forward index with COMPLETE metadata
                u32 doc_id = forward_index.add_document(metadata, temp_lex);

                // Build inverted index
                inverted_index.add_document(doc_id, temp_lex);
            } 
            else {
                failed_count++;
                std::cout << "Paper No: " << paper_count 
                          << " (CORD: " << metadata.cord_uid << ") - Lemmatization Failed\n";
            }
        } 
        else {
            skipped_count++;
            std::cout << "Paper No: " << paper_count 
                      << " (CORD: " << metadata.cord_uid << ") - Full text empty\n";
        }

        // Progress indicator
        if (paper_count % 100 == 0) {
            std::cout << "\n=== Progress Report ===\n";
            std::cout << "Papers processed: " << paper_count << "\n";
            std::cout << "Successfully indexed: " << lemmatize_count << "\n";
            std::cout << "Unique terms: " << text_processor.get_lexicon_size() << "\n";
            std::cout << "With PDFs: " << pdf_count << "\n";
            std::cout << "With XMLs: " << xml_count << "\n";
            std::cout << "Missing full text: " << not_found << "\n";
            std::cout << "Missing metadata: " << missing_metadata << "\n\n";
        }
        
        // Limit for testing
        if (paper_count >= 1000) { 
            break; 
        }
    }

    std::cout << "\n";
    std::cout << "             PROCESSING SUMMARY                                \n";
    std::cout << "Total papers read:          " << paper_count << "\n";
    std::cout << "Successfully indexed:       " << lemmatize_count << "\n";
    std::cout << "Failed to process:          " << failed_count << "\n";
    std::cout << "Skipped (no text):          " << skipped_count << "\n";
    std::cout << "Missing title/abstract:     " << missing_metadata << "\n";
    std::cout << "PDF's found:                " << pdf_count << "\n";
    std::cout << "XML's found:                " << xml_count << "\n";
    std::cout << "Not found (no PDF/XML):     " << not_found << "\n";
    std::cout << "Unique terms in lexicon:    " << lexicon.size() << "\n";
    std::cout << "\n";

    // Print top terms
    lexicon.print_top_words(50);
    
    // Print forward index stats
    forward_index.print_statistics();

    // Print inverted_index stats
    inverted_index.print_statistics();
    
    std::cout << "\n";
    std::cout << "             SAVING INDICES                                   \n";

    // Save lexicon to file (binary)
    lexicon.save_to_file_binary("indices/lexicon_cordR1.bin");
    
    // Save forward index to file (binary form)
    forward_index.save_to_file("indices/forward_index_cordR1.bin");

    // Save inverted_index with barrels
    inverted_index.save_barrels();

    std::cout << "\n✓ All indices saved successfully!\n";
    std::cout << "\nYou can now run the search engine with these indices.\n";

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
    u32 total_papers = 0;
    u32 has_title = 0;
    u32 has_abstract = 0;
    u32 has_sha = 0;
    u32 has_pmcid = 0;
    
    u32 found_pdf = 0;
    u32 found_xml = 0;
    u32 found_both = 0;
    u32 found_neither = 0;
    
    u32 has_fulltext = 0;  // title + abstract + (pdf or xml)
    u32 has_partial = 0;   // title + abstract only
    u32 has_minimal = 0;   // title or abstract only
    u32 has_nothing = 0;   // no title, no abstract, no full text
    
    // Text length statistics
    uint64_t total_title_chars = 0;
    uint64_t total_abstract_chars = 0;
    size_t min_title_len = SIZE_MAX;
    size_t max_title_len = 0;
    size_t min_abstract_len = SIZE_MAX;
    size_t max_abstract_len = 0;
    
    // Field size frequency (for debugging)
    std::map<size_t, u32> field_count_freq;
    
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
    u32 processable = has_fulltext + has_partial;
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



// Add these static functions to metadata_parser.cpp

std::string MetadataParser::find_fulltext_pdf_static(std::string& sha, const std::string& data_path)
{
    if (sha.empty()) { return ""; }
    
    std::string first_sha = sha;
    size_t semi_pos = sha.find(';');
    if (semi_pos != std::string::npos) {
        first_sha = sha.substr(0, semi_pos);
        first_sha.erase(0, first_sha.find_first_not_of(" \t"));
        first_sha.erase(first_sha.find_last_not_of(" \t") + 1);
    }
    
    std::vector<std::string> pdf_search_paths = {
        data_path + "/comm_use_subset/pdf_json/" + first_sha + ".json",
        data_path + "/noncomm_use_subset/pdf_json/" + first_sha + ".json",
        data_path + "/custom_license/pdf_json/" + first_sha + ".json",
        data_path + "/biorxiv_medrxiv/pdf_json/" + first_sha + ".json"
    };
    
    for (const auto& path : pdf_search_paths) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
    
    return "";
}

std::string MetadataParser::find_fulltext_xml_static(std::string& pmcid, const std::string& data_path)
{
    if (pmcid.empty()) { return ""; }
    
    std::vector<std::string> xml_search_paths = {
        data_path + "/comm_use_subset/pmc_json/" + pmcid + ".xml.json",
        data_path + "/noncomm_use_subset/pmc_json/" + pmcid + ".xml.json",
        data_path + "/custom_license/pmc_json/" + pmcid + ".xml.json",
    };
    
    for (const auto& path : xml_search_paths) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
    
    return "";
}

void MetadataParser::extract_body_text_static(const std::string& file_path, std::string& body_text) 
{
    if (file_path.empty()) { return; }
    try {
        std::ifstream file(file_path);
        nlohmann::json data = nlohmann::json::parse(file);
        
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


