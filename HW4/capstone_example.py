# test1.py
from capstone import *
import binascii as b

# CODE = b"\x55\x48\x8b\x05\xb8\x13\x00\x00"
# CODE = 'fc9048f7e7f9fd'.encode('utf-8')
CODE = '52f94889c748ffce4885f6'.encode('utf-8')
CODE = b.a2b_hex(CODE)
# print(CODE)

res = ''

# r = b.a2b_hex('52f94889c748ffce4885f6')
# print(r)
# disasm(r)

md = Cs(CS_ARCH_X86, CS_MODE_64)
for i in md.disasm(CODE, 0x1000):
    res += "%s %s\n" %(i.mnemonic, i.op_str)
print(res)


hex_res = res.encode('utf-8')
final_res = b.hexlify(hex_res).decode('utf-8')
print(final_res)