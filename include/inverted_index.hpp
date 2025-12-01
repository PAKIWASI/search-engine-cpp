#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "word_data.hpp"


class InvertedIndex {
private:
    // word_id -> vec of doc_ids, freq
    std::unordered_map<uint32_t, std::vector<InvertedEntry>> inverted_index;

    static const uint32_t num_barrel = 4;

    // we will only load the needed barrel into RAM
    struct Barrel {
        uint32_t barrel_id;         // also name of barrel file
        uint32_t start_word_id;
        uint32_t end_word_id;
    };

    std::vector<Barrel> barrels;

public:
    InvertedIndex();
    explicit InvertedIndex(const std::string& path_barrels_metadata);

    // add a new inverted_index entry (useless func)
    void addEntry(const uint32_t& word_id, const InvertedEntry& entry);

    // during metadata parsing we have all the words in a doc in temp_lex
    void add_document(const uint32_t& doc_id, const std::unordered_map<std::string, WordData>& temp_lex);

    // get terms for a word
    const std::vector<InvertedEntry>* get_word_terms(uint32_t word_id) const;

    // save in binary format
    void save_to_file(const std::string& output_path);

    void save_barrels(const std::string& output_folder_path);

    // load from binary format
    bool load_from_file(const std::string& input_path);

    bool load_barrel(const std::string& input_folder_path, const uint32_t& word_id);

    void save_as_text(const std::string& output_path, 
                      const std::unordered_map<uint32_t, std::string>& reverse_lex);

    // stats
    uint32_t get_total_docs(uint32_t word_id);

    uint32_t get_word_count() const { return inverted_index.size(); } 

    void print_statistics() const;
};

