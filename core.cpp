#include <fstream>
#include <regex>
#include <sstream>

#include "common.h"

using std::stringstream, std::ifstream, std::regex;

vector<string> getImportsFromLine(string line) {
    /* Returns a vector containing all imports on a given line.
     * "import time" -> ["time],
     * "import time, sys" -> ["time", "sys"] */

    vector<string> imports;
    imports.reserve(16);

    line = trim(line.erase(0, 6));  // Remove 'import' and trim again.

    string import;
    stringstream ss(line);

    while(getline(ss, import, ',')) {
        if(regex_match(import, regex("\\w+ as \\w+"))) {
            import = regex_replace(import, regex(" as \\w+"), "");
        }

        import = trim(import);
        imports.push_back(import);
    }

    return imports;
}

File* getImports(const string& file_path) {  // TODO (V2): Rewrite to take in File and change said File. Add different function for filepath -> file.
    /* Returns a File object containing all imports in a given file.
     * "import time \n import sys" -> {"time": ["file"], "sys": ["file"]},
     * "from time import sleep, perf_counter \n import sys" -> {"time: ["sleep", "perf_counter"], "sys": ["file"]},
     * "from time import *" -> {"time": ["*"]} */

    File* script = new File();
    script->file_path = file_path;

    ifstream in_stream;
    in_stream.open(file_path);

    string str;
    string prev_import_line;
    while(getline(in_stream, str)) {
        string import_line = trim(str);

        /* Multi-import_line import merging */ {
            if (!prev_import_line.empty()) {  // "import a, \ \n b, c" -> "import a, b, c"
                import_line = prev_import_line + import_line;
                prev_import_line.clear();
            }

            if (endsWith(import_line, "\\")) {  // "import a, b, \\ \n c"
                prev_import_line = import_line.substr(0, import_line.find_last_of('\\'));  // Merging with next import_line to get full command
                continue;
            }
        }

        if(startsWith(import_line, "import ")) { // Full import
            for(const string& fileImport: getImportsFromLine(import_line)) {
                Import* import = new Import();
                import->file_name = fileImport;
                import->entire_file = true;

                script->imports.push_back(import);
            }
        } else if (startsWith(import_line, "from ")) {  // Partial import
            string file = trim(import_line.erase(0, 4));   // Remove 'from' from import_line and trim again
            file = file.substr(0, file.find_first_of(" \t"));

            Import* import = new Import();
            import->file_name = file;
            import->entire_file = false;

            import_line = trim(import_line);
            import_line = import_line.substr(import_line.find_first_of(' ') + 1);
            for(const string& imported_object: getImportsFromLine(import_line)) {
                import->imported_content.push_back(imported_object);
            }

            script->imports.push_back(import);
        } else {
            // TODO: File issue warning.
        }
    }

    return script;
}

void sortImports(vector<File*>& files) {
    for (File* file: files) {
        for (Import* import: file->imports) {
            // Find the corresponding file based on file_name
            auto matching_file_iterator = std::find_if(files.begin(), files.end(),
                                              [&import](const File* f) { return f->file_name == import->file_name; });
            File* matching_file = (matching_file_iterator != files.end()) ? *matching_file_iterator : nullptr;

            if (matching_file != nullptr) {
                // Assign the corresponding file to the import
                import->file = matching_file;
                matching_file->imported_by.push_back(file);
            }
        }
    }
}

void addGeneral(vector<File*>& files) {
    File* placeholder_general = new File();
    placeholder_general->file_name = "General";

    files.insert(files.begin(), placeholder_general);
}

bool isHeadFile(const File& file) {
    return file.imported_by.empty() && file.file_name != "General";
}

/* vector<File&> getHeadFiles(const vector<File>& files) {
    vector<File&> head_files;

    std::copy_if(files.begin(), files.end(), std::back_inserter(head_files), [](const File& file) {
        return file.imported_by.empty() && file.file_name != "General";
    });

    if (head_files.empty()) {
        std::cout << "No files with no imports found." << '\n';
    }
    else {
        std::cout << "Files with no imports:" << '\n';
        for (const File& file : head_files) {
            std::cout << '\t' << file.file_name << '\n';
        }
    }

    return head_files;
} */

vector<File*> openNewProject(const string& project_path) {
    vector<string> file_paths = getFilePaths(project_path);
    vector<File*> files;

    for(string file_path: file_paths) {
        string moduleName = getFileName(file_path);
        moduleName = moduleName.substr(0, moduleName.find_last_of('.'));

        File* file = getImports(file_path);
        file->file_name = moduleName;

        files.push_back(file);
    }

    sortImports(files);
    addGeneral(files);

    return files;
}