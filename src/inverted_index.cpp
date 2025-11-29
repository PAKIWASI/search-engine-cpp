#include "inverted_index.hpp"
#include "word_data.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>


void InvertedIndex::addEntry(const uint32_t& word_id, const InvertedEntry& entry)
{
    auto it = inverted_index.find(word_id);
    if (it != inverted_index.end()) {       // already exist
        it->second.push_back( entry ); 
    }
    else {                  // new entry
        inverted_index[word_id] = { entry };
    }
}

void InvertedIndex::add_document(const uint32_t& doc_id, const std::unordered_map<std::string, WordData>& temp_lex)
{
    for (const auto& [word, word_info] : temp_lex) {
        inverted_index[word_info.word_id].push_back( { doc_id, word_info.freq } );
    }
}

const std::vector<InvertedEntry>* InvertedIndex::get_word_terms(uint32_t word_id) const
{
    auto it = inverted_index.find(word_id);
    if (it != inverted_index.end()) { // found
        return &it->second;    
    }

    return nullptr;
}

void InvertedIndex::save_to_file(const std::string& output_path) const
{
    // we store as binary for fast save/load
    std::ofstream file(output_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open inverted index file for writing\n";
        return;
    }

    // write no of words
    uint32_t word_count = static_cast<uint32_t>(inverted_index.size());
    file.write(reinterpret_cast<const char*>(&word_count), sizeof(word_count));


    // write each word entry
    for (const auto& [word_id, terms] : inverted_index) {
        
        // write word_id
        file.write(reinterpret_cast<const char*>(&word_id), sizeof(word_id));

        // write no of terms
        uint32_t term_count = static_cast<uint32_t>(terms.size());
        file.write(reinterpret_cast<const char*>(&term_count), sizeof(term_count));

        // write all terms
        file.write(reinterpret_cast<const char*>(terms.data()),
                   term_count * sizeof(InvertedEntry));
    }

    file.close();
    std::cout << "Inverted index saved to " << output_path << '\n';
}


bool InvertedIndex::load_from_file(const std::string& input_path)
{
    std::ifstream file(input_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open Inverted index file for reading\n";
        return false;
    }

    inverted_index.clear();

    // read no of words
    uint32_t word_count;
    file.read( reinterpret_cast<char*>(&word_count), sizeof(word_count));

    // read each entry
    for (uint32_t i = 0; i < word_count; i++) {

        // read word_id
        uint32_t word_id;
        file.read(reinterpret_cast<char*>(&word_id), sizeof(word_id));

        // read no of terms
        uint32_t term_count;
        file.read(reinterpret_cast<char*>(&term_count), sizeof(term_count));

        // read all the terms
        std::vector<InvertedEntry> terms(term_count);
        file.read(reinterpret_cast<char*>(terms.data()),
                  term_count * sizeof(InvertedEntry));

        // add to inverted index (move all the terms)
        inverted_index[word_id] = std::move(terms);
    }

    file.close();
    std::cout << "Inverted index loaded from: " << input_path << '\n'; 
    std::cout << "Totoal Words: " << inverted_index.size() << '\n';
    return true;
}


void InvertedIndex::save_as_text(const std::string& output_path, 
                                 const std::unordered_map<uint32_t, std::string> reverse_lex)
{
    std::ofstream file(output_path);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open inverted index file for writing\n";
        return;
    }

    file << "word_id: word -> [ (doc_id, freq) ]\n";

    for (const auto& [word_id, terms] : inverted_index) {
        
        file << word_id << ": " << reverse_lex.at(word_id) << " -> [ ";

        for (const auto& [doc_id, freq] : terms) {
            file << "( " << doc_id << ", " << freq << " ), ";
        }

        file << '\n';
    }
     
    file.close();
}



uint32_t InvertedIndex::get_total_docs(uint32_t word_id)
{
    auto it = inverted_index.find(word_id);
    if (it != inverted_index.end()) {       // already exist
        return static_cast<uint32_t>(it->second.size());
    }

    return 0;
}


void InvertedIndex::print_statistics() const
{
    std::cout << "\nInverted Index Statistics:\n";    
    std::cout << "  Total unique words: " << inverted_index.size() << '\n';

    if (!inverted_index.empty()) {
        size_t total_docs = 0;
        size_t min_docs = SIZE_MAX;
        size_t max_docs = 0;
        
        for (const auto& [word_id, docs_list] : inverted_index) {
            size_t count = docs_list.size();
            total_docs += count;
            min_docs = std::min(min_docs, count);
            max_docs = std::max(max_docs, count);
        }
        
        double avg_postings = static_cast<double>(total_docs) / inverted_index.size();
        
        std::cout << "  Total Docs (Repeated):          " << total_docs << '\n';
        std::cout << "  Avg doc per word:   " << avg_postings << '\n';
        std::cout << "  Min docs for a word: " << min_docs << '\n';
        std::cout << "  Max docs for a word: " << max_docs << '\n';
    }
}


