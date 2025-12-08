#include "forward_index.hpp"
#include "inverted_index.hpp"
#include "lexicon.hpp"

#include <iostream>


// WARN: will take aprox 8.3 hours on my cpu to get 50k docs on a single thread

int merge_lex();
int merge_forward();
int merge_inverted();

int main()
{
    /*
    // builder phase
    const std::string data_path = "data/2020-04-10"; 

    MetadataParser parser(data_path);

    parser.metadata_parse_multithreaded(6);
    */
    
    return merge_inverted();
    return 0;
}


int merge_lex()
{
    Lexicon lex;
    lex.load_from_file_binary("indices/1/lexicon_cordR1.bin");

    lex.merge_from_file_binary("indices/2/lexicon_cordR1.bin", lex.size());

    lex.save_to_file_binary("indices/merged/lexicon_12.bin"); 

    return 0;
}

int merge_forward()
{
    ForwardIndex forward;
    forward.load_from_file("indices/1/forward_index_cordR1.bin");
    forward.print_statistics();

    forward.merge_from_file("indices/2/forward_index_cordR1.bin", forward.size());

    forward.print_statistics();

    forward.save_to_file("indices/merged/forward_12.bin");

    return 0;
}

int merge_inverted()
{
    Lexicon merged_lex;
    merged_lex.load_from_file_binary("indices/merged/lexicon_12.bin");
    std::cout << "Merged lexicon has " << merged_lex.size() << " unique words\n";
    
    
    ForwardIndex forward;
    forward.load_from_file("indices/merged/forward_12.bin");
    forward.print_statistics();
    
    
    std::cout << "\n=== STEP 3: Rebuilding Inverted Index from Forward Index ===\n";
    
    InvertedIndex inverted;
    
    u32 doc_count = forward.get_document_count();
    std::cout << "Processing " << doc_count << " documents...\n";
    
    // Iterate through all documents in forward index
    for (u32 doc_id = 0; doc_id < doc_count; doc_id++) {
        const std::vector<WordData>* terms = forward.get_document_terms(doc_id);
        
        if (terms == nullptr) {
            std::cerr << "Warning: doc_id " << doc_id << " not found\n";
            continue;
        }
        
        // For each word in this document, add to inverted index
        for (const auto& word_data : *terms) {
            u32 word_id = word_data.word_id;
            u32 freq = word_data.freq;
            
            InvertedEntry entry = {doc_id, freq};
            inverted.addEntry(word_id, entry);
        }
        
        // Progress indicator
        if ((doc_id + 1) % 1000 == 0) {
            std::cout << "  Processed " << (doc_id + 1) << "/" << doc_count 
                      << " documents\n";
        }
    }
    
    inverted.print_statistics();
    inverted.save_to_file("indices/merged/inverted_12.bin");
    
    return 0;
}

