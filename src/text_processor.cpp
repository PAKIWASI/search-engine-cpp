#include "text_processor.hpp"

#include <fstream>
#include <iostream>
#include <cstdio>
#include <cstdlib>


TextProcessor::TextProcessor( Lexicon& lex,
    const std::string& python_path, 
    const std::string& script_path)
    : lexicon(lex), 
      python_script_path(script_path), 
      python_interpreter(python_path) 
{
}

bool TextProcessor::call_python_lemmatizer_with_text( std::ofstream& lemma_input,
                        std::unordered_map<std::string, WordData>& temp_lex)
{
    temp_lex.clear();
    
    // Create temporary files
    std::string temp_input = "indices/lemma_input.txt";     // already created and has full_text
    std::string temp_output = "indices/lemma_output.csv";
    
    lemma_input.close();
    
    // Build command - redirect stderr to separate file to not interfere with CSV
    std::string temp_error = "indices/lemma_error.log";
    std::string command = python_interpreter + " " + python_script_path + 
                         " < " + temp_input + " > " + temp_output + 
                         " 2> " + temp_error;
    
    // Execute Python script
    int status = system(command.c_str());
    
    if (status != 0) {
        std::cerr << "Error: Python script failed with status " << status << "\n";
        
        // Cleanup
        remove(temp_input.c_str());
        remove(temp_output.c_str());
        return false;
    }
    

                // READ CSV
    std::ifstream output_file(temp_output);
    if (!output_file.is_open()) {
        std::cerr << "Error: Could not read temp output file\n";
        remove(temp_input.c_str());
        remove(temp_output.c_str());
        remove(temp_error.c_str());
        return false;
    }
    
    std::string line;
    bool first_line = true; // for header
    
    while (std::getline(output_file, line)) 
    {
        // skip header
        if (first_line) {
            first_line = false;
            continue;
        }
        
        // skip empty lines
        if (line.empty()) { continue; }
        
        // parse CSV line: word,frequency
        size_t comma_pos = line.find(',');
        if (comma_pos != std::string::npos) {
            std::string word = line.substr(0, comma_pos);
            std::string freq_str = line.substr(comma_pos + 1);
            
            try {
                uint32_t freq = std::stoul(freq_str);
                // store word with frequency, id will be assigned during merge
                temp_lex[word] = {0, freq};  // id=0 as placeholder
            } catch (const std::exception& e) {
                std::cerr << "Warning: Could not parse frequency for word: " 
                         << word << "\n";
            }
        }
    }
    
    output_file.close();
    
    // Cleanup temp files
    remove(temp_input.c_str());
    remove(temp_output.c_str());
    remove(temp_error.c_str());
    
    return !temp_lex.empty();
}



bool TextProcessor::lemmatize_text( std::ofstream& full_text, 
                    std::unordered_map<std::string, WordData>& temp_lex) 
{
    if (!full_text.is_open()) {
        return true;
    }
    
    // Call Python lemmatizer to get temp_lex
    bool success = call_python_lemmatizer_with_text(full_text, temp_lex);
    
    if (success) {
        // Merge temp_lex into global lexicon
        lexicon.merge(temp_lex);
        
        // Update temp_lex with actual IDs from global lexicon
        lexicon.update_ids(temp_lex);
    }
    
    return success;
}


