from pwn import *
from capstone import *
import binascii as b

conn = remote('aup.zoolab.org', 2530)

for i in range(10):
    server_output = conn.recvuntil('Your answer:').decode('utf-8').split()
    # print(server_output[-3])

    CODE = server_output[-3]
    CODE = CODE.encode('utf-8')
    CODE = b.a2b_hex(CODE)

    res = ''

    md = Cs(CS_ARCH_X86, CS_MODE_64)
    for i in md.disasm(CODE, 0x1000):
        res += "%s %s\n" %(i.mnemonic, i.op_str)

    hex_res = res.encode('utf-8')
    final_res = b.hexlify(hex_res).decode('utf-8')

    conn.sendline(final_res)
conn.interactive()