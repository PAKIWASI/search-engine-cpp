#include "text_processor.hpp"

#include <algorithm>
#include <iostream>
#include <sys/wait.h>
#include <cstring>



TextProcessor::TextProcessor(Lexicon& lex,
    const std::string& python_path, 
    const std::string& script_path)
    : lexicon(lex), python_in(nullptr), python_out(nullptr), 
      python_pid(-1), daemon_active(false)
{
    if (!start_daemon(python_path, script_path)) {
        std::cerr << "failed to start python daemon\n";
    }
}

TextProcessor::~TextProcessor() 
{
    stop_daemon();
}

bool TextProcessor::start_daemon(const std::string& python_path,
                                 const std::string& script_path) 
{
            // we are using pipes for persistant communication

    int pipe_in[2];   // parent writes, child reads (stdin for python)
    int pipe_out[2];  // child writes, parent reads (stdout from python)
    
    if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1) {
        std::cerr << "Failed to create pipes\n";
        return false;
    }
    
    python_pid = fork(); 
    
    if (python_pid == -1) {
        std::cerr << "Failed to fork\n"; 
        close(pipe_in[0]); close(pipe_in[1]);
        close(pipe_out[0]); close(pipe_out[1]);
        return false;
    }
    
    if (python_pid == 0) 
    {
        // CHILD PROCESS 
        close(pipe_in[1]);   // close write end of input pipe
        close(pipe_out[0]);  // close read end of output pipe
        
        // redirect stdin to pipe_in[0]
        if (dup2(pipe_in[0], STDIN_FILENO) == -1) {
            std::cerr << "Failed to redirect stdin\n";
            exit(1);
        }
        
        // redirect stdout to pipe_out[1]
        if (dup2(pipe_out[1], STDOUT_FILENO) == -1) {
            std::cerr << "Failed to redirect stdout\n";
            exit(1);
        }
        
        close(pipe_in[0]);
        close(pipe_out[1]);
        
        // execute python daemon
        execlp(python_path.c_str(), python_path.c_str(), 
               script_path.c_str(), nullptr);
        
        // If execlp fails, we only reach here
        std::cerr << "Failed to execute Python\n";
        exit(1);
    }
    
    // PARENT PROCESS
    close(pipe_in[0]);   // close read end of input pipe
    close(pipe_out[1]);  // close write end of output pipe
    
    // convert file descriptors to FILE* for easier I/O  (ai propossed solution, idk it works or not)
    python_in = fdopen(pipe_in[1], "w");
    python_out = fdopen(pipe_out[0], "r");
    
    if ((python_in == nullptr) || (python_out == nullptr)) {
        std::cerr << "Failed to open pipe streams\n";
        stop_daemon();
        return false;
    }
    
    // Disable buffering for immediate communication
    setbuf(python_in, nullptr);
    setbuf(python_out, nullptr);
    
    daemon_active = true;
    std::cout << "Python daemon started (PID: " << python_pid << ")\n";
    return true;
}

void TextProcessor::stop_daemon() 
{
    if (!daemon_active) { return; }
    
    // Signal daemon to shutdown
    if (python_in != nullptr) {
        fprintf(python_in, "0\n");
        fflush(python_in);
        fclose(python_in);
        python_in = nullptr;
    }
    
    if (python_out != nullptr) {
        fclose(python_out);
        python_out = nullptr;
    }
    
    // terminate python process
    if (python_pid > 0) {
        int status;
        
        // wait briefly for graceful shutdown (important, apparently)
        pid_t result = waitpid(python_pid, &status, WNOHANG);
        
        if (result == 0) {
            // process still running, send SIGTERM
            kill(python_pid, SIGTERM);
            
            // wait up to 1 second
            for (int i = 0; i < 10; i++) {
                result = waitpid(python_pid, &status, WNOHANG);
                if (result != 0) { break; }
                usleep(100000); // 100ms
            }
            
            // force kill if still alive
            if (result == 0) {
                kill(python_pid, SIGKILL);
                waitpid(python_pid, &status, 0);
            }
        }
        
        python_pid = -1;
    }
    
    daemon_active = false;
    std::cout << "Python daemon stopped\n";
}


bool TextProcessor::process_with_daemon(const std::string& text,
                    std::unordered_map<std::string, WordData>& temp_lex) 
{
    temp_lex.clear();
    
    if (!daemon_active || (python_in == nullptr) || (python_out == nullptr)) {
        std::cerr << "Python daemon not active\n";
        return false;
    }

    std::cout << "DEBUG: Sending text of length " << text.length() << " to daemon\n";
    
    // send text as a single line (text is a line with space b/w sections)
    
    fprintf(python_in, "%s\n", text.c_str());
    fflush(python_in);
    
    // read CSV output
    char line[4096];
    bool first_line = true;
    
    // get the response
    while (fgets(line, sizeof(line), python_out) != nullptr) 
    {
        
        // remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
            len--;
        }
        
        std::string line_str(line);
        
        // check for end marker
        if (line_str == "END_OF_DOCUMENT") {
            break;
        }
        
        // skip header line
        if (first_line) {
            first_line = false;
            continue;
        }
        
        if (line_str.empty()) { continue; }
        
        // parse CSV: word,freq
        size_t comma_pos = line_str.find(',');
        if (comma_pos != std::string::npos) {
            std::string word = line_str.substr(0, comma_pos);
            std::string freq_str = line_str.substr(comma_pos + 1);
            
            try {
                u32 freq = std::stoul(freq_str);

                temp_lex[word] = {0, freq};         // id = 0 is placeholder
                
            } catch (const std::exception& e) {
                std::cerr << "Parse error for word '" << word 
                         << "': " << e.what() << '\n';
            }
        }
    }
    
    std::cout << "Processed " << temp_lex.size() << " terms\n";
    return !temp_lex.empty();
}


bool TextProcessor::lemmatize_text(std::string& text,
                    std::unordered_map<std::string, WordData>& temp_lex) 
{
    temp_lex.clear();

    if (text.empty()) {
        return true;
    }


     // replace newlines and carriage returns with spaces (in-place)
    std::replace(text.begin(), text.end(), '\n', ' ');
    std::replace(text.begin(), text.end(), '\r', ' ');
    std::replace(text.begin(), text.end(), '\t', ' ');
    
    // remove null bytes and other control characters (in-place)
    text.erase(
        std::remove_if(text.begin(), text.end(),
            [](unsigned char c) {
                return c == '\0' || (c < 32 && c != ' ');
            }
        ),
        text.end()
    );


    // process through python daemon
    bool success = process_with_daemon(text, temp_lex);
    
    if (success && !temp_lex.empty()) {
        // merge temp lexicon with main lexicon
        lexicon.merge(temp_lex);
        
        // update temp_lex with actual word ids from main lex
        lexicon.update_ids(temp_lex);
    }
    
    return success;
}


bool TextProcessor::lemmatize_libstemmer(const std::string& text,
                            std::unordered_map<std::string, WordData>& temp_lex)
{
    temp_lex.clear();
    
    if (text.empty()) {
        return true;
    }
    
    // Use LibStemmer to process text
    std::unordered_map<std::string, uint32_t> term_frequencies;
    stemmer.process_text(text, term_frequencies);
    
    if (term_frequencies.empty()) {
        return false;
    }
    
    // Convert to WordData format and update lexicon
    for (const auto& [term, freq] : term_frequencies) {
        temp_lex[term] = {0, freq}; // word_id placeholder
    }
    
    // Merge with main lexicon and update IDs
    lexicon.merge(temp_lex);
    lexicon.update_ids(temp_lex);
    
    return true;

}
    


