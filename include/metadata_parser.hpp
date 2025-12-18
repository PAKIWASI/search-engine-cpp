#pragma once

#include "common_includes.hpp"
#include <string>
#include <vector>


class MetadataParser 
{
private:
    const std::string data_path;

    static void parse_csv_line(const std::string& line, 
                               std::vector<std::string>& parsed_line);

    std::string find_fulltext_pdf(std::string& sha);
    std::string find_fulltext_xml(std::string& pmcid);

    static void extract_body_text(const std::string& file_path, 
                                  std::string& body_text);

    // for creating sample
    static void copy_file(const std::string& in_path);

public:
    explicit MetadataParser(const std::string& data_path) 
        : data_path(data_path) {}

    // get paper stats + testing (retarded af design ik)
    int metadata_stats();

    // main file for parsing
    int metadata_parse();


// multithreaded version
    int metadata_parse_multithreaded(u32 num_threads = 8);
    
    // Static versions for thread worker access
    static std::string find_fulltext_pdf_static(std::string& sha, const std::string& data_path);
    static std::string find_fulltext_xml_static(std::string& pmcid, const std::string& data_path);
    static void extract_body_text_static(const std::string& file_path, std::string& body_text);
};


