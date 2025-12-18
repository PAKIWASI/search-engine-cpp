#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "common_includes.hpp"

class ForwardIndex {
private:
    std::unordered_map<u32, std::vector<WordData>> forward_index;
    std::unordered_map<u32, DocumentMetadata> doc_metadata; 
    u32 next_doc_id = 0;

public:
    ForwardIndex() = default;

    // UPDATED: Now takes full metadata
    u32 add_document(const DocumentMetadata& metadata,
                     const std::unordered_map<std::string, WordData>& temp_lex);
    
    const std::vector<WordData>* get_document_terms(u32 doc_id) const;
    
    // NEW: Get complete document metadata
    const DocumentMetadata* get_document_metadata(u32 doc_id) const;
    
    // DEPRECATED: Use get_document_metadata instead
    const std::string* get_doc_cord_uid(u32 doc_id) const;
    
    void save_to_file(const std::string& output_path) const;
    bool load_from_file(const std::string& input_path);
    bool merge_from_file(const std::string& input_path, u32 first_doc_id);
    
    void save_as_text(const std::string& output_path, 
                      const std::unordered_map<u32, std::string>& reverse_lex);
    
    u32 get_total_words(u32 doc_id) const;
    u32 get_document_count() const { return forward_index.size(); }
    void print_statistics() const;
    u32 size() const { return forward_index.size(); }
};


