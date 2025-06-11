#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <regex>

std::map<std::string, std::string> readScripts(const std::string& filename) {
    std::map<std::string, std::string> scripts;
    std::ifstream file(filename);
    std::string line;
    
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return scripts;
    }
    
    std::regex scriptRegex("\"([^\"]+)\":\\s*\"([^\"]+)\"");
    std::smatch match;
    
    while (std::getline(file, line)) {
        if (std::regex_search(line, match, scriptRegex)) {
            std::string name = match[1].str();
            std::string path = match[2].str();
            scripts[name] = path;
        }
    }
    
    file.close();
    return scripts;
}

int main() {
    auto scripts = readScripts("AppSettings.conf");
    
    for (const auto& script : scripts) {
        std::cout << "Name: " << script.first << ", Path: " << script.second << std::endl;
    }
    
    return 0;
}