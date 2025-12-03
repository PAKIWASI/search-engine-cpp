#include "inverted_index.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>



InvertedIndex::InvertedIndex()
{
    barrels.reserve(num_barrel);
}


InvertedIndex::InvertedIndex(const std::string& path_barrels_metadata)
{
    std::ifstream file(path_barrels_metadata, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Cound not open: " << path_barrels_metadata << '\n';
        return;
    }

    barrels.reserve(num_barrel);

    // read the terms
    file.read(reinterpret_cast<char*>(barrels.data()), num_barrel * sizeof(Barrel));
}


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

    return nullptr;  // not found
}

void InvertedIndex::save_to_file(const std::string& output_path) 
{
    // we store as binary for fast save/load
    std::ofstream file(output_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open inverted index file for writing\n";
        return;
    }

    // sort all vector elms by doc_id
    for (auto& [word_id, terms] : inverted_index) {  
        std::sort(terms.begin(), terms.end(),
                  [](const InvertedEntry& a, const InvertedEntry& b) {
                      return a.doc_id < b.doc_id;
                  });
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


                        // This ouput path should be only folder path
void InvertedIndex::save_barrels(const std::string& output_folder_path)
{
    // create container for sorted inverted_index
    std::vector<std::pair<uint32_t, std::vector<InvertedEntry>>> sorted;
    sorted.reserve(inverted_index.size());

    // move the vectors out of the map
    for (auto& [word_id, entries] : inverted_index) {
        sorted.emplace_back(word_id, std::move(entries));
    }

    //NOTE:  now inverted_index contains empty vectors after moving
    
    // clear the map if needed:
    inverted_index.clear();

    // first sort all vector elms by doc_id
    for (auto& [word_id, terms] : sorted) {  
        std::sort(terms.begin(), terms.end(),
                  [](const InvertedEntry& a, const InvertedEntry& b) {
                      return a.doc_id < b.doc_id;
                  });
    }

    // sort the inverted_index itself by word_id
    std::sort(sorted.begin(), sorted.end(),
        [](const auto& a, const auto& b) {
            return a.first < b.first;
        });


    // we have sorted data now

    uint32_t curr_barrel_id = 0;
    uint32_t range_size = sorted.size() / num_barrel; 
    uint32_t range_start = 0;

    for (uint32_t i = 0; i < num_barrel; i++) {

        // create a barrel

        uint32_t range_end = range_start + range_size;
        if (range_end > sorted.size() || i == num_barrel - 1) {
            range_end = sorted.size();
        }

        barrels.push_back({curr_barrel_id, range_start, range_end});


        // save the barrel to a binary file
        
        std::ofstream file(output_folder_path + std::to_string(curr_barrel_id) + ".bin", std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open inverted index barrel file for writing\n";
            return;
        }

        std::ofstream text_file(output_folder_path + std::to_string(curr_barrel_id) + ".txt");
        if (!text_file.is_open()) {
            std::cerr << "Error: Cannot open invertd index barrel txt file for wriing\n";
            return;
        }

        // write the range
        for (uint32_t j = range_start; j < range_end; j++) {

            uint32_t word_id = sorted[j].first;
            std::vector<InvertedEntry>* terms = &sorted[j].second;

            // write word_id
            file.write(reinterpret_cast<const char*>(&word_id), sizeof(word_id));
            text_file << word_id << ' ';

            // write no of terms
            uint32_t term_count = static_cast<uint32_t>(terms->size());
            file.write(reinterpret_cast<const char*>(&term_count), sizeof(term_count));
            text_file << term_count << ' ';

            // write all terms
            file.write(reinterpret_cast<const char*>(terms->data()),
                    term_count * sizeof(InvertedEntry));

            for (const auto& [doc_id, freq] : *terms) {
                text_file<< "( " << doc_id << ", " << freq << " ), ";
            }

            text_file << '\n';
        }


        std::cout << "Inverted Index Barrel No. " << i << " saved to " << output_folder_path << " folder\n"; 
        file.close();
        text_file.close();

        range_start = range_end;
        curr_barrel_id++;
    }


    // now write the barrels vector metadata to a file
    std::ofstream metadata(output_folder_path + "barrel_metadata.bin", std::ios::binary);
    if (!metadata.is_open()) {
        std::cerr << "Error: Cannot open barrel metadata file for writing\n";
        return;
    }

    // write the terms
    metadata.write(reinterpret_cast<const char*>(barrels.data()), 
                   num_barrel * sizeof(Barrel));

    metadata.close();
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


bool InvertedIndex::load_barrel(const std::string& input_folder_path, const uint32_t& word_id)
{

    // detemine the barrel no
    for (const auto& barrel : barrels) {
        // TODO: verify ranges (what if word_id is last id ?)
        if (word_id >= barrel.start_word_id && word_id <= barrel.end_word_id) {
            
            // load the barrel
            if (load_from_file(input_folder_path + std::to_string(barrel.barrel_id) + ".bin")) {
                std::cout << "Barrel no. " << barrel.barrel_id << " Loaded\n";
                return true;
            }
            else {
                std::cerr << "Barrel no. " << barrel.barrel_id << " NOT Loaded\n";
                return false;
            }
        }
    }

    std::cerr << "Could not find barrel for word_id: " << word_id << '\n';
    return false;
}


void InvertedIndex::save_as_text(const std::string& output_path, 
                                 const std::unordered_map<uint32_t, std::string>& reverse_lex)
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
     
    std::cout << "Inverted index saved to " << output_path << '\n';
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


