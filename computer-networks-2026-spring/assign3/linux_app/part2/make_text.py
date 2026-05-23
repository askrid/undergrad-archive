TARGET_SIZE = 1024*1024 # can configure any target size 
OUTPUT_FILE = "1mb.txt" # write name of file you want to create

line = "This is a sample line for generating a 1MB text file.\n"

written = 0
with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
    while written + len(line.encode("utf-8")) <= TARGET_SIZE:
        f.write(line)
        written += len(line.encode("utf-8"))

    # Fill any remaining bytes
    remaining = TARGET_SIZE - written
    if remaining > 0:
        f.write("a" * remaining)

print(f"Created {OUTPUT_FILE} with size {TARGET_SIZE} bytes")