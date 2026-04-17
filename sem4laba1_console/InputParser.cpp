#include "Parsers.h"
#include <stdint.h>


void inputParser(string input, DataBase& data_base) 
{
    char delim = ' ';
    size_t end = input.find(delim);

    string command = input.substr(0, end);
    string args = input.substr(end + 1);

    if (command == "Create") {
        string filename;
        uint16_t max_name_len;
        string spec_file_name;
        try {
            createParser(args, filename, max_name_len, spec_file_name);
            create(data_base, filename, max_name_len, spec_file_name);
        }
        catch (const exception& e) {
            cout << e.what() << endl;
        }
    }
    else if (command == "Open") {
        string filename;
        try {
            openParser(args, filename);
            open(data_base, filename);
        }
        catch (const exception& e) {
            cout << e.what() << endl;
        }
    }
    else if (command == "Delete") {
        size_t space2 = args.find(' ');
        if (space2 == string::npos) {
            cout << "Неверный формат Delete" << endl;
            return;
        }
        string sub = args.substr(0, space2);
        string rest = args.substr(space2 + 1);
        if (sub == "component") {
            try {
                cmdDeleteComponent(data_base, rest);
            }
            catch (const exception& e) {
                cout << e.what() << endl;
            }
        }
        else if (sub == "spec") {
            size_t space3 = rest.find(' ');
            if (space3 == string::npos) {
                cout << "Неверный формат Delete spec" << endl;
                return;
            }
            string parent = rest.substr(0, space3);
            string child = rest.substr(space3 + 1);
            try {
                cmdDeleteSpec(data_base, parent, child);
            }
            catch (const exception& e) {
                cout << e.what() << endl;
            }
        }
        else {
            cout << "Неверная подкоманда Delete" << endl;
        }
    }
    else if (command == "Truncate") {
        if (args.empty()) {
            try {
                cmdTruncate(data_base);
            }
            catch (const exception& e) {
                cout << e.what() << endl;
            }
        }
        else {
            cout << "Команда Truncate не принимает аргументов." << endl;
        }
    }
    else if (command == "Input") {
        size_t space2 = args.find(' ');
        if (space2 == string::npos) {
            cout << "Неверный формат Input" << endl;
            return;
        }
        string sub = args.substr(0, space2);
        string rest = args.substr(space2 + 1);

        if (sub == "component") {
            size_t space3 = rest.find(' ');
            if (space3 == string::npos) {
                cout << "Неверный формат Input component. Ожидается: component <имя> <тип>" << endl;
                return;
            }
            string name = rest.substr(0, space3);
            string type = rest.substr(space3 + 1);
            try {
                cmdInputComponent(data_base, name, type);
            }
            catch (const exception& e) {
                cout << e.what() << endl;
            }
        }
        else if (sub == "spec") {
            size_t space3 = rest.find(' ');
            if (space3 == string::npos) {
                cout << "Неверный формат Input spec. Ожидается: spec <родитель> <ребёнок> [количество]" << endl;
                return;
            }
            string parent = rest.substr(0, space3);
            string rest2 = rest.substr(space3 + 1);
            size_t space4 = rest2.find(' ');
            uint16_t qty = 1;
            string child;
            if (space4 == string::npos) {
                child = rest2;
            }
            else {
                child = rest2.substr(0, space4);
                string qtyStr = rest2.substr(space4 + 1);
                if (!tryParseUint16(qtyStr, qty)) {
                    cout << "Неверное количество" << endl;
                    return;
                }
            }
            try {
                cmdInputSpec(data_base, parent, child, qty);
            }
            catch (const exception& e) {
                cout << e.what() << endl;
            }
        }
        else {
            cout << "Неизвестная подкоманда Input. Используйте component или spec." << endl;
        }
    }
    else if (command == "Restore") {
        if (args.empty()) {
            cout << "Неверный формат Restore. Ожидается: Restore <имя> | Restore *" << endl;
            return;
        }
        try {
            cmdRestore(data_base, args);
        }
        catch (const exception& e) {
            cout << e.what() << endl;
        }
    }
    else if (command == "Print") {
        if (args.empty()) {
            cout << "Неверный формат Print. Ожидается: Print <имя> | Print *" << endl;
            return;
        }
        try {
            cmdPrint(data_base, args);
        }
        catch (const exception& e) {
            cout << e.what() << endl;
        }
    }
    else if (command == "Help") {
        cmdHelp();
    }
    else if (command == "Exit") {
        cmdExit(data_base);
    }
    else {
        cout << "Неизвестная команда. Введите Help для списка." << endl;
    }
}


// парсер для аргументов команды Create
void createParser(const string& args, string& filename, uint16_t& max_name_len, string& spec_file_name) {
	size_t open_br = args.find('(');
	size_t close_br = args.find(')');
	if (open_br == string::npos || close_br == string::npos || close_br != args.length() - 1) {
		throw runtime_error("Неверный формат: отсутствуют или неверно расположены скобки");
	}

	filename = args.substr(0, open_br);
	// Удаление пробелов в конце имени
	size_t last = filename.find_last_not_of(" \t");
	if (last != string::npos)
		filename.erase(last + 1);
	else
		filename.clear();

	if (filename.empty())
		throw runtime_error("Имя файла не может быть пустым");

	string inside = args.substr(open_br + 1, close_br - open_br - 1);
	// Удаление начальных и конечных пробелов
	size_t start = inside.find_first_not_of(" \t");
	if (start != string::npos)
		inside.erase(0, start);
	else
		inside.clear();

	last = inside.find_last_not_of(" \t");
	if (last != string::npos)
		inside.erase(last + 1);
	else
		inside.clear();

	size_t comma = inside.find(',');
	if (comma == string::npos) {
		if (!tryParseUint16(inside, max_name_len))
			throw runtime_error("Некорректная длина записи данных");
		spec_file_name.clear();
	}
	else {
		string len_str = inside.substr(0, comma);
		string spec_str = inside.substr(comma + 1);

		// Обрезка пробелов
		start = len_str.find_first_not_of(" \t");
		if (start != string::npos)
			len_str.erase(0, start);
		else
			len_str.clear();

		last = len_str.find_last_not_of(" \t");
		if (last != string::npos)
			len_str.erase(last + 1);
		else
			len_str.clear();

		start = spec_str.find_first_not_of(" \t");
		if (start != string::npos)
			spec_str.erase(0, start);
		else
			spec_str.clear();

		last = spec_str.find_last_not_of(" \t");
		if (last != string::npos)
			spec_str.erase(last + 1);
		else
			spec_str.clear();

		if (!tryParseUint16(len_str, max_name_len))
			throw runtime_error("Некорректная длина записи данных");
		if (spec_str.empty())
			throw runtime_error("Имя файла спецификаций не может быть пустым");
		spec_file_name = spec_str;
	}

	if (!onlyFilenameSymbols(filename) || (!spec_file_name.empty() && !onlyFilenameSymbols(spec_file_name)))
		throw runtime_error("Некорректные символы в имени файла");
}


// преобразование строки в число uint16_t
bool tryParseUint16(const string s, uint16_t& result)
{
	if (s.empty()) { return false; }

	uint32_t temp = 0;	// больший размер для обнаружения переполнения
	for (char c : s) {
		if (c < '0' || c > '9') { return false; }
		temp = temp * 10 + (c - '0');					// сдвиг разряда (формирование числа)
		if (temp > numeric_limits<uint16_t>::max()) {	// проверка на переполнение
			return false;
		}
	}
	result = static_cast<uint16_t>(temp);
	return true;
}


// проверка на корректность символов в имени файлов
bool onlyFilenameSymbols(const string s) {
	if (s.empty()) return false;
	const char symbols[] = "abcdefghijklmnopqrstuvwxyz._-0123456789";
	for (char c : s) {
		bool found = false;
		for (int j = 0; symbols[j]; ++j) {
			if (c == symbols[j]) { found = true; break; }
		}
		if (!found) return false;
	}
	return true;
}


void openParser(const string args, string& filename) {
    string trimmed = args;
    size_t start = trimmed.find_first_not_of(" \t");
    if (start != string::npos) trimmed = trimmed.substr(start);
    size_t end = trimmed.find_last_not_of(" \t");
    if (end != string::npos) trimmed = trimmed.substr(0, end + 1);
    else trimmed.clear();
    if (trimmed.empty()) throw runtime_error("Не указано имя файла");
    if (!onlyFilenameSymbols(trimmed)) {
        throw runtime_error("Ошибка, некорректное имя файла");
    }
    filename = trimmed;
}