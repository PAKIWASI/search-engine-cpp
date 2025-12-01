#include "forward_index.hpp"


#include <fstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>



uint32_t ForwardIndex::add_document(const std::string& cord_uid,
                const std::unordered_map<std::string, WordData>& temp_lex) 
{
    uint32_t doc_id = next_doc_id++;
    
    doc_metadata[doc_id] = cord_uid;  // store cord_uid
    
    // build term freq vector for this document
    std::vector<WordData> terms;
    terms.reserve(temp_lex.size());
    
    for (const auto& [word, word_info] : temp_lex) 
    {
        terms.push_back({word_info.word_id, word_info.freq});
    }
    
    forward_index[doc_id] = std::move(terms); // dont copy 
    
    return doc_id;
}

const std::vector<WordData>* ForwardIndex::get_document_terms(uint32_t doc_id) const 
{
    auto it = forward_index.find(doc_id);
    if (it != forward_index.end()) {
        return &it->second;     // return vec of WordData
    }
    return nullptr;
}

const std::string* ForwardIndex::get_doc_cord_uid(uint32_t doc_id) const 
{
    auto it = doc_metadata.find(doc_id);
    if (it != doc_metadata.end()) {
        return &it->second;         // return cord_uid (str)
    }
    return nullptr;
}

// save in binary format for fast lookups
void ForwardIndex::save_to_file(const std::string& output_path) const 
{
                    // we store as binary for fast save/load
    std::ofstream file(output_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open forward index file for writing\n";
        return;
    }
    
            // write number of documents
    uint32_t doc_count = static_cast<uint32_t>(forward_index.size());
    file.write(reinterpret_cast<const char*>(&doc_count), sizeof(doc_count));
    

            // write each document
    for (const auto& [doc_id, terms] : forward_index) 
    {
            // write doc_id
        file.write(reinterpret_cast<const char*>(&doc_id), sizeof(doc_id));
        
            // write metadata length and metadata
        const std::string& metadata = doc_metadata.at(doc_id); // coord_uid
        uint32_t metadata_len = static_cast<uint32_t>(metadata.size());
        file.write(reinterpret_cast<const char*>(&metadata_len), sizeof(metadata_len));
        file.write(metadata.c_str(), metadata_len);
        
            // write number of terms
        uint32_t term_count = static_cast<uint32_t>(terms.size());
        file.write(reinterpret_cast<const char*>(&term_count), sizeof(term_count));
        
            // write all terms
        file.write(reinterpret_cast<const char*>(terms.data()), 
                   term_count * sizeof(WordData));
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
    
    forward_index.clear();
    doc_metadata.clear();
            // we read in the order that we wrote in binary format
    
    // read number of documents
    uint32_t doc_count;
    file.read(reinterpret_cast<char*>(&doc_count), sizeof(doc_count));
    
    // read each document
    for (uint32_t i = 0; i < doc_count; ++i) {
            // read doc_id
        uint32_t doc_id;
        file.read(reinterpret_cast<char*>(&doc_id), sizeof(doc_id));

            // read metadata
        uint32_t metadata_len;
        file.read(reinterpret_cast<char*>(&metadata_len), sizeof(metadata_len));
        std::string metadata(metadata_len, '\0');
        file.read(metadata.data(), metadata_len);
        doc_metadata[doc_id] = metadata;
        
            // read number of terms
        uint32_t term_count;
        file.read(reinterpret_cast<char*>(&term_count), sizeof(term_count));
        
            // read all terms
        std::vector<WordData> terms(term_count);
        file.read(reinterpret_cast<char*>(terms.data()), 
                 term_count * sizeof(WordData));
        
        // add to forward index
        forward_index[doc_id] = std::move(terms);
        
        if (doc_id >= next_doc_id) {
            next_doc_id = doc_id + 1;
        }
    }
    
    file.close();
    std::cout << "Forward index loaded from " << input_path << '\n';
    return true;
}


void ForwardIndex::save_as_text(const std::string& output_path, 
                                const std::unordered_map<uint32_t, std::string>& reverse_lex)
{
    std::ofstream file(output_path);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open forward index file for writing\n";
        return;
    }    

    file << "doc_id: cord_uid -> [ (word_id, word, freq) ]\n";

    for (const auto& [doc_id, word_data] : forward_index) {
        
        file << doc_id << ": " << doc_metadata[doc_id] << " -> [ ";
        
        for (const auto& [word_id, freq] : word_data) {
            file << "( " << word_id << ", " << reverse_lex.at(word_id) << ", " << freq << " ), ";
        }

        file << '\n';
    }

    std::cout << "Forward index saved to " << output_path << '\n';
    file.close();
}

uint32_t ForwardIndex::get_total_words(uint32_t doc_id) const 
{
    auto it = forward_index.find(doc_id);
    if (it != forward_index.end()) {
        return it->second.size();
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



