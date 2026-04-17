#include <windows.h>
#include <stdio.h>
#include "ItemRecord.h"
#include "SpecRecord.h"
#include "Parsers.h"
#include <locale.h>


using namespace std;

int main()
{
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





