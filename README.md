# CLEANERelsync

CLEANERelsync is a desktop Python source code formatter and optimizer built with PyQt6[cite: 1, 3]. It cleans code artifacts, normalizes non-standard typography, strips comments and docstrings, and validates syntax integrity before writing changes to disk.

---

## Features

* **Typography Normalization:** Converts non-ASCII em-dashes (`—`, `–`) and non-standard quotes (`«»`, `“”`) into standard characters inside string literals and comments.
* **Comment Stripping:** Removes inline comments (`#`) while preserving shebang lines and encoding declarations.
* **Docstring Removal:** Safely strips docstrings and inserts `pass` statements where needed to maintain valid Python AST blocks[cite: 1].
* **Line Compression:** Removes redundant vertical whitespace and empty lines[cite: 1].
* **Syntax Safety Check:** Runs `ast.parse` validation on processed code to prevent saving corrupted files[cite: 1].
* **Backup Creation:** Generates `.bak` files prior to file modification[cite: 1].
* **Multilingual Interface:** Supports English, Russian, and Estonian[cite: 1].

---

## Requirements

* Python 3.10+
* PyQt6[cite: 1]

---

## Installation

```bash
git clone [https://github.com/eloyssync/CLEANERelsync.git](https://github.com/eloyssync/CLEANERelsync.git)
cd CLEANERelsync
pip install PyQt6
Usage
Run the application:

Bash
python CLEANERelsync.py
Select the target directory[cite: 1].

Select optimization options[cite: 1].

Click START OPTIMIZATION[cite: 1].

License
MIT License