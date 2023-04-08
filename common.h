#pragma once

#include <string>

#include <iostream>
#include <cstdio>

#include <vector>
#include <unordered_map>

using std::string, std::vector;

/*strman.cpp*/
bool endsWith(string const & value, string const & ending);
bool startsWith(string const & value, string const & ending);
string getProjectPath(const string& file_path);

string trim(const string& input);

/*core.cpp*/
class ToDo {
    public:
        bool done;  // TODO: Enum (to do, doing (priority levels?) (levels (breakdown (writing, testing, etc)?)), done).
        string content;
        // TODO: Text color?
};

class Import;  // TODO: Clean this.

class File {  // TODO: Change to structs?
    public:
        string file_name;  // TODO: Set method.
        string file_path;
        bool non_project;  // TODO: Implement.

        string notes;
        vector<ToDo*> to_dos;

        vector<Import*> imports;
        vector<File*> imported_by;
};

class Import {
    public:
        string file_name;
        File* file;
        bool entire_file;
        vector<string> imported_content;  // TODO: Optional
};

File* getImports(const string& file_path);
vector<string> getImportsFromLine(string line);

void sortImports(std::vector<File*>& files);
bool isHeadFile(const File &file);

vector<File*> openNewProject(const string& project_path);

/*io.cpp*/
vector<string> getFilePaths(const string& folder_path);
string getFileName(const string& file_path);

void saveDataToJSON(const vector<File*>& files, const std::string& file_path);
vector<File*> loadDataFromJSON(const string& file_path);

/*gui.cpp*/
void createWindow(vector<File*>& original_files);
