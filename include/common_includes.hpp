#pragma once

#include <cstdint>
#include <string>



typedef uint32_t u32;


struct WordData {
    u32 word_id;     // assigned id 
    u32 freq;        // freq of the word
};


struct InvertedEntry {
    u32 doc_id;     // assigned id
    u32 freq;       // freq of a word in that doc
};


/*      WE JUST NEED PMC ID
https://pmc.ncbi.nlm.nih.gov/articles/PMC1435788/
*/

struct DocumentMetadata {
    std::string cord_uid;
    std::string title;
    std::string abstract;
    std::string pmcid;
    
    DocumentMetadata() = default;
    DocumentMetadata(const std::string& uid, const std::string& t, 
                    const std::string& a, const std::string& u)
        : cord_uid(uid), title(t), abstract(a), pmcid(u) {}
};
