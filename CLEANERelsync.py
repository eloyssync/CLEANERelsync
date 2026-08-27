import sys
import os
import ast
import io
import tokenize
from datetime import datetime
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QPushButton, QCheckBox, QFileDialog, QFrame, QTextEdit,
    QComboBox
)
from PyQt6.QtCore import Qt, QPoint
from PyQt6.QtGui import QFont, QCursor

LANG = {
    "en": {
        "title": "CLEANERelsync",
        "sub": "Python Source Code Formatter & Optimizer",
        "sec_rec": "RECOMMENDED ACTIONS",
        "sec_adv": "ADDITIONAL SETTINGS",
        "comments": "Remove inline comments (#)",
        "comments_desc": "Safely strips all hashtags & notes",
        "docs": "Remove docstrings & notes",
        "docs_desc": "Preserves empty blocks via 'pass'",
        "symbols": "Normalize non-ASCII typography",
        "symbols_desc": "Fixes em-dashes (—) & curly quotes",
        "blanks": "Compress vertical empty lines",
        "blanks_desc": "Removes redundant empty rows",
        "backup": "Create backup files (.bak)",
        "backup_desc": "Safety rollback in case of error",
        "no_path": "No project directory selected",
        "browse": "Select Project Folder...",
        "start": "START OPTIMIZATION",
        "ready": "Engine ready. Select directory to begin.",
        "loaded": "Target path set: ",
        "started": "Processing started...",
        "opt": "Optimized: ",
        "fail": "Error in: ",
        "done": "Task complete. Cleaned: {count}, Errors: {err}."
    },
    "ru": {
        "title": "CLEANERelsync",
        "sub": "Форматировщик и оптимизатор кода Python",
        "sec_rec": "РЕКОМЕНДУЕМЫЕ ДЕЙСТВИЯ",
        "sec_adv": "ДОПОЛНИТЕЛЬНЫЕ НАСТРОЙКИ",
        "comments": "Удалить встроенные комментарии (#)",
        "comments_desc": "Безопасно удаляет все хэштеги и заметки",
        "docs": "Удалить строки документации и примечания",
        "docs_desc": "Сохраняет пустые блоки путем вставки 'pass'",
        "symbols": "Нормализовать не-ASCII типографику",
        "symbols_desc": "Исправляет длинные тире (—) и ИИ-кавычки",
        "blanks": "Сжать вертикальные пустые строки",
        "blanks_desc": "Удаляет избыточные пустые строки",
        "backup": "Создать файлы резервных копий (.bak)",
        "backup_desc": "Безопасный откат в случае ошибки",
        "no_path": "Папка проекта не была выбрана",
        "browse": "Выбрать папку...",
        "start": "НАЧАТЬ ОПТИМИЗАЦИЮ",
        "ready": "Движок готов. Выберите директорию, чтобы начать.",
        "loaded": "Целевой путь установлен: ",
        "started": "Обработка началась...",
        "opt": "Очищен: ",
        "fail": "Ошибка в: ",
        "done": "Задача завершена. Очищено: {count}, Ошибок: {err}."
    },
    "et": {
        "title": "CLEANERelsync",
        "sub": "Python lähtekoodi vormindaja ja optimeerija",
        "sec_rec": "SOOVITATAVAD TEGEVUSED",
        "sec_adv": "LISASEADED",
        "comments": "Eemalda kommentaarid (#)",
        "comments_desc": "Kustutab kõik märkmed ja selgitused",
        "docs": "Eemalda dokumendistringid (docstrings)",
        "docs_desc": "Säilitab funktsioonid lisades 'pass'",
        "symbols": "Normaliseeri tüpograafia (—, «»)",
        "symbols_desc": "Asendab pikad kriipsud ja jutumärgid",
        "blanks": "Tihenda tühjad read",
        "blanks_desc": "Eemaldab liigsed tühjad read ja vee",
        "backup": "Loo varukoopiad (.bak)",
        "backup_desc": "Algfaili taastamine vea korral",
        "no_path": "Projekti kausta pole valitud",
        "browse": "Vali kaust...",
        "start": "ALUSTA PUHASTAMIST",
        "ready": "Süsteem on valmis. Vali projekti kaust.",
        "loaded": "Sihtkaust laaditud: ",
        "started": "Puhastamine käivitatud...",
        "opt": "Optimeeritud: ",
        "fail": "Viga failis: ",
        "done": "Valmis. Töödeldud: {count}, vead: {err}."
    }
}

STRING_TOKENS = {tokenize.STRING}
for attr in ['FSTRING_START', 'FSTRING_MIDDLE', 'FSTRING_END']:
    if hasattr(tokenize, attr):
        STRING_TOKENS.add(getattr(tokenize, attr))

def get_spans(source):
    tree = ast.parse(source)
    remove_spans = []
    pass_spans = []
    for node in ast.walk(tree):
        body = getattr(node, "body", None)
        if isinstance(body, list) and len(body) == 1:
            first = body[0]
            if (
                isinstance(first, ast.Expr)
                and isinstance(first.value, ast.Constant)
                and isinstance(first.value.value, str)
            ):
                pass_spans.append(
                    ((first.value.lineno, first.value.col_offset),
                     (first.value.end_lineno, first.value.end_col_offset))
                )
                continue
        if (
            isinstance(node, ast.Expr)
            and isinstance(node.value, ast.Constant)
            and isinstance(node.value.value, str)
        ):
            remove_spans.append(
                ((node.value.lineno, node.value.col_offset),
                 (node.value.end_lineno, node.value.end_col_offset))
            )
    return remove_spans, pass_spans

def is_inside(start, end, spans):
    return any(s <= start and end <= e for s, e in spans)

def clean_code(source, opts):
    remove_spans, pass_spans = get_spans(source)
    out = []
    last_lineno = -1
    last_col = 0

    symbol_map = {
        ord("—"): "-", ord("–"): "-",
        ord("«"): "'", ord("»"): "'",
        ord("“"): "'", ord("”"): "'",
        ord("‘"): "'", ord("’"): "'"
    }

    try:
        token_stream = tokenize.generate_tokens(io.StringIO(source).readline)
        for toktype, ttext, start, end, _ in token_stream:
            slineno, scol = start
            elineno, ecol = end

            if slineno > last_lineno:
                last_col = 0
            if scol > last_col:
                out.append(" " * (scol - last_col))

            if toktype == tokenize.COMMENT:
                if slineno == 1 and ttext.startswith("#!"):
                    out.append(ttext)
                elif slineno <= 2 and "coding" in ttext:
                    out.append(ttext)
                elif not opts["rm_comments"]:
                    if opts["rm_symbols"]:
                        out.append(ttext.translate(symbol_map))
                    else:
                        out.append(ttext)
            elif toktype in STRING_TOKENS:
                text_to_append = ttext
                if opts["rm_symbols"]:
                    text_to_append = ttext.translate(symbol_map)

                if opts["rm_docs"] and toktype == tokenize.STRING:
                    if is_inside(start, end, pass_spans):
                        out.append("pass")
                    elif is_inside(start, end, remove_spans):
                        pass
                    else:
                        out.append(text_to_append)
                else:
                    out.append(text_to_append)
            else:
                out.append(ttext)

            last_lineno = elineno
            last_col = ecol
    except tokenize.TokenError as e:
        raise SyntaxError(f"Tokenization error (possibly unclosed parenthesis or string): {e}")

    raw_text = "".join(out)
    lines = [line.rstrip() for line in raw_text.splitlines()]

    clean_lines = []
    for line in lines:
        if line or (clean_lines and clean_lines[-1] != ""):
            clean_lines.append(line)

    if opts["rm_blanks"]:
        final_lines = []
        for line in clean_lines:
            if line == "" and final_lines and final_lines[-1] == "":
                continue
            final_lines.append(line)
        clean_lines = final_lines

    while clean_lines and clean_lines[-1] == "":
        clean_lines.pop()

    final_text = "\n".join(clean_lines) + "\n"

    try:
        ast.parse(final_text)
    except SyntaxError as e:
        raise SyntaxError(f"Syntax is broken after cleaning: {e}")

    return final_text

class AppWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowFlags(Qt.WindowType.FramelessWindowHint)
        self.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground)
        self.setFixedSize(560, 720)
        self.drag_position = QPoint()
        self.folder_path = ""
        self.cur_lang = "en"
        self.init_ui()

    def init_ui(self):
        self.setStyleSheet("""
            QWidget#mainContainer {
                background-color: #0b0f17;
                border: 1px solid #1e293b;
                border-radius: 12px;
            }
            QFrame#topBar {
                border-bottom: 1px solid #1e293b;
            }
            QLabel {
                color: #e2e8f0;
                font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            }
            QFrame#card {
                background-color: #111827;
                border: 1px solid #1f2937;
                border-radius: 8px;
            }
            QLabel#secTitle {
                color: #38bdf8;
                font-size: 10px;
                font-weight: 800;
                letter-spacing: 1px;
            }
            QCheckBox {
                color: #f1f5f9;
                font-size: 13px;
                font-weight: 600;
                spacing: 12px;
            }
            QCheckBox::indicator {
                width: 20px;
                height: 20px;
                border-radius: 5px;
                border: 1px solid #334155;
                background-color: #0f172a;
            }
            QCheckBox::indicator:hover {
                border-color: #38bdf8;
            }
            QCheckBox::indicator:checked {
                background-color: #2563eb;
                border-color: #2563eb;
                image: url('data:image/svg+xml;utf8,<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"><polyline points="20 6 9 17 4 12"></polyline></svg>');
            }
            QLabel#descLabel {
                color: #64748b;
                font-size: 11px;
                margin-left: 32px;
                margin-top: -4px;
            }
            QComboBox {
                background-color: #141b26;
                border: 1px solid #232f42;
                border-radius: 6px;
                color: #cbd5e1;
                font-size: 11px;
                padding: 3px 8px;
            }
            QComboBox::drop-down {
                border: none;
            }
            QComboBox QAbstractItemView {
                background-color: #141b26;
                border: 1px solid #232f42;
                selection-background-color: #2563eb;
                color: #ffffff;
            }
            QTextEdit#terminal {
                background-color: #06090e;
                border: 1px solid #1e293b;
                border-radius: 6px;
                color: #38bdf8;
                font-family: Consolas, "Courier New", monospace;
                font-size: 11px;
                padding: 8px;
            }
            QPushButton#btnSelect {
                background-color: #1e293b;
                border: 1px solid #334155;
                color: #f1f5f9;
                border-radius: 6px;
                font-size: 12px;
                font-weight: 600;
                padding: 9px;
            }
            QPushButton#btnSelect:hover {
                background-color: #2e3d56;
                border-color: #475569;
            }
            QPushButton#btnStart {
                background-color: #2563eb;
                color: #ffffff;
                border: none;
                border-radius: 6px;
                font-size: 13px;
                font-weight: 700;
                padding: 12px;
            }
            QPushButton#btnStart:hover {
                background-color: #1d4ed8;
            }
            QPushButton#btnStart:disabled {
                background-color: #182234;
                color: #475569;
            }
            QPushButton#winBtn {
                background-color: transparent;
                border: none;
                color: #64748b;
                font-size: 14px;
                font-weight: bold;
                border-radius: 4px;
                padding: 2px 6px;
            }
            QPushButton#winBtn:hover {
                background-color: #1e293b;
                color: #f8fafc;
            }
            QPushButton#closeBtn:hover {
                background-color: #dc2626;
                color: #ffffff;
            }
        """)

        container = QWidget()
        container.setObjectName("mainContainer")
        self.setCentralWidget(container)

        root_layout = QVBoxLayout(container)
        root_layout.setContentsMargins(0, 0, 0, 0)
        root_layout.setSpacing(0)

        top_bar = QFrame()
        top_bar.setObjectName("topBar")
        top_bar.setFixedHeight(44)
        top_layout = QHBoxLayout(top_bar)
        top_layout.setContentsMargins(16, 0, 8, 0)

        self.lbl_header = QLabel("CLEANERelsync")
        self.lbl_header.setFont(QFont("Segoe UI", 10, QFont.Weight.Bold))
        self.lbl_header.setStyleSheet("color: #38bdf8;")
        top_layout.addWidget(self.lbl_header)

        top_layout.addStretch()

        self.lang_box = QComboBox()
        self.lang_box.addItems(["English", "Русский", "Eesti"])
        self.lang_box.currentIndexChanged.connect(self.switch_language)
        top_layout.addWidget(self.lang_box)

        btn_min = QPushButton("─")
        btn_min.setObjectName("winBtn")
        btn_min.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))
        btn_min.clicked.connect(self.showMinimized)
        top_layout.addWidget(btn_min)

        btn_close = QPushButton("✕")
        btn_close.setObjectName("winBtn")
        btn_close.setStyleSheet("QPushButton#winBtn:hover { background-color: #dc2626; color: #ffffff; }")
        btn_close.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))
        btn_close.clicked.connect(self.close)
        top_layout.addWidget(btn_close)

        root_layout.addWidget(top_bar)

        body = QVBoxLayout()
        body.setContentsMargins(24, 16, 24, 20)
        body.setSpacing(12)

        self.lbl_sub = QLabel()
        self.lbl_sub.setFont(QFont("Segoe UI", 9))
        self.lbl_sub.setStyleSheet("color: #64748b;")
        body.addWidget(self.lbl_sub)

        card = QFrame()
        card.setObjectName("card")
        card_layout = QVBoxLayout(card)
        card_layout.setContentsMargins(16, 14, 16, 14)
        card_layout.setSpacing(6)

        self.lbl_sec_rec = QLabel()
        self.lbl_sec_rec.setObjectName("secTitle")
        card_layout.addWidget(self.lbl_sec_rec)

        self.cb_comments = QCheckBox()
        self.lbl_desc_comments = QLabel()
        self.lbl_desc_comments.setObjectName("descLabel")
        card_layout.addWidget(self.cb_comments)
        card_layout.addWidget(self.lbl_desc_comments)

        self.cb_docs = QCheckBox()
        self.lbl_desc_docs = QLabel()
        self.lbl_desc_docs.setObjectName("descLabel")
        card_layout.addWidget(self.cb_docs)
        card_layout.addWidget(self.lbl_desc_docs)

        self.cb_symbols = QCheckBox()
        self.lbl_desc_symbols = QLabel()
        self.lbl_desc_symbols.setObjectName("descLabel")
        card_layout.addWidget(self.cb_symbols)
        card_layout.addWidget(self.lbl_desc_symbols)

        self.lbl_sec_adv = QLabel()
        self.lbl_sec_adv.setObjectName("secTitle")
        self.lbl_sec_adv.setStyleSheet("color: #94a3b8; font-size: 10px; font-weight: 800; letter-spacing: 1px; margin-top: 6px;")
        card_layout.addWidget(self.lbl_sec_adv)

        self.cb_blanks = QCheckBox()
        self.lbl_desc_blanks = QLabel()
        self.lbl_desc_blanks.setObjectName("descLabel")
        card_layout.addWidget(self.cb_blanks)
        card_layout.addWidget(self.lbl_desc_blanks)

        self.cb_backup = QCheckBox()
        self.lbl_desc_backup = QLabel()
        self.lbl_desc_backup.setObjectName("descLabel")
        card_layout.addWidget(self.cb_backup)
        card_layout.addWidget(self.lbl_desc_backup)

        for cb in (self.cb_comments, self.cb_docs, self.cb_symbols, self.cb_blanks, self.cb_backup):
            cb.setChecked(True)
            cb.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))

        body.addWidget(card)

        self.lbl_path = QLabel()
        self.lbl_path.setStyleSheet("color: #64748b; font-size: 11px;")
        self.lbl_path.setAlignment(Qt.AlignmentFlag.AlignCenter)
        body.addWidget(self.lbl_path)

        self.btn_select = QPushButton()
        self.btn_select.setObjectName("btnSelect")
        self.btn_select.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))
        self.btn_select.clicked.connect(self.select_folder)
        body.addWidget(self.btn_select)

        self.terminal = QTextEdit()
        self.terminal.setObjectName("terminal")
        self.terminal.setReadOnly(True)
        self.terminal.setFixedHeight(120)
        body.addWidget(self.terminal)

        self.btn_start = QPushButton()
        self.btn_start.setObjectName("btnStart")
        self.btn_start.setEnabled(False)
        self.btn_start.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))
        self.btn_start.clicked.connect(self.run_clean)
        body.addWidget(self.btn_start)

        root_layout.addLayout(body)

        self.apply_translations()
        self.log_msg(LANG[self.cur_lang]["ready"])

    def switch_language(self, idx):
        if idx == 0:
            self.cur_lang = "en"
        elif idx == 1:
            self.cur_lang = "ru"
        elif idx == 2:
            self.cur_lang = "et"
        self.apply_translations()

    def apply_translations(self):
        t = LANG[self.cur_lang]
        self.lbl_sub.setText(t["sub"])
        self.lbl_sec_rec.setText(t["sec_rec"])
        self.lbl_sec_adv.setText(t["sec_adv"])

        self.cb_comments.setText(t["comments"])
        self.lbl_desc_comments.setText(t["comments_desc"])

        self.cb_docs.setText(t["docs"])
        self.lbl_desc_docs.setText(t["docs_desc"])

        self.cb_symbols.setText(t["symbols"])
        self.lbl_desc_symbols.setText(t["symbols_desc"])

        self.cb_blanks.setText(t["blanks"])
        self.lbl_desc_blanks.setText(t["blanks_desc"])

        self.cb_backup.setText(t["backup"])
        self.lbl_desc_backup.setText(t["backup_desc"])

        self.btn_select.setText(t["browse"])
        self.btn_start.setText(t["start"])
        if not self.folder_path:
            self.lbl_path.setText(t["no_path"])

    def mousePressEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            self.drag_position = event.globalPosition().toPoint() - self.frameGeometry().topLeft()
            event.accept()

    def mouseMoveEvent(self, event):
        if event.buttons() == Qt.MouseButton.LeftButton:
            self.move(event.globalPosition().toPoint() - self.drag_position)
            event.accept()

    def log_msg(self, text, status="INFO"):
        time_str = datetime.now().strftime("%H:%M:%S")
        color = "#38bdf8"
        if status == "OK":
            color = "#34d399"
        elif status == "ERR":
            color = "#f87171"

        html = f"<span style='color:#64748b;'>[{time_str}]</span> <span style='color:{color}; font-weight:bold;'>[{status}]</span> {text}"
        self.terminal.append(html)

    def select_folder(self):
        folder = QFileDialog.getExistingDirectory(self, "Directory")
        if folder:
            self.folder_path = folder
            self.lbl_path.setText(f"📁 {os.path.basename(folder)} ({folder})")
            self.lbl_path.setStyleSheet("color: #34d399; font-size: 11px; font-weight: bold;")
            self.btn_start.setEnabled(True)
            self.log_msg(f"{LANG[self.cur_lang]['loaded']}{folder}", "OK")

    def run_clean(self):
        opts = {
            "rm_comments": self.cb_comments.isChecked(),
            "rm_docs": self.cb_docs.isChecked(),
            "rm_symbols": self.cb_symbols.isChecked(),
            "rm_blanks": self.cb_blanks.isChecked(),
            "backup": self.cb_backup.isChecked()
        }
        t = LANG[self.cur_lang]
        self.log_msg(t["started"], "INFO")
        count = 0
        failed = 0

        for root, dirs, files in os.walk(self.folder_path):
            if any(bad in root for bad in ["venv", "__pycache__", ".git"]):
                continue
            for file in files:
                if file.endswith(".py") and file != os.path.basename(__file__):
                    path = os.path.join(root, file)
                    try:
                        with tokenize.open(path) as f:
                            code = f.read()
                        cleaned = clean_code(code, opts)
                        if code != cleaned:
                            if opts["backup"]:
                                with open(path + ".bak", "w", encoding="utf-8") as f:
                                    f.write(code)
                            with open(path, "w", encoding="utf-8") as f:
                                f.write(cleaned)
                            count += 1
                            self.log_msg(f"{t['opt']}{file}", "OK")
                    except Exception as e:
                        failed += 1
                        self.log_msg(f"{t['fail']}{file} ({e})", "ERR")

        self.log_msg(t["done"].format(count=count, err=failed), "INFO")

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = AppWindow()
    window.show()
    sys.exit(app.exec())