import argparse
import re
import pandas as pd
import os

def parse_file(input_file):
    with open(input_file, 'r') as file:
        lines = file.readlines()

    # Initialize variables
    problem_name = ""
    encoding_type = ""
    rows = []
    total_real_time = ""
    total_solve_time = ""
    total_space = ""

    for line in lines:
        line = line.strip()
        
        if line.startswith("[runlim] argv["):
            args_match = re.search(r"argv\[(\d+)\]:", line)
            args_value = int(args_match.group(1))
            
            if (args_value == 1):
                problem_dir = line.split(":")[1].strip()
                problem_name = os.path.basename(problem_dir)
            elif (args_value > 1):
                encoding_type += line.split(":")[1].strip() + " "
            
        if line.startswith("c LB-w ="):
            lb_match = re.search(r"\s*LB-w\s*=\s*(\d+)\s*", line)
            lb_value = int(lb_match.group(1))
        
        if line.startswith("c UB-w ="):
            ub_match = re.search(r"\s*UB-w\s*=\s*(\d+)\s*", line)
            ub_value = int(ub_match.group(1))
        
        # Extract section-specific information
        elif line.startswith("c Initializing a "):
            n_match = re.search(r"n\s*=\s*(\d+)", line)
            n_value = n_match.group(1) if n_match else None

        elif line.startswith("c Encoding starts with w"):
            w_match = re.search(r"w\s*=\s*(\d+)", line)
            w_value = w_match.group(1) if w_match else None
            
        elif (line.startswith("c	Number of clauses:")):
            num_clauses = line.split(":")[1].strip()
            
        elif (line.startswith("c	Number of variables:")):
            num_vars = line.split(":")[1].strip()
            
        elif (line.startswith("c	Solving duration:")):
            solve_time = line.split(":")[1].strip()

        elif line.startswith("s"):
            result_match = re.search(r"s\s+(SAT|UNSAT)", line)
            result = result_match.group(1) if result_match else None

            if problem_name and encoding_type and n_value and w_value:
                rows.append([problem_name, encoding_type, lb_value, ub_value, n_value, w_value, num_clauses, num_vars, result, solve_time])
                
        elif line.startswith("[runlim] status:"):
            if ("out of time" in line):
                result = "TO"
                solve_time = "-"
                if problem_name and encoding_type and n_value and w_value:
                    rows.append([problem_name, encoding_type, lb_value, ub_value, n_value, w_value, num_clauses, num_vars, result, solve_time])
            elif ("out of memory" in line):
                result = "MO"
                solve_time = "-"
                num_clauses = "-"
                num_vars = "-"
                if problem_name and encoding_type and n_value and w_value:
                    rows.append([problem_name, encoding_type, lb_value, ub_value, n_value, w_value, num_clauses, num_vars, result, solve_time])
        elif line.startswith("[runlim] real:"):
            total_real_time = line.split(":")[1].strip()
        elif line.startswith("[runlim] time:"):
            total_solve_time = line.split(":")[1].strip()
        elif line.startswith("[runlim] space:"):
            total_space = line.split(":")[1].strip()
    
    for row in rows:
        row.append(total_real_time)
        row.append(total_solve_time)
        row.append(total_space)
        
    print(rows)
    return rows

def save_to_excel(data, output_file):
    columns_title = ["Problem Name", "Encoding Type", "LB", "UB", "n", "w", "Number clauses", "Number variables", "Result", "Solve time", "Total real time", "Total solve time", "Total space"]
    
    if (os.path.exists(output_file)):
        df_existing = pd.read_excel(output_file)
        new_data = pd.DataFrame([columns_title], columns=df_existing.columns)
        df_updated = pd.concat([df_existing, new_data], ignore_index=True)
        df_updated.to_excel(output_file, index=False)
    else:
        df = pd.DataFrame(data, columns=columns_title)
        df.to_excel(output_file, index=False)

def process_directories(directories, output_file):
    all_rows = []

    # Loop through each directory and parse all files
    for directory in directories:
        if os.path.isdir(directory):
            for filename in sorted(os.listdir(directory)):
                file_path = os.path.join(directory, filename)
                if os.path.isfile(file_path):
                    print(f"Processing file: {file_path}")
                    rows = parse_file(file_path)
                    all_rows.extend(rows)
        else:
            print(f"{directory} is not a valid directory.")
    
    # Save all collected data to Excel
    save_to_excel(all_rows, output_file)

def main():
    parser = argparse.ArgumentParser(description="Process runlim output files from multiple directories and save to Excel.")
    parser.add_argument("directories", nargs='+', help="List of directories containing the runlim files")
    parser.add_argument("output_file", help="Path to save the output Excel file")
    args = parser.parse_args()

    # Process all files in the given directories and save to Excel
    process_directories(args.directories, args.output_file)

if __name__ == "__main__":
    main()
