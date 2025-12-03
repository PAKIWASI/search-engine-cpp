#pragma once

#include <string>
#include <unordered_map>

#include "common_includes.hpp"



class Lexicon {
private:
    // word -> (word_id, freq)
    std::unordered_map<std::string, WordData> lexicon;
    u32 next_word_id = 0;

    // reverse mapping word_id ->word (needed for visualization)
    std::unordered_map<u32, std::string> reverse_lex;

    // add or update a word in the lexicon
    // returns the word_id of the word
    u32 add_word(const std::string& word, const u32& freq);

public:
    Lexicon() = default;
    
        // get word_data for a word 
    const WordData* get_word_data(const std::string& word) const;
    
    // get word_id only 
    u32 get_word_id(const std::string& word) const;
    
    // get freq for a word
    u32 get_freq(const std::string& word) const;

    std::string* get_word(const u32& word_id);
    
    // check if word exists
    bool contains(const std::string& word) const;
    
    // get lexicon size
    size_t size() const { return lexicon.size(); }
    
    // get all data (for iteration)
    const std::unordered_map<std::string, WordData>& get_lexicon() const 
    {
        return lexicon;
    }

    const std::unordered_map<u32, std::string>& get_reverse_lexicon() const
    {
        return reverse_lex;
    }
    
    //merge temp lex into main lex
    void merge(const std::unordered_map<std::string, WordData>& temp_lex);
    
    // update temp_lex with actual word IDs from this lexicon
    void update_ids(std::unordered_map<std::string, WordData>& temp_lex) const;
    
    // save lexicon to CSV file
    void save_to_file_csv(const std::string& output_path) const;
    
    // load lexicon from CSV file
    bool load_from_file_csv(const std::string& input_path);

        // save/ load in binary format
    void save_to_file_binary(const std::string& output_path) const;

    bool load_from_file_binary(const std::string& input_path);


    // print top N most frequent words
    void print_top_words(int n) const;
    
    // Clear the lexicon
    void clear();
};


