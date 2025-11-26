#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

#include "word_data.hpp"



class ForwardIndex {
private:
    // TODO: just use cord_uid as the key to remove 2nd map?

    // doc_id -> vector of (word_id, frequency)
    std::unordered_map<uint32_t, std::vector<WordData>> forward_index;
    
    // doc_id -> document metadata (cord_uid, title, etc.)
    std::unordered_map<uint32_t, std::string> doc_metadata;
    
    uint32_t next_doc_id = 0;

public:
    // Add a document to the forward index
    uint32_t add_document(const std::string& cord_uid,
                         const std::unordered_map<std::string, WordData>& temp_lex);
    
    // Get terms for a document
    const std::vector<WordData>* get_document_terms(uint32_t doc_id) const;
    
    // Get doc's cord_uid
    const std::string* get_doc_cord_uid(uint32_t doc_id) const;
    
    // Save forward index to file
    void save_to_file(const std::string& output_path) const;
    
    // Load forward index from file
    bool load_from_file(const std::string& input_path);
    
    // Statistics
    size_t get_document_count() const { return forward_index.size(); }
    size_t get_total_term_count() const;
    
    void print_statistics() const;
};


