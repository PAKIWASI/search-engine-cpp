#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <utility>


struct TermFrequency {
    uint32_t word_id;
    uint32_t frequency;
};


class ForwardIndex {
private:
    // TODO: just use cord_uid as the key to remove 2nd map?

    // doc_id -> vector of (word_id, frequency)
    std::unordered_map<uint32_t, std::vector<TermFrequency>> index;
    
    // doc_id -> document metadata (cord_uid, title, etc.)
    std::unordered_map<uint32_t, std::string> doc_metadata;
    
    uint32_t next_doc_id = 0;

public:
    // Add a document to the forward index
    uint32_t add_document(const std::string& cord_uid,
                         const std::unordered_map<std::string, std::pair<uint32_t, uint32_t>>& temp_lex);
    
    // Get terms for a document
    const std::vector<TermFrequency>* get_document_terms(uint32_t doc_id) const;
    
    // Get document metadata
    const std::string* get_document_metadata(uint32_t doc_id) const;
    
    // Save forward index to file
    void save_to_file(const std::string& output_path) const;
    
    // Load forward index from file
    bool load_from_file(const std::string& input_path);
    
    // Statistics
    size_t get_document_count() const { return index.size(); }
    size_t get_total_term_count() const;
    
    void print_statistics() const;
};


