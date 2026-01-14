# C++ Development Environment - Setup Guide

## 🎯 Quick Start

Your C++ development environment is **fully configured** and ready to use!

### Instant Build + Run
1. Open any `.cpp` file in VS Code
2. Press **F5** → Code compiles and runs automatically
3. See output in the integrated terminal

### Alternative Shortcuts
- **Cmd+Shift+B**: Build only (compile without running)
- **Ctrl+Shift+P** → "Tasks: Run Task" → "Clean Build": Remove all binaries

---

## 📁 Workspace Structure

```
CPP_Workspace/
├── .vscode/              # VS Code configuration (auto-configured)
│   ├── tasks.json        # Build tasks
│   ├── launch.json       # F5 debug/run configuration
│   ├── c_cpp_properties.json  # IntelliSense settings
│   └── settings.json     # Editor preferences
├── bin/                  # Compiled executables (auto-generated)
├── main.cpp              # Your Hello World test file
├── docs/                 # Documentation
└── reference/            # C++ learning resources
```

---

## 🔧 Compiler Configuration

**Compiler**: Apple Clang++ 17.0.0  
**Location**: `/usr/bin/clang++`  
**Standard**: C++17 (configurable)

### Compilation Flags (Automatically Applied)
- `-std=c++17`: Use C++17 standard
- `-Wall -Wextra`: Enable all common warnings
- `-O2`: Level 2 optimization (balanced speed/size)
- `-g`: Include debug symbols for troubleshooting

---

## 🚀 Usage Workflow

### Creating a New Program
1. Create a new `.cpp` file in `CPP_Workspace`
2. Write your code
3. Press **F5** to build and run
4. Binary appears in `bin/` folder with the same name as your file

### Example Session
```bash
# You write: my_program.cpp
# You press: F5
# Result: bin/my_program (executable)
```

---

## 🛠️ Customization

### Change C++ Standard
Edit `.vscode/tasks.json` and modify the `-std=` flag:
- C++11: `-std=c++11`
- C++14: `-std=c++14`
- C++17: `-std=c++17` (current)
- C++20: `-std=c++20`
- C++23: `-std=c++23`

### Adjust Optimization Level
In `tasks.json`, change the `-O` flag:
- `-O0`: No optimization (fastest compile)
- `-O1`: Basic optimization
- `-O2`: Moderate optimization (default)
- `-O3`: Aggressive optimization (best performance)

### Add External Libraries
Example: Adding math library explicitly
```json
"args": [
    "-std=c++17",
    "-Wall",
    "-Wextra",
    "-O2",
    "-g",
    "${file}",
    "-o",
    "${fileDirname}/bin/${fileBasenameNoExtension}",
    "-lm"  // Add math library
]
```

---

## 🐛 Troubleshooting

### Issue: "Command not found: clang++"
**Solution**: Install Xcode Command Line Tools
```bash
xcode-select --install
```

### Issue: "No such file or directory: bin/"
**Solution**: The `bin/` folder is auto-created on first compile. If missing:
```bash
mkdir -p ~/Documents/CPP_Workspace/bin
```

### Issue: F5 doesn't work
**Solution**: 
1. Make sure a `.cpp` file is open and active
2. Check that the C/C++ extension is installed in VS Code
3. Reload VS Code: Cmd+Shift+P → "Developer: Reload Window"

### Issue: IntelliSense not working
**Solution**: Install the official C/C++ extension:
1. Open Extensions (Cmd+Shift+X)
2. Search for "C/C++" by Microsoft
3. Click Install
4. Reload VS Code

---

## 📦 Required VS Code Extensions

### Essential
- **C/C++** (ms-vscode.cpptools) - Language support and IntelliSense
- **CodeLLDB** (vadimcn.vscode-lldb) - Debugging support for macOS

### Optional but Recommended
- **C/C++ Themes** (ms-vscode.cpptools-themes) - Syntax highlighting
- **Error Lens** (usernamehw.errorlens) - Inline error messages
- **Better Comments** (aaron-bond.better-comments) - Enhanced comment styling

Install extensions via Terminal:
```bash
code --install-extension ms-vscode.cpptools
code --install-extension vadimcn.vscode-lldb
```

---

## 🎓 Learning Resources

Check the `reference/` folder for:
- **CPP_BASICS.md**: Syntax fundamentals
- **FUNCTIONS_REFERENCE.md**: Functions, classes, templates
- **NUMERICAL_PHYSICS.md**: Physics simulation techniques
- **OPTIMIZATION_TIPS.md**: Performance best practices

---

## 💡 Pro Tips

1. **Multiple Files**: For projects with multiple source files, create a `Makefile` or use CMake
2. **Header Files**: Store `.h` files in an `include/` directory
3. **Version Control**: Initialize git for tracking changes: `git init`
4. **Fast Iteration**: Keep terminal visible to see errors immediately
5. **Code Formatting**: VS Code auto-formats on save (configured)

---

## 🔍 Verification Checklist

- [✓] Compiler installed (Clang++ 17.0.0)
- [✓] VS Code configured
- [✓] F5 builds and runs code
- [✓] IntelliSense active (autocomplete works)
- [✓] Error highlighting enabled
- [✓] Terminal output displays correctly

---

## 📞 Need Help?

1. Check `USAGE_GUIDE.md` for keyboard shortcuts
2. Review `reference/` folder for C++ syntax
3. Examine `.vscode/tasks.json` to understand build process
4. Test with `main.cpp` to verify setup

**Environment Status**: ✅ Fully Operational  
**Last Updated**: January 7, 2026
