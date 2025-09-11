input_path = "ATM_15_path_l.txt"
output_path = "ATM_15_path_l_reversed.txt"

with open(input_path, "r", encoding="utf-8") as f:
    lines = f.readlines()

with open(output_path, "w", encoding="utf-8") as f:
    for line in reversed(lines):
        f.write(line.rstrip('\n') + '\n')
