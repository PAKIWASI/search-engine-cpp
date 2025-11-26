#include "forward_index.hpp"

#include <fstream>
#include <iostream>
#include <algorithm>



uint32_t ForwardIndex::add_document(const std::string& cord_uid,
                const std::unordered_map<std::string, std::pair<uint32_t, uint32_t>>& temp_lex) 
{
    uint32_t doc_id = next_doc_id++;
    
    // Store metadata
    doc_metadata[doc_id] = cord_uid;
    
    // Build term frequency vector for this document
    std::vector<TermFrequency> terms;
    terms.reserve(temp_lex.size());
    
    for (const auto& [word, pair] : temp_lex) {
        uint32_t word_id = pair.first;
        uint32_t frequency = pair.second;
        terms.push_back({word_id, frequency});
    }
    
    // Sort by word_id for better cache locality during lookups
    std::sort(terms.begin(), terms.end(), 
              [](const TermFrequency& a, const TermFrequency& b) {
                  return a.word_id < b.word_id;
              });
    
    index[doc_id] = std::move(terms);
    
    return doc_id;
}

const std::vector<TermFrequency>* ForwardIndex::get_document_terms(uint32_t doc_id) const 
{
    auto it = index.find(doc_id);
    if (it != index.end()) {
        return &it->second;
    }
    return nullptr;
}

const std::string* ForwardIndex::get_document_metadata(uint32_t doc_id) const 
{
    auto it = doc_metadata.find(doc_id);
    if (it != doc_metadata.end()) {
        return &it->second;
    }
    return nullptr;
}

// save in binary format for fast lookups
void ForwardIndex::save_to_file(const std::string& output_path) const 
{
    std::ofstream file(output_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open forward index file for writing\n";
        return;
    }
    
    // Write number of documents
    uint32_t doc_count = static_cast<uint32_t>(index.size());
    file.write(reinterpret_cast<const char*>(&doc_count), sizeof(doc_count));
    
    // Write each document
    for (const auto& [doc_id, terms] : index) {
        // Write doc_id
        file.write(reinterpret_cast<const char*>(&doc_id), sizeof(doc_id));
        
        // Write metadata length and metadata
        const std::string& metadata = doc_metadata.at(doc_id);
        uint32_t metadata_len = static_cast<uint32_t>(metadata.size());
        file.write(reinterpret_cast<const char*>(&metadata_len), sizeof(metadata_len));
        file.write(metadata.c_str(), metadata_len);
        
        // Write number of terms
        uint32_t term_count = static_cast<uint32_t>(terms.size());
        file.write(reinterpret_cast<const char*>(&term_count), sizeof(term_count));
        
        // Write all terms
        file.write(reinterpret_cast<const char*>(terms.data()), 
                  term_count * sizeof(TermFrequency));
    }
    
    file.close();
    std::cout << "Forward index saved to " << output_path << '\n';
}

bool ForwardIndex::load_from_file(const std::string& input_path) 
{
    std::ifstream file(input_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open forward index file for reading\n";
        return false;
    }
    
    index.clear();
    doc_metadata.clear();
    
    // Read number of documents
    uint32_t doc_count;
    file.read(reinterpret_cast<char*>(&doc_count), sizeof(doc_count));
    
    // Read each document
    for (uint32_t i = 0; i < doc_count; ++i) {
        // Read doc_id
        uint32_t doc_id;
        file.read(reinterpret_cast<char*>(&doc_id), sizeof(doc_id));
        
        // Read metadata
        uint32_t metadata_len;
        file.read(reinterpret_cast<char*>(&metadata_len), sizeof(metadata_len));
        std::string metadata(metadata_len, '\0');
        file.read(&metadata[0], metadata_len);
        doc_metadata[doc_id] = metadata;
        
        // Read number of terms
        uint32_t term_count;
        file.read(reinterpret_cast<char*>(&term_count), sizeof(term_count));
        
        // Read all terms
        std::vector<TermFrequency> terms(term_count);
        file.read(reinterpret_cast<char*>(terms.data()), 
                 term_count * sizeof(TermFrequency));
        
        index[doc_id] = std::move(terms);
        
        if (doc_id >= next_doc_id) {
            next_doc_id = doc_id + 1;
        }
    }
    
    file.close();
    std::cout << "Forward index loaded from " << input_path << '\n';
    return true;
}

size_t ForwardIndex::get_total_term_count() const 
{
    size_t total = 0;
    for (const auto& [doc_id, terms] : index) {
        total += terms.size();
    }
    return total;
}

void ForwardIndex::print_statistics() const 
{
    std::cout << "\nForward Index Statistics:\n";
    std::cout << "  Total documents: " << index.size() << '\n';
    std::cout << "  Total unique terms across all docs: " << get_total_term_count() << '\n';
    
    if (!index.empty()) {
        size_t min_terms = SIZE_MAX;
        size_t max_terms = 0;
        size_t total_terms = 0;
        
        for (const auto& [doc_id, terms] : index) {
            size_t count = terms.size();
            min_terms = std::min(min_terms, count);
            max_terms = std::max(max_terms, count);
            total_terms += count;
        }
        
        double avg_terms = static_cast<double>(total_terms) / index.size();
        std::cout << "  Avg terms per document: " << avg_terms << '\n';
        std::cout << "  Min terms in a document: " << min_terms << '\n';
        std::cout << "  Max terms in a document: " << max_terms << '\n';
    }
}



