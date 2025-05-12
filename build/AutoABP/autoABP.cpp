#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <regex>
#include <chrono>

// Function to remove leading and trailing whitespace from a string
std::string trim(const std::string &str)
{
    const auto start = str.find_first_not_of(" \t");
    if (start == std::string::npos)
    {
        return "";
    }
    const auto end = str.find_last_not_of(" \t");
    return str.substr(start, end - start + 1);
}

// Function to split a string by commas and return a vector of strings
std::vector<std::string> split(const std::string &str, char delimiter)
{
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, delimiter))
    {
        result.push_back(trim(item));
    }
    return result;
}

// Function to read the config.config and parse into configMap
bool read_config(std::unordered_map<std::string, std::string> &configMap)
{
    std::ifstream file("config.config"); // Open the file
    if (!file)
    {
        std::cerr << "Error opening file!" << std::endl;
        return false;
    }

    std::string line;

    // Read the file line by line
    while (std::getline(file, line))
    {
        // Remove leading and trailing whitespace
        line = trim(line);

        if (line.empty() || line[0] == '#')
        { // Skip comments and empty lines
            continue;
        }

        // Find the position of the colon (':') to separate the key from the value
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos)
        {
            std::cerr << "Invalid format in line: " << line << std::endl;
            continue;
        }

        std::string key = trim(line.substr(0, colonPos));
        std::string value = trim(line.substr(colonPos + 1));

        // Save the value based on the key
        configMap[key] = value;
    }

    file.close();

    // Display the saved configuration values
    std::cout << "Parsed Configuration:" << std::endl;
    for (const auto &pair : configMap)
    {
        std::cout << pair.first << ": " << pair.second;
        std::cout << std::endl;
    }

    return true;
}

std::string getFileName(const std::string &path)
{
    // Find the position of the last '/'
    size_t pos = path.find_last_of('/');

    // Extract the substring after the last '/'
    if (pos != std::string::npos)
    {
        return path.substr(pos + 1); // Return the substring after the last '/'
    }
    else
    {
        return path; // Return the whole path if '/' is not found
    }
}

// Function to find all files with a specific extension in a directory
std::vector<std::filesystem::path> findFilesWithExtension(const std::filesystem::path &dirPath, const std::string &extension)
{
    std::vector<std::filesystem::path> filePaths;
    std::regex extensionRegex(".*\\." + extension + "$");

    for (const auto &entry : std::filesystem::directory_iterator(dirPath))
    {
        if (std::filesystem::is_regular_file(entry.path()) && std::regex_match(entry.path().filename().string(), extensionRegex))
        {
            filePaths.push_back(getFileName(entry.path()));
        }
    }
    return filePaths;
}

std::string remove_suffix(const std::string &filename, const std::string &suffix)
{
    if (filename.size() >= suffix.size() &&
        filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0)
    {
        return filename.substr(0, filename.size() - suffix.size());
    }
    return filename;
}

bool get_benchmarks(std::unordered_map<std::string, std::string> &configMap, std::vector<std::filesystem::path> &benchmarks)
{
    std::string fileExtension = "mtx.rnd";
    benchmarks = findFilesWithExtension(configMap["benchmark_dir"], fileExtension);

    for (int i = 0; i < (int)benchmarks.size(); i++)
    {

        benchmarks[i] = remove_suffix(benchmarks[i], fileExtension);
    }

    return true;
}

int main()
{
    std::unordered_map<std::string, std::string> configMap;

    if (!read_config(configMap))
    {
        std::cout << "Cannot read the config file.";
        return -1;
    }

    std::vector<std::filesystem::path> benchmarks = {};

    if (!get_benchmarks(configMap, benchmarks))
    {
        std::cout << "Error getting benchmarks.";
        return -1;
    }

    std::cout << "Start running benchmarks\n";

    for (std::string benchmark : benchmarks)
    {
        auto now = std::chrono::system_clock::now();

        // Convert to time_t to get a C-style time
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);

        // Convert to local time and format
        std::tm *local_tm = std::localtime(&now_c);
        std::ostringstream oss;
        oss << std::put_time(local_tm, "%Y-%m-%d_%H:%M:%S");

        std::string abw_enc_command = configMap["abw_enc_dir"] + " " + configMap["benchmark_dir"] +
                                      "/" + benchmark + "mtx.rnd" + " " + configMap["abw_enc_config"] +
                                      " " + configMap["additional_abw_enc_config"];

        std::string command = abw_enc_command;

        std::cout << "Run command:" << std::endl;
        std::cout << command << std::endl;

        int result = system(command.c_str());

        if (result == -1)
        {
            std::cerr << "Error executing command: " << benchmark << std::endl;
        }
        else
        {
            std::cout << "Command executed successfully: " << benchmark << std::endl;
        }
    }

    return 0;
}
