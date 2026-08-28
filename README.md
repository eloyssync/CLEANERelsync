# CLEANERelsync

CLEANERelsync is a high-performance desktop source code formatter and optimizer built with native C++ and Win32 API. It cleans code artifacts, normalizes non-standard typography, strips comments and docstrings, and validates syntax integrity before writing changes to disk.

---

## Features

* **Typography Normalization:** Converts non-ASCII em-dashes (`—`, `–`) and non-standard quotes (`«»`, `“”`) into standard characters inside string literals and comments.
* **Comment Stripping:** Removes inline comments while preserving essential compiler directives, shebang lines, and encoding declarations.
* **Docstring & Block Cleanup:** Safely strips multiline docstrings and blocks while preserving overall structural integrity.
* **Line Compression:** Removes redundant vertical whitespace and empty lines.
* **Syntax Safety Check:** Validates source syntax prior to saving to prevent file corruption.
* **Backup Creation:** Generates `.bak` files prior to file modification.
* **Multilingual Interface:** Supports English, Russian, and Estonian.

[Download CLEANERelsync v2.0.0](https://github.com/eloyssync/CLEANERelsync/releases/tag/v2.0.0)


---

## Requirements

* Windows 10 / 11 (x64)
* Microsoft Visual Studio 2022 (MSVC v143+) / C++20 standard

---

## Build & Installation

1. **Clone the repository:**
```bash
git clone [https://github.com/eloyssync/CLEANERelsync.git](https://github.com/eloyssync/CLEANERelsync.git)
cd CLEANERelsync
.
