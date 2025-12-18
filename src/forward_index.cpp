
#include "forward_index.hpp"
#include <fstream>
#include <iostream>


u32 ForwardIndex::add_document(const DocumentMetadata& metadata,
                const std::unordered_map<std::string, WordData>& temp_lex) 
{
    u32 doc_id = next_doc_id++;
    
    doc_metadata[doc_id] = metadata;  // Store complete metadata
    
    std::vector<WordData> terms;
    terms.reserve(temp_lex.size());
    
    for (const auto& [word, word_info] : temp_lex) {
        terms.push_back({word_info.word_id, word_info.freq});
    }
    
    forward_index[doc_id] = std::move(terms);
    return doc_id;
}

const DocumentMetadata* ForwardIndex::get_document_metadata(u32 doc_id) const {
    auto it = doc_metadata.find(doc_id);
    if (it != doc_metadata.end()) {
        return &it->second;
    }
    return nullptr;
}

const std::string* ForwardIndex::get_doc_cord_uid(u32 doc_id) const {
    auto it = doc_metadata.find(doc_id);
    if (it != doc_metadata.end()) {
        return &it->second.cord_uid;
    }
    return nullptr;
}

void ForwardIndex::save_to_file(const std::string& output_path) const {
    std::ofstream file(output_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open forward index file for writing\n";
        return;
    }
    
    u32 doc_count = static_cast<u32>(forward_index.size());
    file.write(reinterpret_cast<const char*>(&doc_count), sizeof(doc_count));
    
    for (const auto& [doc_id, terms] : forward_index) {
        // Write doc_id
        file.write(reinterpret_cast<const char*>(&doc_id), sizeof(doc_id));
        
        // Write metadata
        const DocumentMetadata& meta = doc_metadata.at(doc_id);
        
        // Write cord_uid
        u32 len = static_cast<u32>(meta.cord_uid.size());
        file.write(reinterpret_cast<const char*>(&len), sizeof(len));
        file.write(meta.cord_uid.c_str(), len);
        
        // Write title
        len = static_cast<u32>(meta.title.size());
        file.write(reinterpret_cast<const char*>(&len), sizeof(len));
        file.write(meta.title.c_str(), len);
        
        // Write abstract
        len = static_cast<u32>(meta.abstract.size());
        file.write(reinterpret_cast<const char*>(&len), sizeof(len));
        file.write(meta.abstract.c_str(), len);
        
        // Write pmcid
        len = static_cast<u32>(meta.pmcid.size());
        file.write(reinterpret_cast<const char*>(&len), sizeof(len));
        file.write(meta.pmcid.c_str(), len);
        
        // Write terms
        u32 term_count = static_cast<u32>(terms.size());
        file.write(reinterpret_cast<const char*>(&term_count), sizeof(term_count));
        file.write(reinterpret_cast<const char*>(terms.data()), 
                   term_count * sizeof(WordData));
    }
    
    file.close();
    std::cout << "Forward index saved to " << output_path << '\n';
}

bool ForwardIndex::load_from_file(const std::string& input_path) {
    std::ifstream file(input_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open forward index file for reading\n";
        return false;
    }
    
    forward_index.clear();
    doc_metadata.clear();
    
    u32 doc_count;
    file.read(reinterpret_cast<char*>(&doc_count), sizeof(doc_count));
    
    for (u32 i = 0; i < doc_count; ++i) {
        u32 doc_id;
        file.read(reinterpret_cast<char*>(&doc_id), sizeof(doc_id));
        
        DocumentMetadata meta;
        
        // Read cord_uid
        u32 len;
        file.read(reinterpret_cast<char*>(&len), sizeof(len));
        meta.cord_uid.resize(len);
        file.read(&meta.cord_uid[0], len);
        
        // Read title
        file.read(reinterpret_cast<char*>(&len), sizeof(len));
        meta.title.resize(len);
        file.read(&meta.title[0], len);
        
        // Read abstract
        file.read(reinterpret_cast<char*>(&len), sizeof(len));
        meta.abstract.resize(len);
        file.read(&meta.abstract[0], len);
        
        // Read URL
        file.read(reinterpret_cast<char*>(&len), sizeof(len));
        meta.pmcid.resize(len);
        file.read(&meta.pmcid[0], len);
        
        doc_metadata[doc_id] = meta;
        
        // Read terms
        u32 term_count;
        file.read(reinterpret_cast<char*>(&term_count), sizeof(term_count));
        std::vector<WordData> terms(term_count);
        file.read(reinterpret_cast<char*>(terms.data()), 
                 term_count * sizeof(WordData));
        
        forward_index[doc_id] = std::move(terms);
        
        if (doc_id >= next_doc_id) {
            next_doc_id = doc_id + 1;
        }
    }
    
    file.close();
    std::cout << "Forward index loaded from " << input_path << '\n';
    return true;
}


u32 ForwardIndex::get_total_words(u32 doc_id) const 
{
    auto it = forward_index.find(doc_id);
    if (it != forward_index.end()) {
        // Sum up all frequencies to get total word count
        u32 total = 0;
        for (const auto& word_data : it->second) {
            total += word_data.freq;
        }
        return total;
    }
    else {
        return 0;
    }
}

void ForwardIndex::print_statistics() const 
{
    std::cout << "\nForward Index Statistics:\n";
    std::cout << "  Total documents: " << forward_index.size() << '\n';
    
    if (!forward_index.empty()) {
        size_t min_terms = SIZE_MAX;
        size_t max_terms = 0;
        size_t total_terms = 0;
        
        for (const auto& [doc_id, terms] : forward_index) {
            size_t count = terms.size();
            min_terms = std::min(min_terms, count);
            max_terms = std::max(max_terms, count);
            total_terms += count;
        }
        
        double avg_terms = static_cast<double>(total_terms) / forward_index.size();
        std::cout << "  Avg terms per document: " << avg_terms << '\n';
        std::cout << "  Min terms in a document: " << min_terms << '\n';
        std::cout << "  Max terms in a document: " << max_terms << '\n';
    }
}


const std::vector<WordData>* ForwardIndex::get_document_terms(u32 doc_id) const 
{
    auto it = forward_index.find(doc_id);
    if (it != forward_index.end()) {
        return &it->second;     // return vec of WordData
    }
    return nullptr;
}

