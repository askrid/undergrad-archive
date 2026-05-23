TARGET_SIZE = 10*1024*1024
OUTPUT_FILE = "10mb.txt"

line = "This is a sample line.\n"

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