/*
#include "metadata_parser.hpp"
#include "inverted_index.hpp"
#include "nlohmann_json.hpp"
#include "text_processor.hpp"
#include "lexicon.hpp"
#include "forward_index.hpp"

#include <iostream> 
#include <fstream>  
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>

using json = nlohmann::json;

#define HEADER_SIZE 18
#define CORD_UID    0
#define SHA         1
#define TITLE       3
#define PMC_ID      5
#define ABSTRACT    8


// Shared data structures with thread safety
struct SharedData {
    std::mutex lexicon_mutex;
    std::mutex forward_mutex;
    std::mutex inverted_mutex;
    std::mutex cout_mutex;  // For thread-safe console output
    
    Lexicon lexicon;
    ForwardIndex forward_index;
    InvertedIndex inverted_index;
    
    std::atomic<u32> paper_count{0};
    std::atomic<u32> lemmatize_count{0};
    std::atomic<u32> failed_count{0};
    std::atomic<u32> skipped_count{0};
    std::atomic<u32> pdf_count{0};
    std::atomic<u32> xml_count{0};
    std::atomic<u32> not_found{0};
    std::atomic<bool> should_stop{false};
    
    SharedData() : inverted_index("indices/") {}
};

// Work item for processing
struct PaperWork {
    u32 paper_id;
    std::vector<std::string> parsed_line;
};

// Thread-safe work queue
class WorkQueue {
private:
    std::queue<PaperWork> queue;
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;
    size_t max_size = 10000;  // Limit queue size to prevent memory issues
    
public:
    void push(PaperWork work) 
    {
        std::unique_lock<std::mutex> lock(mutex);
        // Block if queue is too large
        cv.wait(lock, [this] { return queue.size() < max_size || done; });
        
        if (!done) {
            queue.push(std::move(work));
        }
        lock.unlock();
        cv.notify_one();
    }
    
    bool pop(PaperWork& work) 
    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this] { return !queue.empty() || done; });
        
        if (queue.empty()) {
            return false;
        }
        
        work = std::move(queue.front());
        queue.pop();
        lock.unlock();
        cv.notify_all();  // Notify producers that space is available
        return true;
    }
    
    void set_done() 
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            done = true;
        }
        cv.notify_all();
    }
    
    size_t size() 
    {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.size();
    }
};

// Worker thread function
void worker_thread(WorkQueue& work_queue, SharedData& shared, 
                   const std::string& data_path, u32 thread_id) 
{
    try {
        // Each thread gets its own TextProcessor with its own Python daemon
        TextProcessor text_processor(
            shared.lexicon,
            "python/.venv/bin/python3", 
            "python/lemmatizer_daemon.py"
        );
        
        {
            std::lock_guard<std::mutex> lock(shared.cout_mutex);
            std::cout << "Thread " << thread_id << " started with daemon\n";
        }
        
        std::unordered_map<std::string, WordData> temp_lex;
        u32 processed_this_thread = 0;
        
        while (!shared.should_stop.load()) {
            PaperWork work;
            if (!work_queue.pop(work)) {
                break; // Queue is done
            }
            
            u32 paper_id = work.paper_id;
            auto& parsed_line = work.parsed_line;
            
            try {
                // Build text as a string
                std::string full_text;
                full_text.reserve(50000);  // Reserve space to avoid reallocation
                
                // Add title
                if (parsed_line.size() > TITLE && !parsed_line[TITLE].empty()) {
                    full_text += parsed_line[TITLE];
                    full_text += ' ';
                }
                
                // Add abstract
                if (parsed_line.size() > ABSTRACT && !parsed_line[ABSTRACT].empty()) {
                    full_text += parsed_line[ABSTRACT];
                    full_text += ' ';
                }
                
                // Try to find PDFs first
                bool pdf_found = false;
                bool xml_found = false;
                
                if (parsed_line.size() > SHA && !parsed_line[SHA].empty()) {
                    std::string path_pdf = MetadataParser::find_fulltext_pdf_static(
                        parsed_line[SHA], data_path);
                    if (!path_pdf.empty()) {
                        MetadataParser::extract_body_text_static(path_pdf, full_text);
                        pdf_found = true;
                        shared.pdf_count++;
                    }
                }
                
                // If no PDF, try XML
                if (!pdf_found && parsed_line.size() > PMC_ID && !parsed_line[PMC_ID].empty()) {
                    std::string path_xml = MetadataParser::find_fulltext_xml_static(
                        parsed_line[PMC_ID], data_path);
                    if (!path_xml.empty()) {
                        MetadataParser::extract_body_text_static(path_xml, full_text);
                        xml_found = true;
                        shared.xml_count++;
                    }
                }
                
                if (!pdf_found && !xml_found) {
                    shared.not_found++;
                }
                
                if (!full_text.empty()) {
                    // Clear temp_lex before use
                    temp_lex.clear();
                    
                    // Lemmatize text
                    bool success = text_processor.lemmatize_text(full_text, temp_lex);
                    
                    if (success && !temp_lex.empty()) {
                        shared.lemmatize_count++;
                        processed_this_thread++;
                        
                        std::string cord_uid = parsed_line.size() > CORD_UID ? 
                                              parsed_line[CORD_UID] : "";
                        
                        // Thread-safe operations on shared data structures
                        u32 doc_id;
                        {
                            std::lock_guard<std::mutex> lock(shared.forward_mutex);
                            doc_id = shared.forward_index.add_document(cord_uid, temp_lex);
                        }
                        
                        {
                            std::lock_guard<std::mutex> lock(shared.inverted_mutex);
                            shared.inverted_index.add_document(doc_id, temp_lex);
                        }
                    } else {
                        shared.failed_count++;
                    }
                } else {
                    shared.skipped_count++;
                }
                
            } catch (const std::exception& e) {
                std::lock_guard<std::mutex> lock(shared.cout_mutex);
                std::cerr << "Thread " << thread_id << " error processing paper " 
                          << paper_id << ": " << e.what() << "\n";
                shared.failed_count++;
            }
            
            // Progress indicator (less frequent to reduce contention)
            u32 current_count = shared.lemmatize_count.load();
            if (current_count % 500 == 0 && processed_this_thread % 100 == 0) {
                std::lock_guard<std::mutex> lock(shared.cout_mutex);
                std::cout << "Progress: " << current_count 
                          << " processed, " << shared.lexicon.size() << " unique terms, "
                          << "Queue: " << work_queue.size() << "\n";
            }
        }
        
        {
            std::lock_guard<std::mutex> lock(shared.cout_mutex);
            std::cout << "Thread " << thread_id << " finished (processed " 
                      << processed_this_thread << " papers)\n";
        }
        
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(shared.cout_mutex);
        std::cerr << "Thread " << thread_id << " fatal error: " << e.what() << "\n";
        shared.should_stop.store(true);
    }
}


// Main multithreaded parser
int MetadataParser::metadata_parse_multithreaded(u32 num_threads) 
{
    
    std::ifstream file(data_path + "/metadata.csv");
    if (!file.is_open()) {
        std::cout << "Error: can't open metadata.csv\n";
        return -1;
    }
    
    std::string line;
    
    // Read header
    if (!std::getline(file, line)) {
        std::cerr << "Error: Empty metadata file\n";
        return -1;
    }
    
    std::vector<std::string> parsed_line;
    parse_csv_line(line, parsed_line);
    
    std::cout << "\nStarting multithreaded paper processing with " 
              << num_threads << " threads...\n";
    std::cout << "Note: Queue size limited to 10000 to prevent memory issues\n\n";
    
    // Shared data and work queue
    SharedData shared;
    WorkQueue work_queue;
    
    // Start worker threads
    std::vector<std::thread> threads;
    for (u32 i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker_thread, std::ref(work_queue), 
                           std::ref(shared), data_path, i);
    }
    
    // Read CSV and enqueue work
    u32 paper_id = 0;
    const u32 START_FROM = 5001;      // Start from first paper
    const u32 LIMIT_TO = 10000;    // Process up to 50k papers
    
    try {
        while (std::getline(file, line) && !shared.should_stop.load()) {
            paper_id++;
            
            parse_csv_line(line, parsed_line);
            
            // Skip papers before START_FROM
            if (paper_id < START_FROM) { continue; }
            
            // Stop at LIMIT_TO
            if (paper_id >= LIMIT_TO) { break; }
            
            shared.paper_count++;
            
            PaperWork work;
            work.paper_id = paper_id;
            work.parsed_line = parsed_line;
            
            work_queue.push(std::move(work));
            
            // Progress for reading
            if (paper_id % 1000 == 0) {
                std::cout << "===Read " << paper_id << " papers from CSV...\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading CSV: " << e.what() << "\n";
        shared.should_stop.store(true);
    }
    
    file.close();
    std::cout << "Finished reading CSV. Waiting for workers to complete...\n";
    
    // Signal workers that we're done adding work
    work_queue.set_done();
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    // Print statistics
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "PROCESSING SUMMARY\n";
    std::cout << std::string(60, '=') << "\n";
    std::cout << "Total papers read:       " << shared.paper_count.load() << "\n";
    std::cout << "Successfully Lemmatized: " << shared.lemmatize_count.load() << "\n";
    std::cout << "Failed:                  " << shared.failed_count.load() << "\n";
    std::cout << "Skipped (no text):       " << shared.skipped_count.load() << "\n";
    std::cout << "PDF's Found:             " << shared.pdf_count.load() << "\n";
    std::cout << "XML's Found:             " << shared.xml_count.load() << "\n";
    std::cout << "Not Found (No PDF, XML): " << shared.not_found.load() << "\n";
    std::cout << "Unique terms in lexicon: " << shared.lexicon.size() << "\n";
    
    // Print top terms
    shared.lexicon.print_top_words(50);
    
    // Print forward index stats
    shared.forward_index.print_statistics();
    
    // Print inverted index stats
    shared.inverted_index.print_statistics();
    
    std::cout << "\nSaving indices...\n";
    
    // Save results
    try {
        shared.lexicon.save_to_file_binary("indices/2/lexicon_cordR1.bin");
        shared.forward_index.save_to_file("indices/2/forward_index_cordR1.bin");
        shared.inverted_index.save_to_file("indices/2/inverted_index_cordR1.bin");
        std::cout << "All indices saved successfully!\n";
    } catch (const std::exception& e) {
        std::cerr << "Error saving indices: " << e.what() << "\n";
        return -1;
    }
    
    return 0;
}

*/
