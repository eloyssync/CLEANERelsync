# CLEANERelsync

CLEANERelsync is a desktop Python source code formatter and optimizer built with PyQt6. It cleans code artifacts, normalizes non-standard typography, strips comments and docstrings, and validates syntax integrity before writing changes to disk.

> **Note on Antivirus Detections:**
> Standalone Windows executables built with packaging tools may trigger 2-3 false positive detections on some antivirus engines. 
> The project is 100% open source. Check our [VirusTotal Report](https://www.virustotal.com/gui/file/e481c00001d40190faed8ba8a4a5374ad350ada2386ab63f395089db1174faa2).

---

## Features

* **Typography Normalization:** Converts non-ASCII em-dashes (`—`, `–`) and non-standard quotes (`«»`, `“”`) into standard characters inside string literals and comments.
* **Comment Stripping:** Removes inline comments (`#`) while preserving shebang lines and encoding declarations.
* **Docstring Removal:** Safely strips docstrings and inserts `pass` statements where needed to maintain valid Python AST blocks.
* **Line Compression:** Removes redundant vertical whitespace and empty lines.
* **Syntax Safety Check:** Runs `ast.parse` validation on processed code to prevent saving corrupted files.
* **Backup Creation:** Generates `.bak` files prior to file modification.
* **Multilingual Interface:** Supports English, Russian, and Estonian.

---

## Requirements

* Python 3.10+
* PyQt6

---

## Installation

```bash
git clone https://github.com/eloyssync/CLEANERelsync.git
cd CLEANERelsync
pip install PyQt6
