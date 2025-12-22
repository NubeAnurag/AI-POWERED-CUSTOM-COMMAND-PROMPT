# 🌟 AI-POWERED CUSTOM COMMAND PROMPT

⚡ **Neon Command Interface** - An intelligent, AI-powered shell with beautiful neon aesthetics and smart command completion.

![Version](https://img.shields.io/badge/version-1.0-brightgreen)
![Language](https://img.shields.io/badge/language-C-blue)
![AI](https://img.shields.io/badge/AI-Ollama-purple)
![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux-lightgrey)

---

## ✨ Features

### 🎨 Beautiful Neon Interface
- **Quirky neon color scheme** with black background
- **Two-line visual prompt** with directory path display
- **Color-coded command output** for better readability
- **Professional box-drawing characters** for UI elements

### 🤖 Intelligent AI Completion
- **Deterministic built-in command help** - No hallucinations for shell built-ins
- **PATH-based external command completion** - Suggests executables from your system
- **Context-aware suggestions** - Understands partial commands (e.g., `ver` → `version`)
- **Command-specific flags** - Smart suggestions for tools like `gcc`, `git`, etc.
- **Ollama integration** - AI-powered descriptions for unfamiliar commands

### ⚡ Smart Features
- **TAB completion** - Press TAB for instant suggestions
- **Auto-completion** - Unique matches auto-fill automatically
- **Multi-match handling** - Shows list when multiple commands match
- **Backspace support** - Full editing capabilities
- **Command history** - Track your command usage
- **Built-in calculator** - Quick math operations
- **File operations** - ls, cat, tree, find, count, mkdir, touch, rm

---

## 🛠️ Built-in Commands

| Command | Description |
|---------|-------------|
| `cd` | Change directory with smart path suggestions |
| `help` | List all available commands |
| `version` | Show shell version |
| `calc` | Calculator (e.g., `calc 10 + 5`) |
| `datetime` | Display current date and time |
| `ls` | List directory contents |
| `pwd` | Print working directory |
| `cat` | Display file contents |
| `tree` | Show directory tree structure |
| `find` | Find files by pattern |
| `count` | Count files and directories |
| `mkdir` | Create directory |
| `touch` | Create file |
| `rm` | Remove file |
| `whoami` | Show current user |
| `clear` | Clear screen |
| `echo` | Print text |
| `history` | Show command history |
| `exit` | Exit shell |

---

## 📋 Requirements

### System Requirements
- **OS**: macOS or Linux
- **Compiler**: gcc or clang
- **Libraries**: 
  - libcurl
  - json-c

### AI Requirements
- **Ollama** installed and running
- **tinyllama** model (or any compatible model)

---

## 🚀 Installation

### 1. Install Dependencies

**macOS (Homebrew):**
```bash
brew install curl json-c
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get install libcurl4-openssl-dev libjson-c-dev
```

### 2. Install Ollama
```bash
curl -fsSL https://ollama.com/install.sh | sh
ollama serve
ollama pull tinyllama
```

### 3. Clone and Build
```bash
git clone https://github.com/NubeAnurag/AI-POWERED-CUSTOM-COMMAND-PROMPT.git
cd AI-POWERED-CUSTOM-COMMAND-PROMPT
make
```

---

## 🎮 Usage

### Start the Shell
```bash
./shell2_complete_ai
```

### Try These Examples

**1. Basic Commands:**
```bash
help          # Show all commands
version       # Show shell version
ls            # List files
pwd           # Current directory
```

**2. Calculator:**
```bash
calc 10 + 5   # Returns: 15
calc 2 ^ 8    # Returns: 256
```

**3. File Operations:**
```bash
tree          # Show directory tree
find "*.c"    # Find all C files
count         # Count files in current dir
cat README.md # Display file
```

**4. TAB Completion:**
- Type `ver` and press **TAB** → auto-completes to `version` + shows help
- Type `c` and press **TAB** → shows: cd, calc, cat, count, clear
- Type `gcc -W` and press **TAB** → shows: -Wall, -Wextra, -Werror

**5. External Commands:**
```bash
gc<TAB>       # Completes to gcc, shows usage
git<TAB>      # Shows git suggestions
python<TAB>   # Shows python help
```

---

## 🎨 Color Scheme

| Element | Color | ANSI Code |
|---------|-------|-----------|
| Borders/Frames | Bright Magenta | `\033[1;95m` |
| Highlights | Bright Cyan | `\033[1;96m` |
| Command Input | Bright Green | `\033[1;92m` |
| Titles | Bright Yellow | `\033[1;93m` |
| Tips | Gray | `\033[1;90m` |
| Background | Black | `\033[40m` |

---

## 📁 Project Structure

```
AI-POWERED-CUSTOM-COMMAND-PROMPT/
├── shell2_complete.c       # Main shell implementation
├── ollama_integration.c    # AI integration logic
├── ollama_integration.h    # Header file
├── test_ollama.c           # Test file for Ollama
├── test_ollama_direct.c    # Direct Ollama test
├── Makefile                # Build configuration
├── RUN_COMMANDS.md         # Quick reference guide
├── .gitignore              # Git ignore rules
└── README.md               # This file
```

---

## 🔧 How It Works

### 1. Command Input
- User types command and presses TAB
- Shell enters raw mode for character-by-character processing
- Backspace, enter, and special keys handled properly

### 2. Completion Flow
```
User Input → Check Built-ins → Check PATH → Ask Ollama → Display Results
```

### 3. Built-in Command Help
- Deterministic help database (no AI hallucinations)
- Structured format: usage, examples, related commands
- Instant response, no network required

### 4. External Command Completion
- Scans directories in your PATH
- Finds executables matching partial input
- Picks "best match" (shortest, no version numbers)
- Auto-completes if only one match

### 5. Ollama Integration
- Used only as fallback for unknown commands
- Generates contextual descriptions
- Provides usage examples
- Respects --help output for accuracy

---

## 🐛 Troubleshooting

### Ollama Connection Issues
```bash
# Check if Ollama is running
pgrep -f ollama

# Start Ollama
ollama serve

# Check if model is installed
ollama list

# Pull model if missing
ollama pull tinyllama
```

### Compilation Errors
```bash
# Check if libraries are installed
pkg-config --exists libcurl && echo "curl: OK"
pkg-config --exists json-c && echo "json-c: OK"

# Rebuild from scratch
make clean
make
```

### Terminal Display Issues
- Make sure your terminal supports ANSI color codes
- Try a different terminal (iTerm2, Terminal.app, gnome-terminal)

---

## 🤝 Contributing

Contributions welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

---

## 📝 License

This project is open source and available under the MIT License.

---

## 👥 Authors

- **Anurag Mandal** - [NubeAnurag](https://github.com/NubeAnurag)

---

## 🙏 Acknowledgments

- **Ollama** - For the local AI model infrastructure
- **TinyLlama** - For the efficient language model
- Original shell concepts from various Unix shell implementations

---

## 📊 Stats

- **Lines of Code**: ~2,500
- **Built-in Commands**: 20
- **External Command Support**: ✓
- **AI Suggestions**: ✓
- **Color Themes**: Neon + Black

---

## 🔗 Links

- **Repository**: https://github.com/NubeAnurag/AI-POWERED-CUSTOM-COMMAND-PROMPT
- **Issues**: https://github.com/NubeAnurag/AI-POWERED-CUSTOM-COMMAND-PROMPT/issues
- **Ollama**: https://ollama.com

---

Made with ⚡ and 🎨 by [NubeAnurag](https://github.com/NubeAnurag)

