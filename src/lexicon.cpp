#include "lexicon.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>



u32 Lexicon::add_word(const std::string& word, const u32& freq) 
{
    auto it = lexicon.find(word);
    if (it != lexicon.end()) {
        it->second.freq += freq;   // word exists, update freq
        return it->second.word_id;
    }
    else {

        u32 word_id = next_word_id++;  // new word, assign new id
        
        
        lexicon[word] = { word_id, freq };

        reverse_lex[word_id] = word; // add reverse mapping
        

        return word_id;
    }
}

const WordData* Lexicon::get_word_data(const std::string& word) const 
{
    auto it = lexicon.find(word);
    if (it != lexicon.end()) {
        return &it->second;
    }
    return nullptr;
}

u32 Lexicon::get_word_id(const std::string& word) const 
{
    auto it = lexicon.find(word);
    if (it != lexicon.end()) {
        return it->second.word_id;
    }
    return UINT32_MAX;      // return max as not found
}


std::string* Lexicon::get_word(const u32& word_id)
{
    auto it = reverse_lex.find(word_id);
    if (it != reverse_lex.end()) {
        return &it->second;
    }
    else {
        return nullptr;
    }
}

u32 Lexicon::get_freq(const std::string& word) const 
{
    auto it = lexicon.find(word);
    if (it != lexicon.end()) {
        return it->second.freq;
    }
    return 0;
}

bool Lexicon::contains(const std::string& word) const 
{
    return lexicon.contains(word);
}

void Lexicon::merge(const std::unordered_map<std::string, WordData>& temp_lex) 
{
    for (const auto& [word, pair] : temp_lex) 
    {
        add_word(word, pair.freq); // ignoring return value
    }
}

void Lexicon::update_ids(std::unordered_map<std::string, WordData>& temp_lex) const 
{
    for (auto& [word, pair] : temp_lex) {
        auto it = lexicon.find(word);
        if (it != lexicon.end()) {
            pair.word_id = it->second.word_id;  // update with actual ID
        }
    }
}

void Lexicon::save_to_file_csv(const std::string& output_path) const 
{
    std::ofstream file(output_path);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open output file " << output_path << '\n';
        return;
    }
    
    // convert to vector for sorting (can't sort the hashmap(wtf?))
    std::vector<std::pair<std::string, WordData>> sorted_data(lexicon.begin(), lexicon.end());
    
    // Sort by freq
    std::sort(sorted_data.begin(), sorted_data.end(),
              [](const auto& a, const auto& b) { 
                  return a.second.freq > b.second.freq; 
              });
    
    // write to file
    file << "word,wordid,freq\n";
    for (const auto& [word, pair] : sorted_data) {
        file << word << "," << pair.word_id << "," << pair.freq << '\n';
    }
    
    file.close();
    std::cout << "\nLexicon saved to " << output_path << '\n';
    std::cout << "  Total unique terms: " << lexicon.size() << '\n';
}

bool Lexicon::load_from_file_csv(const std::string& input_path) 
{
    std::ifstream file(input_path);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open input file " << input_path << '\n';
        return false;
    }
    
    clear();
    
    std::string line;
    bool first_line = true;
    
    while (std::getline(file, line)) {
        // Skip header
        if (first_line) {
            first_line = false;
            continue;
        }
        
        if (line.empty()) { continue; }
        
        // parse CSV line: word,wordid,freq
        std::stringstream ss(line);
        std::string word;
        std::string word_id_str;
        std::string freq_str;
        
        if (std::getline(ss, word, ',') &&
            std::getline(ss, word_id_str, ',') &&
            std::getline(ss, freq_str, ',')) {
            
            try {
                u32 word_id = std::stoul(word_id_str);
                u32 freq = std::stoul(freq_str);
                
                lexicon[word] = {word_id, freq};
                
                if (word_id >= next_word_id) {
                    next_word_id = word_id + 1;
                }
            } catch (const std::exception& e) {
                std::cerr << "cant parse this line: " << line << '\n';
            }
        }
    }
    
    file.close();
    std::cout << "Lexicon loaded from " << input_path << '\n';
    std::cout << "Total unique terms: " << lexicon.size() << '\n';
    return true;
}

void Lexicon::save_to_file_binary(const std::string& output_path) const
{
    std::ofstream file(output_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open Lexicon file for writing\n";
        return;
    }   

    // convert to vector for sorting (can't sort the hashmap(wtf?))
    std::vector<std::pair<std::string, WordData>> sorted_data(lexicon.begin(), lexicon.end());
    
    // Sort by freq
    std::sort(sorted_data.begin(), sorted_data.end(),
              [](const auto& a, const auto& b) { 
                  return a.second.freq > b.second.freq; 
              });


    // write no of entries
    u32 num_entries = static_cast<u32>(lexicon.size());
    file.write(reinterpret_cast<const char*>(&num_entries), sizeof(num_entries));

    // write all entries (word, word_id, freq)
    for (const auto& [word, word_data] : sorted_data) {
        // write the word
        file.write(word.c_str(), static_cast<long>(word.size() + 1));     // + 1 for /0 
        // write the id
        file.write(reinterpret_cast<const char*>(&word_data.word_id), sizeof(word_data.word_id)); 
        // write the freq
        file.write(reinterpret_cast<const char*>(&word_data.freq), sizeof(word_data.freq));
    }

    file.close();
    std::cout << "\nLexicon saved to " << output_path << '\n';
    std::cout << "  Total unique terms: " << lexicon.size() << '\n';
}

bool Lexicon::load_from_file_binary(const std::string& input_path)
{
    std::ifstream file(input_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open Lexicon file for reading\n";
        return false;
    }

    lexicon.clear();

    // read no of entries
    u32 num_entries;
    file.read(reinterpret_cast<char*>(&num_entries), sizeof(num_entries));

    // read each entry
    for (u32 i = 0; i < num_entries; i++) {
        // read word by finding null terminator
        std::string word;
        std::getline(file, word, '\0');
        // read id
        u32 word_id; 
        file.read(reinterpret_cast<char*>(&word_id), sizeof(word_id));
        // read freq
        u32 freq;
        file.read(reinterpret_cast<char*>(&freq), sizeof(freq));

        // save to lex
        lexicon[word] = { word_id, freq };
    }

    file.close();
    std::cout << "Lexicon loaded from " << input_path << '\n';
    std::cout << "Total unique terms: " << lexicon.size() << '\n';

    return true;
}

void Lexicon::print_top_words(int n) const 
{
    std::vector<std::pair<std::string, WordData>> sorted_data(lexicon.begin(), lexicon.end());

    std::sort(sorted_data.begin(), sorted_data.end(),
              [](const auto& a, const auto& b) { 
                  return a.second.freq > b.second.freq; 
              });
    
    std::cout << "\n  Top " << n << " Most Frequent Terms:\n";
    std::cout << "  " << std::string(50, '-') << '\n';
    for (int i = 0; i < std::min(n, static_cast<int>(sorted_data.size())); ++i) 
    {
        std::cout << "  " << std::setw(4) << (i + 1) << ". "
                  << std::setw(20) << std::left << sorted_data[i].first 
                  << " (ID:" << std::setw(6) << sorted_data[i].second.word_id << ")"
                  << " freq: " << std::setw(8) << std::right << sorted_data[i].second.freq << '\n';
    }
}

void Lexicon::clear() 
{
    lexicon.clear();
    next_word_id = 0;
}


