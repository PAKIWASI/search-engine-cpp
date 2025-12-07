#include "inverted_index.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>


InvertedIndex::InvertedIndex()
{
    barrels.reserve(num_barrel);
}


InvertedIndex::InvertedIndex(const std::string& barrel_folder_path)
{
    std::ifstream file(barrel_folder_path + "barrel_metadata.bin", std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Cound not open: " << barrel_folder_path << '\n';
        return;
    }

    barrels.resize(num_barrel);

    // read the terms
    if (!file.read(reinterpret_cast<char*>(barrels.data()), num_barrel * sizeof(Barrel)))
    {
        std::cerr << "barrel_metadata NOT found in: " << barrel_folder_path << '\n';
    }

    barrel_path = barrel_folder_path;
}


void InvertedIndex::addEntry(u32 word_id, const InvertedEntry& entry)
{
    auto it = inverted_index.find(word_id);
    if (it != inverted_index.end()) {       // already exist
        it->second.push_back( entry ); 
    }
    else {                  // new entry
        inverted_index[word_id] = { entry };
    }
}

void InvertedIndex::add_document(u32 doc_id, const std::unordered_map<std::string, WordData>& temp_lex)
{
    for (const auto& [word, word_info] : temp_lex) {
        inverted_index[word_info.word_id].push_back( { doc_id, word_info.freq } );
    }
}

const std::vector<InvertedEntry>* InvertedIndex::get_word_terms(u32 word_id) const
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
    u32 word_count = static_cast<u32>(inverted_index.size());
    file.write(reinterpret_cast<const char*>(&word_count), sizeof(word_count));


    // write each word entry
    for (const auto& [word_id, terms] : inverted_index) {

        // write word_id
        file.write(reinterpret_cast<const char*>(&word_id), sizeof(word_id));

        // write no of terms
        u32 term_count = static_cast<u32>(terms.size());
        file.write(reinterpret_cast<const char*>(&term_count), sizeof(term_count));

        // write all terms
        file.write(reinterpret_cast<const char*>(terms.data()),
                   term_count * sizeof(InvertedEntry));
    }

    file.close();
    std::cout << "Inverted index saved to " << output_path << '\n';
}


                        // This ouput path should be only folder path
void InvertedIndex::save_barrels()
{
    // create container for sorted inverted_index
    std::vector<std::pair<u32, std::vector<InvertedEntry>>> sorted;
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

    u32 range_size = sorted.size() / num_barrel; 
    u32 range_start = 0;

    for (u32 i = 0; i < num_barrel; i++) {

        // create a barrel

        u32 range_end = range_start + range_size;
        if (range_end > sorted.size() || i == num_barrel - 1) {
            range_end = sorted.size();
        }

        barrels.push_back({range_start, range_end - 1});

        // save the barrel to a binary file
        
        std::ofstream file(barrel_path + std::to_string(i) + ".bin", std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open inverted index barrel file for writing\n";
            return;
        }

        // the load_from_file func requres a count
        u32 words_in_barrel = range_end - range_start;
        file.write(reinterpret_cast<const char*>(&words_in_barrel), sizeof(words_in_barrel));

        // write the range
        for (u32 j = range_start; j < range_end; j++) {

            u32 word_id = sorted[j].first;
            std::vector<InvertedEntry>* terms = &sorted[j].second;

            // write word_id
            file.write(reinterpret_cast<const char*>(&word_id), sizeof(word_id));

            // write no of terms
            u32 term_count = static_cast<u32>(terms->size());
            file.write(reinterpret_cast<const char*>(&term_count), sizeof(term_count));

            // write all terms
            file.write(reinterpret_cast<const char*>(terms->data()),
                    term_count * sizeof(InvertedEntry));

        }


        std::cout << "Inverted Index Barrel No. " << i << " saved to " << barrel_path << " folder\n"; 
        file.close();

        range_start = range_end;
    }


    // now write the barrels vector metadata to a file
    std::ofstream metadata(barrel_path + "barrel_metadata.bin", std::ios::binary);
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
    u32 word_count;
    file.read( reinterpret_cast<char*>(&word_count), sizeof(word_count));

    // read each entry
    for (u32 i = 0; i < word_count; i++) {

        // read word_id
        u32 word_id;
        file.read(reinterpret_cast<char*>(&word_id), sizeof(word_id));

        // read no of terms
        u32 term_count;
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


bool InvertedIndex::load_barrel(u32 word_id)
{
    // detemine the barrel no
    for (u32 i = 0; i < num_barrel; i++) {
        if (word_id >= barrels[i].start_word_id && word_id <= barrels[i].end_word_id) 
        {
            // check if we already have that barrel in RAM
            if (i == curr_barrel) { return true; }

            // if not, load the barrel
            if (load_from_file(barrel_path + std::to_string(i) + ".bin")) 
            {
                curr_barrel = i;                    // set curr barrel
                std::cout << "Barrel no. " << i << " Loaded\n";
                return true;
            }
            else {
                std::cerr << "Barrel no. " << i << " NOT Loaded\n";
                return false;
            }
        }
    }

    std::cerr << "Could not find barrel for word_id: " << word_id << '\n';
    return false;
}


void InvertedIndex::save_as_text(const std::string& output_path, 
                                 const std::unordered_map<u32, std::string>& reverse_lex)
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



u32 InvertedIndex::get_total_docs(u32 word_id)
{
    auto it = inverted_index.find(word_id);
    if (it != inverted_index.end()) {       // already exist
        return static_cast<u32>(it->second.size());
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

        if (!barrels.empty() && curr_barrel != UINT32_MAX) {
            std::cout << "Current Loaded Barrel No: " << curr_barrel << '\n'; 
        }
                
        std::cout << "  Total Docs (Repeated):          " << total_docs << '\n';
        std::cout << "  Avg doc per word:   " << avg_postings << '\n';
        std::cout << "  Min docs for a word: " << min_docs << '\n';
        std::cout << "  Max docs for a word: " << max_docs << '\n';
    }
}


