# tinyshell v1.3.6

A lightweight, custom Linux shell written in C, featuring smart command parsing, persistent configuration, and an interactive UI.


![preview](/images/tinyshell.jpg)

## 🚀 Features

- **Smart Parsing**: Supports commands and paths with spaces using single (`'`) or double (`"`) quotes (e.g., `cd "Epic Games"`).
- **Tab Completion**: Interactive auto-completion for:
  - Built-in commands.
  - Custom aliases.
  - Folder shortcuts.
  - System files and directories.
- **Custom Configuration**: External `tinyshell.conf` file to define your own aliases and quick-jump folder shortcuts.
- **Output Redirection**: Save command output to files using the `>` symbol.
- **Interactive UI**: 
  - Fancy colored prompt: `tinyshell:[path] :`.
  - Dark grey background theme.
  - Command history using Up/Down arrows.
- **Built-in Commands**: Optimized internal implementation for `cd`, `mkdir`, `rmdir`, `cp`, `mv`, `cat`, `cls`, and `reload`.

## 🛠️ Installation & Setup

### Prerequisites
You need the `GNU Readline` library installed on your system.

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install libreadline-dev
```
