#include <windows.h>
#include <stdio.h>
#include "ItemRecord.h"
#include "SpecRecord.h"
#include "Parsers.h"
#include <locale.h>

#include <direct.h>


using namespace std;

int main()
{
    char cwd[1024];
    _getcwd(cwd, sizeof(cwd));
    cout << "Current working directory: " << cwd << endl;


    setlocale(LC_ALL, "Russian");
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    DataBase data_base;

    string input;
    while (true) {
        cout << "PS> ";
        getline(cin, input);
        if (cin.eof()) break;
        if (input.empty()) continue;

        inputParser(input, data_base);
    }

    return 0;
}
