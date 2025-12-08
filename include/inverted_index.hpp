#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "common_includes.hpp"


class InvertedIndex {
private:
    // word_id -> vec of doc_ids, freq
    std::unordered_map<u32, std::vector<InvertedEntry>> inverted_index;


    // BARRELS
    
    // we will only load the needed barrel into RAM
    struct Barrel {
        u32 start_word_id;
        u32 end_word_id;
    };

    std::string barrel_path;
    static const u32 num_barrel = 4;    // number of barrels index will be split into
    std::vector<Barrel> barrels;        // index is barrel id, also name of file
    u32 curr_barrel = UINT32_MAX;       // curr barrel loaded in ram

public:
    InvertedIndex();
    explicit InvertedIndex(const std::string& barrel_folder_path);

    // add a new inverted_index entry (useless func)
    void addEntry(u32 word_id, const InvertedEntry& entry);

    // during metadata parsing we have all the words in a doc in temp_lex
    void add_document(u32 doc_id, const std::unordered_map<std::string, WordData>& temp_lex);

    // get terms for a word
    const std::vector<InvertedEntry>* get_word_terms(u32 word_id) const;

    // save in binary format
    void save_to_file(const std::string& output_path);

    void save_barrels();

    // load from binary format
    bool load_from_file(const std::string& input_path);

    bool load_barrel(u32 word_id);

    void save_as_text(const std::string& output_path, 
                      const std::unordered_map<u32, std::string>& reverse_lex);

    // stats
    u32 get_total_docs(u32 word_id);

    u32 get_word_count() const { return inverted_index.size(); } 

    void print_statistics() const;

    void clear() { inverted_index.clear(); }
};

