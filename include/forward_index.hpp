#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

#include "word_data.hpp"



class ForwardIndex {
private:
    // doc_id -> vector of (word_id, freq)
    std::unordered_map<uint32_t, std::vector<WordData>> forward_index;
    
    // doc_id -> document metadata (cord_uid)
    std::unordered_map<uint32_t, std::string> doc_metadata;
    
    uint32_t next_doc_id = 0;

public:
    ForwardIndex() = default;

    // add a document to the forward index (params got from parser)
    uint32_t add_document(const std::string& cord_uid,
                         const std::unordered_map<std::string, WordData>& temp_lex);
    
    // get terms for a document
    const std::vector<WordData>* get_document_terms(uint32_t doc_id) const;
    
    // get doc's cord_uid only
    const std::string* get_doc_cord_uid(uint32_t doc_id) const;
    
    // save forward index to file (binary format)
    void save_to_file(const std::string& output_path) const;

    // Load forward index from file
    bool load_from_file(const std::string& input_path);

    // for dubugging, viewing
    void save_as_text(const std::string& output_path, 
                      const std::unordered_map<uint32_t, std::string>& reverse_lex);
    
    // stats
    uint32_t get_total_words(uint32_t doc_id) const;

    uint32_t get_document_count() const { return forward_index.size(); }
    
    void print_statistics() const;
};


