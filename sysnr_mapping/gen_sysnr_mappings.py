import sys
import re
import platform

if len(sys.argv) != 2:
    print('Usage: %s <path to syscall header file>\nYou should provide the header that directly lists the syscall numbers, so not headers that incldue other headers which have the syscall numbers in them' % sys.argv[0])
    sys.exit(1)

syscall_extractor = r'^#define __NR_(\w+) (\d+)$'
syscall_table = []
with open(sys.argv[1], 'r') as infile:
    lines = infile.readlines()
    for ln in lines:
        match = re.match(syscall_extractor, ln)
        if not match: continue
        syscall_table.append((int(match.group(2)), match.group(1)))

syscall_table = sorted(syscall_table)
print(f'/* Generated from {sys.argv[1]} ({platform.release()}) */')
print('#pragma once')
print(f'#define N_SYSCALLS {syscall_table[-1][0] + 1}')
print('const char *sysnr_map[] = {')
prev_nr = -1
for syscall in syscall_table:
    while prev_nr + 1 != syscall[0]:
        print('\"Unknown Syscall!\",')
        prev_nr += 1

    print(f'"{syscall[1]}",')
    prev_nr += 1

print("};")
