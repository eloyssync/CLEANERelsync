#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shobjidl.h> // IFileOpenDialog
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <thread>
#include <atomic>

namespace fs = std::filesystem;

// Подключение системных библиотек
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "uxtheme.lib")

// Включение современного визуального стиля Windows Controls (v6)
#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Идентификаторы элементов управления
enum ControlIDs {
    IDC_LANG_COMBO = 101,
    IDC_LBL_HEADER,
    IDC_LBL_SUB,
    IDC_GB_REC,
    IDC_CB_COMMENTS,
    IDC_LBL_DESC_COMMENTS,
    IDC_CB_DOCS,
    IDC_LBL_DESC_DOCS,
    IDC_CB_SYMBOLS,
    IDC_LBL_DESC_SYMBOLS,
    IDC_GB_ADV,
    IDC_CB_BLANKS,
    IDC_LBL_DESC_BLANKS,
    IDC_CB_BACKUP,
    IDC_LBL_DESC_BACKUP,
    IDC_LBL_PATH,
    IDC_BTN_SELECT,
    IDC_EDIT_LOG,
    IDC_BTN_START
};

#define WM_APP_LOG  (WM_USER + 1)
#define WM_APP_DONE (WM_USER + 2)

// ==========================================
// 1. ЛОКАЛИЗАЦИЯ (i18n)
// ==========================================
struct LangDict {
    std::wstring title;
    std::wstring sub;
    std::wstring sec_rec;
    std::wstring sec_adv;
    std::wstring comments;
    std::wstring comments_desc;
    std::wstring docs;
    std::wstring docs_desc;
    std::wstring symbols;
    std::wstring symbols_desc;
    std::wstring blanks;
    std::wstring blanks_desc;
    std::wstring backup;
    std::wstring backup_desc;
    std::wstring no_path;
    std::wstring browse;
    std::wstring start;
    std::wstring ready;
    std::wstring loaded;
    std::wstring started;
    std::wstring opt;
    std::wstring fail;
    std::wstring done;
};

std::map<std::wstring, LangDict> LANG = {
    {
        L"en", {
            L"CLEANERelsync",
            L"Python Source Code Formatter & Optimizer",
            L"RECOMMENDED ACTIONS",
            L"ADDITIONAL SETTINGS",
            L"Remove inline comments (#)",
            L"Safely strips all hashtags & notes",
            L"Remove docstrings & notes",
            L"Preserves empty blocks via 'pass'",
            L"Normalize non-ASCII typography",
            L"Fixes em-dashes (—) & curly quotes",
            L"Compress vertical empty lines",
            L"Removes redundant empty rows",
            L"Create backup files (.bak)",
            L"Safety rollback in case of error",
            L"No project directory selected",
            L"Select Project Folder...",
            L"START OPTIMIZATION",
            L"Engine ready. Select directory to begin.",
            L"Target path set: ",
            L"Processing started...",
            L"Optimized: ",
            L"Error in: ",
            L"Task complete. Cleaned: %d, Errors: %d."
        }
    },
    {
        L"ru", {
            L"CLEANERelsync",
            L"Форматировщик и оптимизатор кода Python",
            L"РЕКОМЕНДУЕМЫЕ ДЕЙСТВИЯ",
            L"ДОПОЛНИТЕЛЬНЫЕ НАСТРОЙКИ",
            L"Удалить встроенные комментарии (#)",
            L"Безопасно удаляет все хэштеги и заметки",
            L"Удалить строки документации и примечания",
            L"Сохраняет пустые блоки путем вставки 'pass'",
            L"Нормализовать не-ASCII типографику",
            L"Исправляет длинные тире (—) и ИИ-кавычки",
            L"Сжать вертикальные пустые строки",
            L"Удаляет избыточные пустые строки",
            L"Создать файлы резервных копий (.bak)",
            L"Безопасный откат в случае ошибки",
            L"Папка проекта не была выбрана",
            L"Выбрать папку...",
            L"НАЧАТЬ ОПТИМИЗАЦИЮ",
            L"Движок готов. Выберите директорию, чтобы начать.",
            L"Целевой путь установлен: ",
            L"Обработка началась...",
            L"Очищен: ",
            L"Ошибка в: ",
            L"Задача завершена. Очищено: %d, Ошибок: %d."
        }
    },
    {
        L"et", {
            L"CLEANERelsync",
            L"Python lähtekoodi vormindaja ja optimeerija",
            L"SOOVITATAVAD TEGEVUSED",
            L"LISASEADED",
            L"Eemalda kommentaarid (#)",
            L"Kustutab kõik märkmed ja selgitused",
            L"Eemalda dokumendistringid (docstrings)",
            L"Säilitab funktsioonid lisades 'pass'",
            L"Normaliseeri tüpograafia (—, «»)",
            L"Asendab pikad kriipsud ja jutumärgid",
            L"Tihenda tühjad read",
            L"Eemaldab liigsed tühjad read ja vee",
            L"Loo varukoopiad (.bak)",
            L"Algfaili taastamine vea korral",
            L"Projekti kausta pole valitud",
            L"Vali kaust...",
            L"ALUSTA PUHASTAMIST",
            L"Süsteem on valmis. Vali projekti kaust.",
            L"Sihtkaust laaditud: ",
            L"Puhastamine käivitatud...",
            L"Optimeeritud: ",
            L"Viga failis: ",
            L"Valmis. Töödeldud: %d, vead: %d."
        }
    }
};

// ==========================================
// 2. ДВИЖОК ОЧИСТКИ PYTHON-КОДА (UTF-8)
// ==========================================
struct CleanerOptions {
    bool rm_comments = true;
    bool rm_docs = true;
    bool rm_symbols = true;
    bool rm_blanks = true;
    bool backup = true;
};

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

std::string NormalizeTypography(std::string text) {
    text = ReplaceAll(text, "\xE2\x80\x94", "-"); // —
    text = ReplaceAll(text, "\xE2\x80\x93", "-"); // –
    text = ReplaceAll(text, "\xC2\xAB", "'");     // «
    text = ReplaceAll(text, "\xC2\xBB", "'");     // »
    text = ReplaceAll(text, "\xE2\x80\x9C", "'"); // “
    text = ReplaceAll(text, "\xE2\x80\x9D", "'"); // ”
    text = ReplaceAll(text, "\xE2\x80\x98", "'"); // ‘
    text = ReplaceAll(text, "\xE2\x80\x99", "'"); // ’
    return text;
}

std::string CleanPythonCode(const std::string& source, const CleanerOptions& opts) {
    std::string text = source;
    if (opts.rm_symbols) {
        text = NormalizeTypography(text);
    }

    std::vector<std::string> lines;
    {
        std::istringstream stream(text);
        std::string line;
        while (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            lines.push_back(line);
        }
    }

    auto HasOtherStatements = [&](size_t doc_end_idx, size_t doc_indent_len) {
        for (size_t k = doc_end_idx + 1; k < lines.size(); ++k) {
            const std::string& l = lines[k];
            size_t n_ws = l.find_first_not_of(" \t");
            if (n_ws == std::string::npos) continue;
            if (n_ws < doc_indent_len) break;
            if (l[n_ws] == '#') continue;
            return true;
        }
        return false;
    };

    std::vector<std::string> processed_lines;
    bool in_multiline_string = false;
    std::string multiline_delimiter = "";
    bool is_docstring_block = false;

    for (size_t i = 0; i < lines.size(); ++i) {
        std::string line = lines[i];
        size_t line_num = i + 1;

        if (in_multiline_string) {
            size_t end_pos = line.find(multiline_delimiter);
            if (end_pos != std::string::npos) {
                in_multiline_string = false;
                if (is_docstring_block && opts.rm_docs) {
                    is_docstring_block = false;
                    continue;
                }
                line = line.substr(end_pos + 3);
            } else {
                if (is_docstring_block && opts.rm_docs) continue;
                processed_lines.push_back(line);
                continue;
            }
        }

        size_t first_non_ws = line.find_first_not_of(" \t");
        if (first_non_ws == std::string::npos) {
            processed_lines.push_back("");
            continue;
        }

        std::string indent = line.substr(0, first_non_ws);
        std::string trimmed = line.substr(first_non_ws);

        // Сохраняем Shebang (#!) и кодировку на первых строках
        if (trimmed.rfind("#!", 0) == 0 && line_num == 1) {
            processed_lines.push_back(line);
            continue;
        }
        if (line_num <= 2 && trimmed.find("coding") != std::string::npos && trimmed[0] == '#') {
            processed_lines.push_back(line);
            continue;
        }

        // Проверка на Docstring
        if (opts.rm_docs && (trimmed.rfind("\"\"\"", 0) == 0 || trimmed.rfind("'''", 0) == 0)) {
            std::string delim = trimmed.substr(0, 3);
            size_t closing = trimmed.find(delim, 3);

            bool prev_is_block_start = false;
            for (int p = (int)processed_lines.size() - 1; p >= 0; --p) {
                const std::string& pl = processed_lines[p];
                size_t p_non_ws = pl.find_first_not_of(" \t");
                if (p_non_ws != std::string::npos) {
                    if (pl.back() == ':') prev_is_block_start = true;
                    break;
                }
            }

            if (closing != std::string::npos) {
                if (prev_is_block_start && !HasOtherStatements(i, first_non_ws)) {
                    processed_lines.push_back(indent + "pass");
                }
                continue;
            } else {
                in_multiline_string = true;
                multiline_delimiter = delim;
                is_docstring_block = true;

                size_t close_line_idx = i;
                for (size_t cl = i + 1; cl < lines.size(); ++cl) {
                    if (lines[cl].find(delim) != std::string::npos) {
                        close_line_idx = cl;
                        break;
                    }
                }

                if (prev_is_block_start && !HasOtherStatements(close_line_idx, first_non_ws)) {
                    processed_lines.push_back(indent + "pass");
                }
                continue;
            }
        }

        // Удаление комментариев
        if (opts.rm_comments) {
            std::string out_line;
            bool in_str = false;
            char str_char = '\0';
            bool escaped = false;

            for (size_t c = 0; c < line.size(); ++c) {
                char ch = line[c];
                if (in_str) {
                    out_line += ch;
                    if (escaped) {
                        escaped = false;
                    } else if (ch == '\\') {
                        escaped = true;
                    } else if (ch == str_char) {
                        in_str = false;
                    }
                } else {
                    if (ch == '\'' || ch == '"') {
                        in_str = true;
                        str_char = ch;
                        out_line += ch;
                    } else if (ch == '#') {
                        break;
                    } else {
                        out_line += ch;
                    }
                }
            }

            size_t last_char = out_line.find_last_not_of(" \t");
            if (last_char != std::string::npos) {
                out_line = out_line.substr(0, last_char + 1);
            } else {
                out_line = "";
            }
            line = out_line;
        }

        processed_lines.push_back(line);
    }

    // Сжатие пустых строк
    std::vector<std::string> clean_lines;
    for (const auto& l : processed_lines) {
        if (!l.empty() || (!clean_lines.empty() && !clean_lines.back().empty())) {
            clean_lines.push_back(l);
        }
    }

    if (opts.rm_blanks) {
        std::vector<std::string> final_lines;
        for (const auto& l : clean_lines) {
            if (l.empty() && !final_lines.empty() && final_lines.back().empty()) {
                continue;
            }
            final_lines.push_back(l);
        }
        clean_lines = final_lines;
    }

    while (!clean_lines.empty() && clean_lines.back().empty()) {
        clean_lines.pop_back();
    }

    std::string result;
    for (const auto& l : clean_lines) {
        result += l + "\n";
    }
    return result;
}

// ==========================================
// 3. ГРАФИЧЕСКИЙ ИНТЕРФЕЙС WIN32
// ==========================================
HWND hMainWnd;
HWND hComboLang, hLblHeader, hLblSub;
HWND hGbRec, hCbComments, hLblDescComments;
HWND hCbDocs, hLblDescDocs, hCbSymbols, hLblDescSymbols;
HWND hGbAdv, hCbBlanks, hLblDescBlanks, hCbBackup, hLblDescBackup;
HWND hLblPath, hBtnSelect, hEditLog, hBtnStart;

HFONT hFontBoldLarge = NULL;
HFONT hFontRegular = NULL;
HFONT hFontSmall = NULL;
HFONT hFontMonospace = NULL;

std::wstring curLang = L"en";
std::wstring selectedFolder = L"";
std::atomic<bool> isProcessing(false);

void AppendLog(const std::wstring& text, const std::wstring& status = L"INFO") {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm bt{};
    localtime_s(&bt, &in_time_t);

    std::wstringstream ss;
    ss << L"[" << std::put_time(&bt, L"%H:%M:%S") << L"] [" << status << L"] " << text << L"\r\n";
    
    std::wstring* pMsg = new std::wstring(ss.str());
    PostMessage(hMainWnd, WM_APP_LOG, 0, (LPARAM)pMsg);
}

void ApplyTranslations() {
    const LangDict& t = LANG[curLang];
    SetWindowTextW(hMainWnd, t.title.c_str());
    SetWindowTextW(hLblSub, t.sub.c_str());
    SetWindowTextW(hGbRec, t.sec_rec.c_str());
    SetWindowTextW(hGbAdv, t.sec_adv.c_str());

    SetWindowTextW(hCbComments, t.comments.c_str());
    SetWindowTextW(hLblDescComments, t.comments_desc.c_str());

    SetWindowTextW(hCbDocs, t.docs.c_str());
    SetWindowTextW(hLblDescDocs, t.docs_desc.c_str());

    SetWindowTextW(hCbSymbols, t.symbols.c_str());
    SetWindowTextW(hLblDescSymbols, t.symbols_desc.c_str());

    SetWindowTextW(hCbBlanks, t.blanks.c_str());
    SetWindowTextW(hLblDescBlanks, t.blanks_desc.c_str());

    SetWindowTextW(hCbBackup, t.backup.c_str());
    SetWindowTextW(hLblDescBackup, t.backup_desc.c_str());

    SetWindowTextW(hBtnSelect, t.browse.c_str());
    SetWindowTextW(hBtnStart, t.start.c_str());

    if (selectedFolder.empty()) {
        SetWindowTextW(hLblPath, t.no_path.c_str());
    }
}

void SelectFolderDialog() {
    IFileOpenDialog *pfd = NULL;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)))) {
        DWORD dwOptions;
        if (SUCCEEDED(pfd->GetOptions(&dwOptions))) {
            pfd->SetOptions(dwOptions | FOS_PICKFOLDERS);
        }
        if (SUCCEEDED(pfd->Show(hMainWnd))) {
            IShellItem *psi = NULL;
            if (SUCCEEDED(pfd->GetResult(&psi))) {
                PWSTR pszPath = NULL;
                if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                    selectedFolder = pszPath;
                    SetWindowTextW(hLblPath, selectedFolder.c_str());
                    EnableWindow(hBtnStart, TRUE);
                    AppendLog(LANG[curLang].loaded + selectedFolder, L"OK");
                    CoTaskMemFree(pszPath);
                }
                psi->Release();
            }
        }
        pfd->Release();
    }
}

void ProcessFolderWorker() {
    isProcessing = true;
    EnableWindow(hBtnStart, FALSE);
    EnableWindow(hBtnSelect, FALSE);

    CleanerOptions opts;
    opts.rm_comments = (SendMessage(hCbComments, BM_GETCHECK, 0, 0) == BST_CHECKED);
    opts.rm_docs     = (SendMessage(hCbDocs, BM_GETCHECK, 0, 0) == BST_CHECKED);
    opts.rm_symbols  = (SendMessage(hCbSymbols, BM_GETCHECK, 0, 0) == BST_CHECKED);
    opts.rm_blanks   = (SendMessage(hCbBlanks, BM_GETCHECK, 0, 0) == BST_CHECKED);
    opts.backup      = (SendMessage(hCbBackup, BM_GETCHECK, 0, 0) == BST_CHECKED);

    const LangDict& t = LANG[curLang];
    AppendLog(t.started, L"INFO");

    int count = 0;
    int failed = 0;

    try {
        for (const auto& entry : fs::recursive_directory_iterator(selectedFolder)) {
            if (!entry.is_regular_file()) continue;

            std::string pathStr = entry.path().u8string();
            if (pathStr.find("venv") != std::string::npos ||
                pathStr.find("__pycache__") != std::string::npos ||
                pathStr.find(".git") != std::string::npos) {
                continue;
            }

            if (entry.path().extension() == ".py") {
                try {
                    std::ifstream in(entry.path(), std::ios::binary);
                    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                    in.close();

                    std::string cleaned = CleanPythonCode(content, opts);
                    if (content != cleaned) {
                        if (opts.backup) {
                            std::ofstream bak(entry.path().u8string() + ".bak", std::ios::binary);
                            bak.write(content.data(), content.size());
                            bak.close();
                        }
                        std::ofstream out(entry.path(), std::ios::binary);
                        out.write(cleaned.data(), cleaned.size());
                        out.close();

                        count++;
                        AppendLog(t.opt + entry.path().filename().wstring(), L"OK");
                    }
                } catch (const std::exception& ex) {
                    failed++;
                    std::string err = ex.what();
                    AppendLog(t.fail + entry.path().filename().wstring() + L" (" + std::wstring(err.begin(), err.end()) + L")", L"ERR");
                }
            }
        }
    } catch (const std::exception& ex) {
        std::string err = ex.what();
        AppendLog(L"Directory error: " + std::wstring(err.begin(), err.end()), L"ERR");
    }

    wchar_t doneBuf[256];
    swprintf_s(doneBuf, t.done.c_str(), count, failed);
    AppendLog(doneBuf, L"INFO");

    PostMessage(hMainWnd, WM_APP_DONE, 0, 0);
    isProcessing = false;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        hMainWnd = hWnd;
        INITCOMMONCONTROLSEX icex{ sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES };
        InitCommonControlsEx(&icex);

        // Создание шрифтов
        hFontBoldLarge = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        hFontRegular   = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        hFontSmall     = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        hFontMonospace = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");

        // Заголовок и подзаголовок
        hLblHeader = CreateWindowW(L"STATIC", L"CLEANERelsync", WS_VISIBLE | WS_CHILD, 20, 14, 250, 24, hWnd, (HMENU)IDC_LBL_HEADER, NULL, NULL);
        SendMessage(hLblHeader, WM_SETFONT, (WPARAM)hFontBoldLarge, TRUE);

        hLblSub = CreateWindowW(L"STATIC", L"", WS_VISIBLE | WS_CHILD, 20, 38, 360, 20, hWnd, (HMENU)IDC_LBL_SUB, NULL, NULL);
        SendMessage(hLblSub, WM_SETFONT, (WPARAM)hFontRegular, TRUE);

        // Выбор языка
        hComboLang = CreateWindowW(L"COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, 410, 14, 110, 150, hWnd, (HMENU)IDC_LANG_COMBO, NULL, NULL);
        SendMessage(hComboLang, CB_ADDSTRING, 0, (LPARAM)L"English");
        SendMessage(hComboLang, CB_ADDSTRING, 0, (LPARAM)L"Русский");
        SendMessage(hComboLang, CB_ADDSTRING, 0, (LPARAM)L"Eesti");
        SendMessage(hComboLang, CB_SETCURSEL, 0, 0);
        SendMessage(hComboLang, WM_SETFONT, (WPARAM)hFontRegular, TRUE);

        // Группа: Рекомендуемые действия
        hGbRec = CreateWindowW(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 20, 68, 500, 160, hWnd, (HMENU)IDC_GB_REC, NULL, NULL);
        SendMessage(hGbRec, WM_SETFONT, (WPARAM)hFontRegular, TRUE);

        hCbComments = CreateWindowW(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 35, 90, 470, 20, hWnd, (HMENU)IDC_CB_COMMENTS, NULL, NULL);
        hLblDescComments = CreateWindowW(L"STATIC", L"", WS_VISIBLE | WS_CHILD, 55, 110, 450, 16, hWnd, (HMENU)IDC_LBL_DESC_COMMENTS, NULL, NULL);

        hCbDocs = CreateWindowW(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 35, 130, 470, 20, hWnd, (HMENU)IDC_CB_DOCS, NULL, NULL);
        hLblDescDocs = CreateWindowW(L"STATIC", L"", WS_VISIBLE | WS_CHILD, 55, 150, 450, 16, hWnd, (HMENU)IDC_LBL_DESC_DOCS, NULL, NULL);

        hCbSymbols = CreateWindowW(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 35, 170, 470, 20, hWnd, (HMENU)IDC_CB_SYMBOLS, NULL, NULL);
        hLblDescSymbols = CreateWindowW(L"STATIC", L"", WS_VISIBLE | WS_CHILD, 55, 190, 450, 16, hWnd, (HMENU)IDC_LBL_DESC_SYMBOLS, NULL, NULL);

        // Группа: Дополнительные настройки
        hGbAdv = CreateWindowW(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 20, 238, 500, 115, hWnd, (HMENU)IDC_GB_ADV, NULL, NULL);
        SendMessage(hGbAdv, WM_SETFONT, (WPARAM)hFontRegular, TRUE);

        hCbBlanks = CreateWindowW(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 35, 260, 470, 20, hWnd, (HMENU)IDC_CB_BLANKS, NULL, NULL);
        hLblDescBlanks = CreateWindowW(L"STATIC", L"", WS_VISIBLE | WS_CHILD, 55, 280, 450, 16, hWnd, (HMENU)IDC_LBL_DESC_BLANKS, NULL, NULL);

        hCbBackup = CreateWindowW(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 35, 300, 470, 20, hWnd, (HMENU)IDC_CB_BACKUP, NULL, NULL);
        hLblDescBackup = CreateWindowW(L"STATIC", L"", WS_VISIBLE | WS_CHILD, 55, 320, 450, 16, hWnd, (HMENU)IDC_LBL_DESC_BACKUP, NULL, NULL);

        // Установка шрифтов и состояния чекбоксов
        HWND checkBoxes[] = { hCbComments, hCbDocs, hCbSymbols, hCbBlanks, hCbBackup };
        for (HWND cb : checkBoxes) {
            SendMessage(cb, BM_SETCHECK, BST_CHECKED, 0);
            SendMessage(cb, WM_SETFONT, (WPARAM)hFontRegular, TRUE);
        }
        HWND descLabels[] = { hLblDescComments, hLblDescDocs, hLblDescSymbols, hLblDescBlanks, hLblDescBackup };
        for (HWND lbl : descLabels) {
            SendMessage(lbl, WM_SETFONT, (WPARAM)hFontSmall, TRUE);
        }

        // Выбор пути
        hLblPath = CreateWindowW(L"STATIC", L"", WS_VISIBLE | WS_CHILD | SS_CENTER, 20, 362, 500, 20, hWnd, (HMENU)IDC_LBL_PATH, NULL, NULL);
        SendMessage(hLblPath, WM_SETFONT, (WPARAM)hFontSmall, TRUE);

        hBtnSelect = CreateWindowW(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 20, 386, 500, 34, hWnd, (HMENU)IDC_BTN_SELECT, NULL, NULL);
        SendMessage(hBtnSelect, WM_SETFONT, (WPARAM)hFontRegular, TRUE);

        // Окно лога (терминал)
        hEditLog = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            20, 428, 500, 135, hWnd, (HMENU)IDC_EDIT_LOG, NULL, NULL);
        SendMessage(hEditLog, WM_SETFONT, (WPARAM)hFontMonospace, TRUE);

        // Кнопка Старт
        hBtnStart = CreateWindowW(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_DISABLED,
            20, 572, 500, 38, hWnd, (HMENU)IDC_BTN_START, NULL, NULL);
        SendMessage(hBtnStart, WM_SETFONT, (WPARAM)hFontBoldLarge, TRUE);

        ApplyTranslations();
        AppendLog(LANG[curLang].ready, L"INFO");
        return 0;
    }

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        int wmEvent = HIWORD(wParam);

        if (wmId == IDC_LANG_COMBO && wmEvent == CBN_SELCHANGE) {
            int idx = (int)SendMessage(hComboLang, CB_GETCURSEL, 0, 0);
            if (idx == 0) curLang = L"en";
            else if (idx == 1) curLang = L"ru";
            else if (idx == 2) curLang = L"et";
            ApplyTranslations();
        }
        else if (wmId == IDC_BTN_SELECT) {
            SelectFolderDialog();
        }
        else if (wmId == IDC_BTN_START) {
            if (!selectedFolder.empty() && !isProcessing) {
                std::thread worker(ProcessFolderWorker);
                worker.detach();
            }
        }
        return 0;
    }

    case WM_APP_LOG: {
        std::wstring* pMsg = reinterpret_cast<std::wstring*>(lParam);
        if (pMsg) {
            int len = GetWindowTextLengthW(hEditLog);
            SendMessage(hEditLog, EM_SETSEL, (WPARAM)len, (LPARAM)len);
            SendMessage(hEditLog, EM_REPLACESEL, FALSE, (LPARAM)pMsg->c_str());
            delete pMsg;
        }
        return 0;
    }

    case WM_APP_DONE: {
        EnableWindow(hBtnStart, TRUE);
        EnableWindow(hBtnSelect, TRUE);
        return 0;
    }

    case WM_DESTROY:
        if (hFontBoldLarge) DeleteObject(hFontBoldLarge);
        if (hFontRegular) DeleteObject(hFontRegular);
        if (hFontSmall) DeleteObject(hFontSmall);
        if (hFontMonospace) DeleteObject(hFontMonospace);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    WNDCLASSEXW wc{ sizeof(WNDCLASSEXW) };
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hInstance = hInstance;
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = L"CleanerElsyncClass";
    wc.style = CS_HREDRAW | CS_VREDRAW;

    RegisterClassExW(&wc);

    HWND hWnd = CreateWindowExW(
        0,
        L"CleanerElsyncClass",
        L"CLEANERelsync",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 556, 665,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CoUninitialize();
    return (int)msg.wParam;
}