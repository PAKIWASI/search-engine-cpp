#include "lexicon.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>



uint32_t Lexicon::add_word(const std::string& word, uint32_t frequency) 
{
    auto it = data.find(word);
    if (it != data.end()) {
        it->second.second += frequency;   // word exists, update frequency
        return it->second.first;
    } else {
        uint32_t word_id = next_word_id++;  // New word, assign new ID
        data[word] = {word_id, frequency};
        return word_id;
    }
}

const std::pair<uint32_t, uint32_t>* 
            Lexicon::get_word_info(const std::string& word) const 
{
    auto it = data.find(word);
    if (it != data.end()) {
        return &it->second;
    }
    return nullptr;
}

uint32_t Lexicon::get_word_id(const std::string& word) const 
{
    auto it = data.find(word);
    if (it != data.end()) {
        return it->second.first;
    }
    return UINT32_MAX;      // return max as not found
}

uint32_t Lexicon::get_frequency(const std::string& word) const 
{
    auto it = data.find(word);
    if (it != data.end()) {
        return it->second.second;
    }
    return 0;
}

bool Lexicon::contains(const std::string& word) const 
{
    return data.contains(word);
}

void Lexicon::merge(const std::unordered_map<std::string, std::pair<uint32_t, uint32_t>>& temp_lex) 
{
    for (const auto& [word, pair] : temp_lex) 
    {
        add_word(word, pair.second);
    }
}

void Lexicon::update_ids(std::unordered_map<std::string, std::pair<uint32_t, uint32_t>>& temp_lex) const 
{
    for (auto& [word, pair] : temp_lex) {
        auto it = data.find(word);
        if (it != data.end()) {
            pair.first = it->second.first;  // update with actual ID
        }
    }
}

void Lexicon::save_to_file(const std::string& output_path) const 
{
    std::ofstream file(output_path);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open output file " << output_path << '\n';
        return;
    }
    
    // convert to vector for sorting (can't sort the hashmap(wtf?))
    std::vector<std::pair<std::string, std::pair<uint32_t, uint32_t>>> sorted_data(data.begin(), data.end());
    
    // Sort by frequency
    std::sort(sorted_data.begin(), sorted_data.end(),
              [](const auto& a, const auto& b) { 
                  return a.second.second > b.second.second; 
              });
    
    // write to file
    file << "word,wordid,frequency\n";
    for (const auto& [word, pair] : sorted_data) {
        file << word << "," << pair.first << "," << pair.second << '\n';
    }
    
    file.close();
    std::cout << "\nLexicon saved to " << output_path << '\n';
    std::cout << "  Total unique terms: " << data.size() << '\n';
}

bool Lexicon::load_from_file(const std::string& input_path) 
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
        
        // parse CSV line: word,wordid,frequency
        std::stringstream ss(line);
        std::string word;
        std::string word_id_str;
        std::string freq_str;
        
        if (std::getline(ss, word, ',') &&
            std::getline(ss, word_id_str, ',') &&
            std::getline(ss, freq_str, ',')) {
            
            try {
                uint32_t word_id = std::stoul(word_id_str);
                uint32_t frequency = std::stoul(freq_str);
                
                data[word] = {word_id, frequency};
                
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
    std::cout << "Total unique terms: " << data.size() << '\n';
    return true;
}

void Lexicon::print_top_words(int n) const 
{
    std::vector<std::pair<std::string, std::pair<uint32_t, uint32_t>>> sorted_data(data.begin(), data.end());
    std::sort(sorted_data.begin(), sorted_data.end(),
              [](const auto& a, const auto& b) { 
                  return a.second.second > b.second.second; 
              });
    
    std::cout << "\n  Top " << n << " Most Frequent Terms:\n";
    std::cout << "  " << std::string(50, '-') << '\n';
    for (int i = 0; i < std::min(n, static_cast<int>(sorted_data.size())); ++i) 
    {
        std::cout << "  " << std::setw(4) << (i + 1) << ". "
                  << std::setw(20) << std::left << sorted_data[i].first 
                  << " (ID:" << std::setw(6) << sorted_data[i].second.first << ")"
                  << " freq: " << std::setw(8) << std::right << sorted_data[i].second.second << '\n';
    }
}

void Lexicon::clear() 
{
    data.clear();
    next_word_id = 0;
}



