#pragma once

#include <string>
#include <vector>


class MetadataParser 
{
private:
    const std::string data_path;


    static void parse_csv_line(const std::string& line, std::vector<std::string>& parsed_line);

    std::string find_fulltext_pdf(std::string& sha);
    std::string find_fulltext_xml(std::string& pmcid);

    static void extract_body_text(const std::string& file_path, std::string& body_text);
public:
    explicit MetadataParser(const std::string& data_path) : data_path(data_path) {}


    // get paper stats + testing (retarded af design)
    int metadata_stats();

    // main file for parsing
    int metadata_parse();
};


