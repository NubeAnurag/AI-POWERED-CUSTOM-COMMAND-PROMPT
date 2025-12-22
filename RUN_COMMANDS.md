# How to Run and Test the AI Shell Project

## Step-by-Step Commands

### 1. Navigate to the project directory
```bash
cd "/Users/anuragmandal/Desktop/ qualcomm ai command prompt/NubeAnurag/AI_automated_custom_shell"
```

### 2. Verify Ollama is running
```bash
ollama list
```
**Expected output:** Should show `tinyllama` model in the list

### 3. If Ollama is not running, start it:
```bash
ollama serve
```
*(Leave this terminal window open, or run in background)*

### 4. Build the project (if not already built)
```bash
make clean
make
```
**Expected output:** Should compile without errors

### 5. Run the shell
```bash
./shell2_complete_ai
```

---

## Testing Commands (run these INSIDE the shell)

Once the shell is running, try these commands one by one:

### Basic Commands:
```
help
```
*Lists all available built-in commands*

```
version
```
*Shows shell version*

```
pwd
```
*Shows current directory*

```
ls
```
*Lists files in current directory*

### Advanced Commands:
```
calc 10 + 5
```
*Calculator - should output: 15*

```
datetime
```
*Shows current date and time*

```
history
```
*Shows command history*

### File Operations:
```
mkdir test_folder
```
*Creates a test folder*

```
touch test_file.txt
```
*Creates a test file*

```
cat test_file.txt
```
*Displays file contents*

### AI Feature Test:
```
ls<TAB>
```
*Press TAB while typing to get AI suggestions*

```
cd<TAB>
```
*Press TAB after "cd" to get AI directory suggestions*

### Exit:
```
exit
```
*Exits the shell*

---

## Quick Test Script (Alternative)

To test multiple commands at once without manual input:

```bash
printf "help\nversion\npwd\nls\ncalc 10 + 5\ndatetime\nexit\n" | ./shell2_complete_ai
```

---

## Troubleshooting

If you get "command not found" errors:
```bash
# Check if you're in the right directory
pwd

# Check if executable exists
ls -la shell2_complete_ai

# Make sure Ollama is running
curl -s http://localhost:11434/api/tags | grep tinyllama
```

If Ollama connection fails:
```bash
# Start Ollama if not running
ollama serve

# Or check if it's already running
pgrep -f ollama
```

