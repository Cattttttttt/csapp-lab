movq %rsp, %rax # 0x401a06
movq %rax, %rdi # 0x4019a2
popq %rax # 0x4019cc
movl %eax, %edx # 0x4019dd
movl %edx, %ecx # 0x401a34
movl %ecx, %esi # 0x401a13
lea (%rdi, %rsi, 1), %rax # 0x4019d6
movq %rax, %rdi # 0x4019a2