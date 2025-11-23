#include "text_processor.hpp"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <vector>


TextProcessor::TextProcessor(const std::string& python_path, 
                             const std::string& script_path)
    : python_script_path(script_path), python_interpreter(python_path) 
{
}

bool TextProcessor::call_python_lemmatizer_with_file(const std::string& text,
                                                      std::unordered_map<std::string, uint32_t>& temp_lexicon)
{
    // TODO: check if we can pass a string directly to python rather than an intermediate file

    temp_lexicon.clear();
    
    // Create temporary files
    std::string temp_input = "indices/lemma_input.txt";
    std::string temp_output = "indices/lemma_output.csv";
    
    // Write input text
    std::ofstream input_file(temp_input);
    if (!input_file.is_open()) {
        std::cerr << "Error: Could not create temp input file\n";
        return false;
    }
    input_file << text;
    input_file.close();
    
    // Build command - redirect stderr to separate file to not interfere with CSV
    std::string temp_error = "indices/lemma_error.log";
    std::string command = python_interpreter + " " + python_script_path + 
                         " < " + temp_input + " > " + temp_output + " 2> " + temp_error;
    
    // Execute Python script
    int status = system(command.c_str());
    
    if (status != 0) {
        std::cerr << "Error: Python script failed with status " << status << "\n";
        
        // Cleanup
        remove(temp_input.c_str());
        remove(temp_output.c_str());
       // remove(temp_error.c_str());
        return false;
    }
    
    // Read CSV output
    std::ifstream output_file(temp_output);
    if (!output_file.is_open()) {
        std::cerr << "Error: Could not read temp output file\n";
        remove(temp_input.c_str());
        remove(temp_output.c_str());
        remove(temp_error.c_str());
        return false;
    }
    
    std::string line;
    bool first_line = true;
    
    while (std::getline(output_file, line)) {
        // skip header
        if (first_line) {
            first_line = false;
            continue;
        }
        
        // skip empty lines
        if (line.empty()) { continue; }
        
        // parse CSV line: word,frequency
        size_t comma_pos = line.find(',');
        if (comma_pos != std::string::npos) 
        {
            std::string word = line.substr(0, comma_pos);
            std::string freq_str = line.substr(comma_pos + 1);
            
            try {
                uint32_t freq = std::stoul(freq_str);
                temp_lexicon[word] = freq;
            } catch (const std::exception& e) {
                std::cerr << "Warning: Could not parse frequency for word: " << word << "\n";
            }
        }
    }
    
    output_file.close();
    
    // Cleanup temp files
    remove(temp_input.c_str());
    remove(temp_output.c_str());
    remove(temp_error.c_str());
    
    return !temp_lexicon.empty();
}

void TextProcessor::merge_lexicon(const std::unordered_map<std::string, uint32_t>& temp_lexicon) 
{
    for (const auto& [word, freq] : temp_lexicon) 
    {
        lexicon[word] += freq;
    }
}

bool TextProcessor::process_text(const std::string& text) 
{
    if (text.empty()) {
        return true;
    }
    
    std::unordered_map<std::string, uint32_t> temp_lexicon;
    
    bool success = call_python_lemmatizer_with_file(text, temp_lexicon);
    
    if (success) {
        merge_lexicon(temp_lexicon);
    }
    
    return success;
}

const std::unordered_map<std::string, uint32_t>& TextProcessor::get_lexicon() const 
{
    return lexicon;
}

void TextProcessor::save_lexicon(const std::string& output_path) 
{
    
    std::ofstream file(output_path);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open output file " << output_path << '\n';
        return;
    }
    
    // Convert to vector for sorting
    std::vector<std::pair<std::string, uint32_t>> sorted_lexicon(lexicon.begin(), lexicon.end());
    
    // Sort by frequency (descending)
    std::sort(sorted_lexicon.begin(), sorted_lexicon.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Write to file
    uint32_t docid = 0;
    file << "word,docid,frequency\n";
    for (const auto& [word, freq] : sorted_lexicon) {
        file << word << "," << docid++ << "," << freq << '\n';
    }
    
    file.close();
    std::cout << "\nLexicon saved to " << output_path << '\n';
    std::cout << "  Total unique terms: " << lexicon.size() << '\n';
}

void TextProcessor::print_top_words(int n) 
{
    std::vector<std::pair<std::string, uint32_t>> sorted_lexicon(lexicon.begin(), lexicon.end());
    std::sort(sorted_lexicon.begin(), sorted_lexicon.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::cout << "  Top " << n << " Most Frequent Terms\n";
    for (int i = 0; i < std::min(n, static_cast<int>(sorted_lexicon.size())); ++i) {
        std::cout << std::setw(4) << (i + 1) << ". "
                  << std::setw(25) << std::left << sorted_lexicon[i].first 
                  << std::setw(10) << std::right << sorted_lexicon[i].second << '\n';
    }
}

void TextProcessor::clear_lexicon() 
{
    lexicon.clear();
}
