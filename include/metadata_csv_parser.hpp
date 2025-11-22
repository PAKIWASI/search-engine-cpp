#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <string>


/* Fields we Need
1 -> cord_uid
2 -> sha
4 -> title
9 -> abstract
11 -> authors
15 -> has_pdf_parse
16 -> has_pmc_xml_parse
17 -> full_text_file

TOTAL SIZE = 18 (we'll consider only these)
 */

struct Paper {
    uint16_t id; // our own id
    std::string cord_uid;
    std::string sha;
    std::string title;
    std::string abstract;
    std::vector<std::string> authors;
    std::string* full_text;
};

/*
    // Get combined text for indexing
    std::string get_indexable_text() const {
        return title + " " + abstract + " " + full_text;
    }
    
    bool has_text() const {
        return !title.empty() || !abstract.empty() || !full_text.empty();
    }
*/

// Parse CSV line handling quotes and commas
void parseCSVLine(const std::string& line, std::vector<std::string>& parsed_line);


int metadata_parse_stats();


class PaperLoader {
private:
    std::string dataset_path;
    std::unordered_map<std::string, int> column_index;
    
    // Find JSON file by SHA
    std::string find_json_by_sha(const std::string& sha);
    
    // Extract text from JSON file
    std::string extract_text_from_json(const std::string& json_path);
    
    // Get field from parsed CSV line
    std::string get_field(const std::vector<std::string>& fields, const std::string& col_name);
    
public:
    explicit PaperLoader(const std::string& path);
    
    // Load papers from metadata.csv
    std::vector<Paper> load_papers(int max_papers = -1);
    
    // Print statistics
    void print_stats(const std::vector<Paper>& papers);
};
