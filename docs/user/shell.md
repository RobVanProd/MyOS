# Shell Interface

The MyOS Shell provides a command-line interface for interacting with the operating system. It offers a familiar environment similar to CMD or PowerShell for Windows users.

## Starting the Shell

The shell automatically starts when the system boots up, presenting you with a window titled "MyOS Shell". The shell prompt looks like this:

```
MyOS>
```

## Basic Usage

### Command Syntax

Commands in MyOS Shell follow this general syntax:
```
command [arguments]
```

For example:
```
copy source.txt destination.txt
```

### Command History

- Use the **Up** and **Down** arrow keys to navigate through previously entered commands
- Press **Enter** to execute a command
- Use **Backspace** to delete characters

## Built-in Commands

### File System Commands

| Command | Description | Usage |
|---------|-------------|-------|
| `cd` | Change directory | `cd [directory]` |
| `dir` | List directory contents | `dir [directory]` |
| `type` | Display file contents | `type <file>` |
| `copy` | Copy files | `copy <source> <destination>` |
| `del` | Delete files | `del <file>` |
| `mkdir` | Create directory | `mkdir <directory>` |
| `rmdir` | Remove directory | `rmdir <directory>` |

### System Commands

| Command | Description | Usage |
|---------|-------------|-------|
| `cls` | Clear screen | `cls` |
| `echo` | Display messages | `echo [message]` |
| `help` | Show available commands | `help` |
| `ver` | Show OS version | `ver` |
| `date` | Show/set date | `date [MM-DD-YYYY]` |
| `time` | Show/set time | `time [HH:MM:SS]` |

## Examples

1. Creating and navigating directories:
   ```
   mkdir documents
   cd documents
   dir
   ```

2. File operations:
   ```
   echo Hello World > test.txt
   type test.txt
   copy test.txt backup.txt
   del test.txt
   ```

3. System information:
   ```
   ver
   date
   time
   ```

## Window Management

The shell runs in a window that can be:
- Moved by dragging the title bar
- Resized by dragging the window edges
- Minimized/maximized using window controls

## Advanced Features

### Command Line Editing

- **Insert Mode**: Default mode for typing
- **Cursor Movement**: Left/Right arrow keys to move within the command
- **Line Clearing**: Ctrl+C to clear current line

### Error Handling

The shell provides clear error messages when:
- A command is not found
- Invalid arguments are provided
- Operations fail due to permissions or other issues

Example error messages:
```
Unknown command. Type 'help' for list of commands.
Usage: copy <source> <destination>
Error: File not found
```

## Configuration

The shell behavior can be customized through:
- Environment variables (coming soon)
- Shell configuration file (coming soon)

## Troubleshooting

Common issues and solutions:

1. **Command Not Found**
   - Check command spelling
   - Use `help` to see available commands
   - Ensure proper case sensitivity

2. **File Operation Failures**
   - Verify file paths
   - Check file permissions
   - Ensure sufficient disk space

3. **Window Issues**
   - Try closing and reopening the shell
   - Check for sufficient memory
   - Verify display driver status

## Future Enhancements

Planned features for future releases:
- Tab completion
- Command aliases
- Script execution
- Pipe and redirection operators
- Background process management
- Custom prompt configuration
- Command history persistence

## Technical Details

The shell is implemented in:
- `src/apps/shell.c`: Main shell implementation
- `src/apps/shell.h`: Shell interface definitions
- `src/kernel/command.c`: Command system implementation
- `src/kernel/command.h`: Command system interface

Key components:
1. Command Parser
2. History Manager
3. Window Manager Integration
4. File System Interface
5. Input/Output Handler

## See Also

- [File System Documentation](../core/filesystem.md)
- [Window System Documentation](../core/window.md)
- [Process Management](../core/process.md)
- [User Guide](README.md)
