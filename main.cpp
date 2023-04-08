#include "common.h"


int main() {
    vector<File*> files = openNewProject(R"(D:/Pycharm/Pie-Menus-Expanded)");

    createWindow(files);
}
