#define _CRT_SECURE_NO_WARNINGS
#include "FileUtils.h"



// создание файла
void create(DataBase& data_base, string filename, uint16_t max_name_len, string spec_file_name) {
    closeFiles(data_base.items_file, data_base.spec_file);

    string items_path = filename + ".prd";
    string spec_path;
    if (spec_file_name.empty()) {
        spec_path = filename + ".prs";
    }
    else {
        spec_path = spec_file_name;
        if (spec_path.find('.') == string::npos)
            spec_path += ".prs";
    }


    // Проверка существования файла
    FILE* test = fopen(items_path.c_str(), "rb");
    if (test) {
        ItemsHeader test_header;
        if (fread(&test_header, sizeof(ItemsHeader), 1, test) == 1) {
            if (test_header.signature[0] == 'P' && test_header.signature[1] == 'S') {
                fclose(test);
                if (!overwritingFile()) return;
            }
            else {
                fclose(test);
                throw runtime_error("Существующий файл имеет неверную сигнатуру");
            }
        }
        else {
            fclose(test);
            throw runtime_error("Ошибка чтения сигнатуры");
        }
    }


    // Создание файла изделий
    data_base.items_file = fopen(items_path.c_str(), "wb+");
    if (!data_base.items_file)
        throw runtime_error("Ошибка создания файла компонентов");

    ItemsHeader h;
    h.signature[0] = 'P';
    h.signature[1] = 'S';
    h.data_lenght = max_name_len;
    h.pointer_to_first_record = -1;
    h.pointer_to_free_memory = sizeof(ItemsHeader);
    memset(h.spec_file_name, ' ', 16);
    size_t len = spec_path.size();
    if (len > 16) len = 16;
    memcpy(h.spec_file_name, spec_path.c_str(), len);

    if (fwrite(&h, sizeof(ItemsHeader), 1, data_base.items_file) != 1) {
        fclose(data_base.items_file);
        data_base.items_file = nullptr;
        throw runtime_error("Ошибка записи заголовка");
    }
    fflush(data_base.items_file);
    data_base.items_header = h;

    // Создание файла спецификаций
    data_base.spec_file = fopen(spec_path.c_str(), "wb+");
    if (!data_base.spec_file) {
        fclose(data_base.items_file);
        data_base.items_file = nullptr;
        throw runtime_error("Ошибка создания файла спецификаций");
    }

    SpecHeader sh;
    sh.pointer_to_first_record = -1;
    sh.pointer_to_free_memory = sizeof(SpecHeader);

    // Запись заголовка через буфер (для надёжности)
    uint8_t buffer[8];
    memcpy(buffer, &sh, 8);
    if (fwrite(buffer, 1, 8, data_base.spec_file) != 8) {
        fclose(data_base.spec_file);
        fclose(data_base.items_file);
        data_base.items_file = data_base.spec_file = nullptr;
        throw runtime_error("Ошибка записи заголовка спецификаций");
    }
    fflush(data_base.spec_file);
    data_base.spec_header = sh;

    data_base.items_filename = items_path;
    data_base.spec_filename = spec_path;
}



// разрешение на перезапись файла
bool overwritingFile() {
    cout << "Файл с таким именем существует. Перезаписать его? (y / n): ";
    char answer;
    cin >> answer;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    return (answer == 'y' || answer == 'Y');
}


// Закрытие файлов
void closeFiles(FILE*& items_file, FILE*& spec_file) {
    if (items_file) {
        fclose(items_file);
        items_file = nullptr;
    }
    if (spec_file) {
        fclose(spec_file);
        spec_file = nullptr;
    }
}


// открытие файла
void open(DataBase& data_base, string filename) {
    closeFiles(data_base.items_file, data_base.spec_file);

    string items_path = filename + ".prd";
    string spec_path = filename + ".prs";

    data_base.items_file = fopen(items_path.c_str(), "rb+");
    if (!data_base.items_file)
        throw runtime_error("Ошибка открытия файла компонентов");

    // Чтение заголовка
    ItemsHeader h;
    if (fread(&h, sizeof(ItemsHeader), 1, data_base.items_file) != 1) {
        closeFiles(data_base.items_file, data_base.spec_file);
        throw runtime_error("Ошибка чтения заголовка");
    }
    if (h.signature[0] != 'P' || h.signature[1] != 'S') {
        closeFiles(data_base.items_file, data_base.spec_file);
        throw runtime_error("Неверная сигнатура");
    }
    data_base.items_header = h;

    data_base.spec_file = fopen(spec_path.c_str(), "rb+");
    if (!data_base.spec_file) {
        closeFiles(data_base.items_file, data_base.spec_file);
        throw runtime_error("Ошибка открытия файла спецификаций");
    }

    // Чтение заголовка спецификаций через буфер
    SpecHeader sh;
    uint8_t buffer[8];
    if (fread(buffer, 1, 8, data_base.spec_file) != 8) {
        closeFiles(data_base.items_file, data_base.spec_file);
        throw runtime_error("Ошибка чтения заголовка спецификаций");
    }
    memcpy(&sh, buffer, 8);
    data_base.spec_header = sh;

    data_base.items_filename = items_path;
    data_base.spec_filename = spec_path;
}



// Чтение записи из файла изделий по смещению
ItemRecord readItemRecord(DataBase& db, int32_t offset) {
    if (!db.items_file) throw runtime_error("Файл не открыт");
    fseek(db.items_file, offset, SEEK_SET);

    ItemRecord rec;
    if (fread(&rec.deleted, 1, 1, db.items_file) != 1 ||
        fread(&rec.pointer_to_spec_record, 4, 1, db.items_file) != 1 ||
        fread(&rec.pointer_to_next_record, 4, 1, db.items_file) != 1) {
        throw runtime_error("Ошибка чтения полей записи");
    }

    // Чтение данных фиксированной длины
    char* buf = new char[db.items_header.data_lenght];
    if (fread(buf, db.items_header.data_lenght, 1, db.items_file) != 1) {
        delete[] buf;
        throw runtime_error("Ошибка чтения данных записи");
    }
    // Убираем пробелы справа
    string s(buf, db.items_header.data_lenght);
    size_t end = s.find_last_not_of(' ');
    if (end != string::npos) s = s.substr(0, end + 1);
    else s.clear();
    rec.data = s;
    delete[] buf;
    return rec;
}


// Запись записи в файл изделий по смещению
void writeItemRecord(DataBase& db, int32_t offset, const ItemRecord& rec) {
    if (!db.items_file) throw runtime_error("Файл не открыт");
    fseek(db.items_file, offset, SEEK_SET);

    if (fwrite(&rec.deleted, 1, 1, db.items_file) != 1 ||
        fwrite(&rec.pointer_to_spec_record, 4, 1, db.items_file) != 1 ||
        fwrite(&rec.pointer_to_next_record, 4, 1, db.items_file) != 1) {
        throw runtime_error("Ошибка записи полей записи");
    }

    // Подготовка данных: дополнение пробелами до фиксированной длины
    string out = rec.data;
    if (out.length() > db.items_header.data_lenght)
        out = out.substr(0, db.items_header.data_lenght);
    else
        out.append(db.items_header.data_lenght - out.length(), ' ');

    if (fwrite(out.c_str(), db.items_header.data_lenght, 1, db.items_file) != 1)
        throw runtime_error("Ошибка записи данных записи");
    fflush(db.items_file);
}


// Добавление новой записи в конец файла изделий
int32_t addItemRecord(DataBase& db, const ItemRecord& rec) {
    int32_t newOffset = db.items_header.pointer_to_free_memory;
    writeItemRecord(db, newOffset, rec);

    // Обновляем свободную область
    int32_t recordSize = 1 + 4 + 4 + db.items_header.data_lenght;
    db.items_header.pointer_to_free_memory += recordSize;
    // Записываем обновлённый заголовок в файл
    fseek(db.items_file, 0, SEEK_SET);
    if (fwrite(&db.items_header, sizeof(ItemsHeader), 1, db.items_file) != 1)
        throw runtime_error("Ошибка обновления заголовка");

    // Если это первая запись, обновляем указатель на первую запись
    if (db.items_header.pointer_to_first_record == -1) {
        db.items_header.pointer_to_first_record = newOffset;
        fseek(db.items_file, 0, SEEK_SET);
        fwrite(&db.items_header, sizeof(ItemsHeader), 1, db.items_file);
    }
    else {
        // Иначе находим последнюю запись и обновляем её next_item_offset
        int32_t current = db.items_header.pointer_to_first_record;
        while (true) {
            ItemRecord curRec = readItemRecord(db, current);
            if (curRec.pointer_to_next_record == -1) {
                curRec.pointer_to_next_record = newOffset;
                writeItemRecord(db, current, curRec);
                break;
            }
            current = curRec.pointer_to_next_record;
        }
    }
    return newOffset;
}


// Чтение записи спецификации
SpecRecord readSpecRecord(DataBase& db, int32_t offset) {
    if (!db.spec_file) throw runtime_error("Файл спецификаций не открыт");
    fseek(db.spec_file, offset, SEEK_SET);
    SpecRecord rec;
    if (fread(&rec.deleted, 1, 1, db.spec_file) != 1 ||
        fread(&rec.pointer_to_item_record, 4, 1, db.spec_file) != 1 ||
        fread(&rec.quantity, 2, 1, db.spec_file) != 1 ||
        fread(&rec.pointer_to_next_record, 4, 1, db.spec_file) != 1) {
        throw runtime_error("Ошибка чтения записи спецификации");
    }
    return rec;
}


// Запись записи спецификации
void writeSpecRecord(DataBase& db, int32_t offset, const SpecRecord& rec) {
    if (!db.spec_file) throw runtime_error("Файл спецификаций не открыт");
    fseek(db.spec_file, offset, SEEK_SET);
    if (fwrite(&rec.deleted, 1, 1, db.spec_file) != 1 ||
        fwrite(&rec.pointer_to_item_record, 4, 1, db.spec_file) != 1 ||
        fwrite(&rec.quantity, 2, 1, db.spec_file) != 1 ||
        fwrite(&rec.pointer_to_next_record, 4, 1, db.spec_file) != 1) {
        throw runtime_error("Ошибка записи спецификации");
    }
    fflush(db.spec_file);
}


// Добавление новой записи спецификации в конец файла
int32_t addSpecRecord(DataBase& db, const SpecRecord& rec) {
    int32_t newOffset = db.spec_header.pointer_to_free_memory;
    writeSpecRecord(db, newOffset, rec);
    db.spec_header.pointer_to_free_memory += 11;
    if (db.spec_header.pointer_to_first_record == -1)
        db.spec_header.pointer_to_first_record = newOffset;
    fseek(db.spec_file, 0, SEEK_SET);
    fwrite(&db.spec_header, sizeof(SpecHeader), 1, db.spec_file);
    return newOffset;
}


// Поиск компонента по имени (возвращает смещение или -1)
int32_t findItemByName(DataBase& db, const string& name) {
    int32_t offset = db.items_header.pointer_to_first_record;
    while (offset != -1) {
        ItemRecord rec = readItemRecord(db, offset);
        if (rec.deleted == 0 && rec.data == name)
            return offset;
        offset = rec.pointer_to_next_record;
    }
    return -1;
}



// Input component: добавление нового компонента
void cmdInputComponent(DataBase& db, const string& name, const string& type) {
    if (!db.items_file) throw runtime_error("Файл не открыт");
    if (name.empty()) throw runtime_error("Имя не может быть пустым");
    if (findItemByName(db, name) != -1)
        throw runtime_error("Компонент с таким именем уже существует");

    ItemRecord rec;
    rec.deleted = 0;
    rec.pointer_to_spec_record = -1;
    rec.pointer_to_next_record = -1;
    rec.data = name;
    addItemRecord(db, rec);
    cout << "Компонент добавлен." << endl;
}


// Input spec: добавление комплектующего в спецификацию
void cmdInputSpec(DataBase& db, const string& parentName, const string& childName, uint16_t quantity) {
    if (!db.items_file || !db.spec_file) throw runtime_error("Файлы не открыты");

    int32_t parentOffset = findItemByName(db, parentName);
    if (parentOffset == -1) throw runtime_error("Родительский компонент не найден");

    int32_t childOffset = findItemByName(db, childName);
    if (childOffset == -1) throw runtime_error("Дочерний компонент не найден");

    ItemRecord parent = readItemRecord(db, parentOffset);

    // Создаём запись спецификации
    SpecRecord spec;
    spec.deleted = 0;
    spec.pointer_to_item_record = childOffset;
    spec.quantity = quantity;
    spec.pointer_to_next_record = -1;

    int32_t newSpecOffset = addSpecRecord(db, spec);

    // Привязываем к родителю
    if (parent.pointer_to_spec_record == -1) {
        // Первая спецификация
        parent.pointer_to_spec_record = newSpecOffset;
        writeItemRecord(db, parentOffset, parent);
    }
    else {
        // Ищем последнюю запись в цепочке
        int32_t cur = parent.pointer_to_spec_record;
        while (true) {
            SpecRecord curSpec = readSpecRecord(db, cur);
            if (curSpec.pointer_to_next_record == -1) {
                curSpec.pointer_to_next_record = newSpecOffset;
                writeSpecRecord(db, cur, curSpec);
                break;
            }
            cur = curSpec.pointer_to_next_record;
        }
    }
    cout << "Комплектующее добавлено в спецификацию." << endl;
}


// Проверяет, есть ли активные ссылки на компонент в спецификациях
bool hasReferences(DataBase& db, int32_t itemOffset) {
    if (db.spec_header.pointer_to_first_record == -1) return false;
    int32_t cur = db.spec_header.pointer_to_first_record;
    while (cur != -1) {
        SpecRecord rec = readSpecRecord(db, cur);
        if (rec.deleted == 0 && rec.pointer_to_item_record == itemOffset)
            return true;
        cur = rec.pointer_to_next_record;
    }
    return false;
}

// Помечает все записи в цепочке спецификаций как удалённые
void markSpecChainDeleted(DataBase& db, int32_t firstSpecOffset) {
    int32_t cur = firstSpecOffset;
    while (cur != -1) {
        SpecRecord rec = readSpecRecord(db, cur);
        if (rec.deleted == 0) {
            rec.deleted = -1;
            writeSpecRecord(db, cur, rec);
        }
        cur = rec.pointer_to_next_record;
    }
}



void cmdDeleteComponent(DataBase& db, const string& name) {
    if (!db.items_file) throw runtime_error("Файл не открыт");
    int32_t offset = findItemByName(db, name);
    if (offset == -1) throw runtime_error("Компонент не найден");

    // Проверяем, есть ли ссылки на этот компонент
    if (hasReferences(db, offset))
        throw runtime_error("Нельзя удалить компонент: на него есть ссылки в спецификациях");

    ItemRecord rec = readItemRecord(db, offset);
    if (rec.deleted != 0) throw runtime_error("Компонент уже удалён");

    // Помечаем сам компонент удалённым
    rec.deleted = -1;
    writeItemRecord(db, offset, rec);

    // Помечаем все его спецификации удалёнными
    if (rec.pointer_to_spec_record != -1)
        markSpecChainDeleted(db, rec.pointer_to_spec_record);

    cout << "Компонент помечен на удаление." << endl;
}



void cmdDeleteSpec(DataBase& db, const string& parentName, const string& childName) {
    if (!db.items_file || !db.spec_file) throw runtime_error("Файлы не открыты");

    int32_t parentOffset = findItemByName(db, parentName);
    if (parentOffset == -1) throw runtime_error("Родительский компонент не найден");

    int32_t childOffset = findItemByName(db, childName);
    if (childOffset == -1) throw runtime_error("Дочерний компонент не найден");

    ItemRecord parent = readItemRecord(db, parentOffset);
    if (parent.pointer_to_spec_record == -1)
        throw runtime_error("У родителя нет спецификации");

    // Ищем в цепочке спецификаций родителя запись, ссылающуюся на child
    int32_t cur = parent.pointer_to_spec_record;
    bool found = false;
    while (cur != -1) {
        SpecRecord rec = readSpecRecord(db, cur);
        if (rec.deleted == 0 && rec.pointer_to_item_record == childOffset) {
            // Помечаем удалённой
            rec.deleted = -1;
            writeSpecRecord(db, cur, rec);
            found = true;
            break;
        }
        cur = rec.pointer_to_next_record;
    }

    if (found)
        cout << "Запись удалена из спецификации." << endl;
    else
        throw runtime_error("Такая запись не найдена в спецификации родителя");
}



// Перестраивает список изделий в алфавитном порядке
void rebuildItemsOrder(DataBase& db) {
    if (db.items_header.pointer_to_first_record == -1) return;

    // Собираем все активные записи
    vector<pair<string, int32_t>> entries; // имя, смещение
    int32_t cur = db.items_header.pointer_to_first_record;
    while (cur != -1) {
        ItemRecord rec = readItemRecord(db, cur);
        if (rec.deleted == 0) {
            entries.push_back({ rec.data, cur });
        }
        cur = rec.pointer_to_next_record;
    }

    if (entries.empty()) {
        db.items_header.pointer_to_first_record = -1;
        fseek(db.items_file, 0, SEEK_SET);
        fwrite(&db.items_header, sizeof(ItemsHeader), 1, db.items_file);
        return;
    }

    // Сортируем по имени
    sort(entries.begin(), entries.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });

    // Перелинковываем
    int32_t prevOffset = -1;
    for (size_t i = 0; i < entries.size(); ++i) {
        int32_t currentOffset = entries[i].second;
        ItemRecord rec = readItemRecord(db, currentOffset);
        int32_t nextOffset = (i + 1 < entries.size()) ? entries[i + 1].second : -1;
        if (rec.pointer_to_next_record != nextOffset) {
            rec.pointer_to_next_record = nextOffset;
            writeItemRecord(db, currentOffset, rec);
        }
        if (i == 0) {
            db.items_header.pointer_to_first_record = currentOffset;
        }
    }

    // Обновляем заголовок
    fseek(db.items_file, 0, SEEK_SET);
    fwrite(&db.items_header, sizeof(ItemsHeader), 1, db.items_file);
}



void cmdRestore(DataBase& db, const string& name) {
    if (!db.items_file) throw runtime_error("Файл не открыт");

    if (name == "*") {
        // Восстанавливаем все записи в файле изделий
        int32_t cur = db.items_header.pointer_to_first_record;
        while (cur != -1) {
            ItemRecord rec = readItemRecord(db, cur);
            if (rec.deleted != 0) {
                rec.deleted = 0;
                writeItemRecord(db, cur, rec);
            }
            cur = rec.pointer_to_next_record;
        }
        // Восстанавливаем все записи в файле спецификаций
        cur = db.spec_header.pointer_to_first_record;
        while (cur != -1) {
            SpecRecord rec = readSpecRecord(db, cur);
            if (rec.deleted != 0) {
                rec.deleted = 0;
                writeSpecRecord(db, cur, rec);
            }
            cur = rec.pointer_to_next_record;
        }
        // Восстанавливаем алфавитный порядок
        rebuildItemsOrder(db);
        cout << "Все удалённые записи восстановлены." << endl;
    }
    else {
        // Восстанавливаем конкретный компонент
        int32_t offset = findItemByName(db, name);
        if (offset == -1) throw runtime_error("Компонент не найден");

        ItemRecord rec = readItemRecord(db, offset);
        if (rec.deleted == 0) throw runtime_error("Компонент не удалён");

        rec.deleted = 0;
        writeItemRecord(db, offset, rec);

        // Восстанавливаем его спецификации (если они были помечены)
        if (rec.pointer_to_spec_record != -1) {
            int32_t cur = rec.pointer_to_spec_record;
            while (cur != -1) {
                SpecRecord spec = readSpecRecord(db, cur);
                if (spec.deleted != 0) {
                    spec.deleted = 0;
                    writeSpecRecord(db, cur, spec);
                }
                cur = spec.pointer_to_next_record;
            }
        }

        // Перестраиваем порядок
        rebuildItemsOrder(db);
        cout << "Компонент восстановлен." << endl;
    }
}



void printSpecification(DataBase& db, int32_t componentOffset, int32_t specOffset, int level) {
    if (specOffset == -1) return;
    int32_t cur = specOffset;
    while (cur != -1) {
        SpecRecord spec = readSpecRecord(db, cur);
        if (spec.deleted == 0) {
            ItemRecord child = readItemRecord(db, spec.pointer_to_item_record);
            if (child.deleted == 0) {
                for (int i = 0; i < level; ++i) cout << "  ";
                string type = (child.pointer_to_spec_record == -1) ? "Деталь" : "Узел";
                cout << "- " << child.data << " (" << type << ")\n";
                if (child.pointer_to_spec_record != -1) {
                    printSpecification(db, spec.pointer_to_item_record, child.pointer_to_spec_record, level + 1);
                }
            }
        }
        cur = spec.pointer_to_next_record;
    }
}

void cmdPrint(DataBase& db, const string& name) {
    if (!db.items_file) throw runtime_error("Файл не открыт");

    if (name == "*") {
        // Вывести все компоненты
        int32_t cur = db.items_header.pointer_to_first_record;
        while (cur != -1) {
            ItemRecord rec = readItemRecord(db, cur);
            if (rec.deleted == 0) {
                string type = (rec.pointer_to_spec_record == -1) ? "Деталь" : "Узел/Изделие";
                cout << rec.data << " - " << type << endl;
            }
            cur = rec.pointer_to_next_record;
        }
    }
    else {
        int32_t offset = findItemByName(db, name);
        if (offset == -1) throw runtime_error("Компонент не найден");
        ItemRecord rec = readItemRecord(db, offset);
        if (rec.deleted != 0) throw runtime_error("Компонент удалён");

        if (rec.pointer_to_spec_record == -1) {
            cout << "Компонент '" << rec.data << "' - деталь, спецификации нет." << endl;
            return;
        }

        cout << rec.data << ":\n";
        printSpecification(db, offset, rec.pointer_to_spec_record, 1);
    }
}



void cmdHelp() {
    cout << "Список команд:\n"
        << "  Create имя_файла.prd (максимальная_длина_имени[, имя_файла_спецификаций])\n"
        << "  Open имя_файла\n"
        << "  Input component <имя> <тип>\n"
        << "  Input spec <родитель> <ребёнок> [количество]\n"
        << "  Delete component <имя>\n"
        << "  Delete spec <родитель> <ребёнок>\n"
        << "  Restore <имя> | *\n"
        << "  Truncate\n"
        << "  Print <имя> | *\n"
        << "  Help\n"
        << "  Exit\n";
}



void cmdExit(DataBase& db) {
    closeFiles(db.items_file, db.spec_file);
    cout << "Выход." << endl;
    exit(0);
}




void cmdTruncate(DataBase& db) {
    if (!db.items_file || !db.spec_file)
        throw runtime_error("Файлы не открыты");

    vector<ItemRecord> activeItems;
    vector<int32_t> oldItemOffsets;
    int32_t cur = db.items_header.pointer_to_first_record;
    while (cur != -1) {
        ItemRecord rec = readItemRecord(db, cur);
        if (rec.deleted == 0) {
            activeItems.push_back(rec);
            oldItemOffsets.push_back(cur);
        }
        cur = rec.pointer_to_next_record;
    }

    vector<SpecRecord> activeSpecs;
    vector<int32_t> oldSpecOffsets;
    cur = db.spec_header.pointer_to_first_record;
    while (cur != -1) {
        SpecRecord rec = readSpecRecord(db, cur);
        if (rec.deleted == 0) {
            activeSpecs.push_back(rec);
            oldSpecOffsets.push_back(cur);
        }
        cur = rec.pointer_to_next_record;
    }

    fclose(db.items_file);
    fclose(db.spec_file);
    db.items_file = nullptr;
    db.spec_file = nullptr;

    remove(db.items_filename.c_str());
    remove(db.spec_filename.c_str());

    // Создание новых файлов
    db.items_file = fopen(db.items_filename.c_str(), "wb+");
    db.spec_file = fopen(db.spec_filename.c_str(), "wb+");
    if (!db.items_file || !db.spec_file)
        throw runtime_error("Не удалось создать новые файлы");

    ItemsHeader newItemsHeader = db.items_header;
    newItemsHeader.pointer_to_first_record = -1;
    newItemsHeader.pointer_to_free_memory = sizeof(ItemsHeader);
    fseek(db.items_file, 0, SEEK_SET);
    fwrite(&newItemsHeader, sizeof(ItemsHeader), 1, db.items_file);

    SpecHeader newSpecHeader = db.spec_header;
    newSpecHeader.pointer_to_first_record = -1;
    newSpecHeader.pointer_to_free_memory = sizeof(SpecHeader);
    fseek(db.spec_file, 0, SEEK_SET);
    fwrite(&newSpecHeader, sizeof(SpecHeader), 1, db.spec_file);

    // Записываем активные записи изделий
    map<int32_t, int32_t> newItemOffsets;
    int32_t newOffset = sizeof(ItemsHeader);
    for (size_t i = 0; i < activeItems.size(); ++i) {
        newItemOffsets[oldItemOffsets[i]] = newOffset;
        fseek(db.items_file, newOffset, SEEK_SET);
        fwrite(&activeItems[i].deleted, 1, 1, db.items_file);
        fwrite(&activeItems[i].pointer_to_spec_record, 4, 1, db.items_file);
        int32_t dummy = -1;
        fwrite(&dummy, 4, 1, db.items_file);
        string data = activeItems[i].data;
        if (data.length() < db.items_header.data_lenght)
            data.append(db.items_header.data_lenght - data.length(), ' ');
        fwrite(data.c_str(), db.items_header.data_lenght, 1, db.items_file);
        newOffset += 1 + 4 + 4 + db.items_header.data_lenght;
    }
    for (size_t i = 0; i < activeItems.size(); ++i) {
        int32_t curNewOff = newItemOffsets[oldItemOffsets[i]];
        int32_t nextNewOff = (i + 1 < activeItems.size()) ? newItemOffsets[oldItemOffsets[i + 1]] : -1;
        fseek(db.items_file, curNewOff + 5, SEEK_SET);
        fwrite(&nextNewOff, 4, 1, db.items_file);
    }
    // Обновляем заголовок изделий
    newItemsHeader.pointer_to_first_record = activeItems.empty() ? -1 : newItemOffsets[oldItemOffsets[0]];
    newItemsHeader.pointer_to_free_memory = newOffset;
    fseek(db.items_file, 0, SEEK_SET);
    fwrite(&newItemsHeader, sizeof(ItemsHeader), 1, db.items_file);
    db.items_header = newItemsHeader;

    map<int32_t, int32_t> newSpecOffsets;
    newOffset = sizeof(SpecHeader);
    for (size_t i = 0; i < activeSpecs.size(); ++i) {
        newSpecOffsets[oldSpecOffsets[i]] = newOffset;
        fseek(db.spec_file, newOffset, SEEK_SET);
        fwrite(&activeSpecs[i].deleted, 1, 1, db.spec_file);
        int32_t itemOff = activeSpecs[i].pointer_to_item_record;
        if (newItemOffsets.find(itemOff) != newItemOffsets.end())
            itemOff = newItemOffsets[itemOff];
        fwrite(&itemOff, 4, 1, db.spec_file);
        fwrite(&activeSpecs[i].quantity, 2, 1, db.spec_file);
        int32_t dummy = -1;
        fwrite(&dummy, 4, 1, db.spec_file);
        newOffset += 11;
    }
    
    for (size_t i = 0; i < activeSpecs.size(); ++i) {
        int32_t curNewOff = newSpecOffsets[oldSpecOffsets[i]];
        int32_t nextNewOff = (i + 1 < activeSpecs.size()) ? newSpecOffsets[oldSpecOffsets[i + 1]] : -1;
        fseek(db.spec_file, curNewOff + 1 + 4 + 2, SEEK_SET);
        fwrite(&nextNewOff, 4, 1, db.spec_file);
    }

    newSpecHeader.pointer_to_first_record = activeSpecs.empty() ? -1 : newSpecOffsets[oldSpecOffsets[0]];
    newSpecHeader.pointer_to_free_memory = newOffset;
    fseek(db.spec_file, 0, SEEK_SET);
    fwrite(&newSpecHeader, sizeof(SpecHeader), 1, db.spec_file);
    db.spec_header = newSpecHeader;

    fflush(db.items_file);
    fflush(db.spec_file);

    cout << "Truncate выполнен успешно." << endl;
}
