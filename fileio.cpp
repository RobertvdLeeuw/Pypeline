#include <any>
#include <filesystem>
#include <map>
#include <regex>

#include <json/json.h>
#include <fstream>

#include "common.h"

vector<string> getFilePaths(const string& folder_path) {
    /* Returns the filepaths of all python files in the given folder. */

    vector<string> files;
    files.reserve(64);  // TODO: Reserve everywhere.

    for (const auto &entry: std::filesystem::directory_iterator(folder_path)) {
        if (endsWith(entry.path().generic_string(), ".py")) {
            files.push_back(entry.path().generic_string());
        }
    }
    return files;
}

string getFileName(const string& file_path) {
    /* Returns filename from filepath. */

    return file_path.substr(file_path.find_last_of("/\\") + 1);
}

Json::Value serializeFile(const File* file) {
    // Create an empty Json::Value object named 'data' with type 'objectValue'
    Json::Value data(Json::objectValue);

    data["name"] = file->file_name;
    data["path"] = file->file_path;
    data["notes"] = file->notes;

    Json::Value to_dos(Json::arrayValue);
    for (const ToDo* to_do : file->to_dos) {
        Json::Value to_do_data(Json::objectValue);

        to_do_data["content"] = to_do->content;
        to_do_data["done"] = to_do->done ? 1 : 0;

        to_dos.append(to_do_data);
    }
    data["todos"] = to_dos;

    return data;
}

void saveJSONToFile(const Json::Value& jsonValue, const string& file_path) {
    std::ofstream output_file_stream(file_path); // Open the output file stream
    if (!output_file_stream.is_open()) { // Check if the file was opened successfully
        throw std::runtime_error("Failed to open JSON file (writing): " + file_path);
    }

    output_file_stream << jsonValue; // Write the JSON value to the output file stream
    output_file_stream.close(); // Close the output file stream
    std::cout << "Saved JSON to file: " << file_path << '\n';
}

void saveDataToJSON(const vector<File*>& files, const string& file_path) {
    Json::Value data(Json::arrayValue);

    for (const File* file : files) {
        std::cout << file->notes << '\n';
        data.append(serializeFile(file));
    }

    // std::cout << data.toStyledString() << '\n';

    saveJSONToFile(data, file_path);
}

File* deserializeFile(const Json::Value& json_data) {
    File* file = new File();

    file->file_name = json_data["name"].asString();
    file->file_path = json_data["path"].asString();

    file->notes = json_data["notes"].asString();

    vector<ToDo*> to_dos;
    for (const Json::Value& to_do_data : json_data["todos"]) {
        ToDo* to_do = new ToDo();

        to_do->content = to_do_data["content"].asString();
        to_do->done = to_do_data["done"].asBool();

        to_dos.push_back(to_do);
    }

    file->to_dos = to_dos;

    return file;
}

vector<File*> loadDataFromJSON(const string& file_path) {
    vector<File*> files;

    std::ifstream input_file_stream(file_path);

    if (!input_file_stream.is_open()) { // Check if the file was opened successfully
        throw std::runtime_error("Failed to open JSON file (reading): " + file_path);
    }

    // Read the file contents into a string
    std::stringstream buffer;
    buffer << input_file_stream.rdbuf();
    std::string json_string = buffer.str();

    // Parse the string as JSON using JsonCPP
    Json::Value data(Json::objectValue);
    Json::CharReaderBuilder builder;
    std::string error_msg;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

    if (!reader->parse(json_string.c_str(), json_string.c_str() + json_string.size(), &data, &error_msg)) {
        throw std::runtime_error("Failed to parse JSON file: " + error_msg);
    }

    // Process

    for(const Json::Value& fileData: data) {  // TODO: Create func for this in core.cpp.
        File* file = deserializeFile(fileData);
        File* imports = getImports(file->file_path);

        file->imports = imports->imports;
        file->imported_by = imports->imported_by;

        files.push_back(file);
    }

    sortImports(files);

    return files;
}