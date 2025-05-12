import re

def split_commands(log_text):
    pattern = r"Run command:(.*?Command executed successfully:)(\s*(\S+))"  # Pattern to match command blocks including trailing text
    matches = re.findall(pattern, log_text, re.DOTALL)  # Extract all command blocks
    return [(match[0].strip() + ' ' + match[1].strip(), match[2].strip().replace('.', '')) for match in matches]  # Return tuple (command block, filename)

def read_log_file(file_path):
    with open(file_path, 'r', encoding='utf-8') as file:
        return file.read()

def write_commands_to_files(commands):
    for command, filename in commands:
        with open(f"{filename}.txt", "w", encoding="utf-8") as file:
            file.write(command)

# Example usage:
log_file_path = "temp.txt"  # Change this to your file path
log_text = read_log_file(log_file_path)
commands = split_commands(log_text)
write_commands_to_files(commands)

for i, (command, filename) in enumerate(commands, 1):
    print(f"Command {i} saved as {filename}.txt")