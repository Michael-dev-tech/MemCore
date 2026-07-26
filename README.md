# Orbit

Orbit is a modern, privacy-focused Markdown notes and knowledge base application designed for desktop environments. Built with C++ and Qt6, it combines high-performance native execution with a clean, distraction-free user interface inspired by modern design principles.

---

## Features

* **Dual-Pane Editor & Preview:** Write raw Markdown on the left while instantly viewing a rendered preview on the right, housed within a sleek "liquid glass" visual structure.
* **Graph View:** Visualize connections between your notes dynamically with a physics-based interactive node map. Supports smooth zoom (`Ctrl` + scroll), smooth node hovering, and direct double-click navigation.
* **Local & Secure:** Complete data sovereignty. All notes are saved locally in a plain-text Markdown structure and encrypted using AES-256-CBC with secure initialization vectors.
* **Integrated Spellcheck:** Real-time spellchecking powered by Hunspell, ensuring your notes remain typo-free.
* **Command Palette:** Quickly navigate, run actions, or create files using a fast, keyboard-driven command interface (`Ctrl + P`).
* **Customizable Themes:** Switch seamlessly between Dark, Light, and Solarized Light modes to match your workflow preference.
* **Rich Markdown Support:** Handles internal `[[Wikilinks]]`, tags (`#tag`), task lists, code blocks, and asset management (drag-and-drop images directly into `.assets/`).

---

## Tech Stack

* **Language:** C++ (Standard 17)
* **Framework:** Qt6 (Widgets, GraphicsView)
* **Cryptography:** OpenSSL (`EVP_aes_256_cbc`)
* **Spellcheck:** Hunspell
* **Build System:** CMake

---

## Getting Started

### Prerequisites

To compile and run Orbit from source, ensure you have the following dependencies installed on your system:

* A C++17 compatible compiler (GCC / Clang)
* CMake (version 3.16 or higher)
* Qt6 Development Packages (`qt6-qtbase-devel` or equivalent)
* OpenSSL Development Libraries
* Hunspell and English dictionaries (`hunspell-devel`, `hunspell-en`)

### Installation & Compilation

1. Clone the repository:
```bash
git clone https://github.com/your-username/Orbit.git
cd Orbit

```


2. Create a build directory and configure the project with CMake:
```bash
mkdir build && cd build
cmake ..

```


3. Compile the application:
```bash
make -j$(nproc)

```


4. Run the executable:
```bash
./MemCore

```



---

## Keyboard Shortcuts

* **`Ctrl + N`**: Create a new note
* **`Ctrl + P`**: Open the Command Palette
* **`Ctrl + B`**: Bold selected text
* **`Ctrl + I`**: Italicize selected text
* **`Ctrl + Scroll`**: Zoom in/out within the Graph View
* **`Delete` / `F2**`: Delete or rename selected notes in the file tree (via context menu or shortcuts)

---

## License

This project is open-source and available under the MIT License.
