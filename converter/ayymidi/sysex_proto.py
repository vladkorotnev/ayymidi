import struct

# AYYMIDI Protocol
# implementation: DRAFT V1.0

MFGID_AYYM_MSG = 0x7A

#reserved = 0x0
#reserved = 0x1
AYYM_CMD_WRITEPAIR = 0x2
AYYM_CMD_SETCLOCK = 0x3

def clock_to_sysex_payload(rate, is_acb):
    clock = struct.pack("<I", rate)
    return bytes([
        # 2 bits CMD, then 4 MSBits of next 4 bytes, then 1 bit for ACB instead of ABC
        (AYYM_CMD_SETCLOCK << 5) | 
            ((clock[0] & 0x80) >> 3) |
            ((clock[1] & 0x80) >> 4) |
            ((clock[2] & 0x80) >> 5) |
            ((clock[3] & 0x80) >> 6) | 
            (0x1 if is_acb else 0x0),
        # Then 4 bytes AND 7F
        clock[0] & 0x7F,
        clock[1] & 0x7F,
        clock[2] & 0x7F,
        clock[3] & 0x7F,
    ])

def regi_enc_pair(accum):
        return [
            (AYYM_CMD_WRITEPAIR << 5) | ((accum[0] & 0xF) << 1) | ((accum[1] & 0x80) >> 7), 
            (accum[1] & 0x7F)
        ]
def regi_accum_to_sysex_payload(accum):
    rslt = []
    for regi, valu in zip(accum[0::2], accum[1::2]):
        rslt.extend(regi_enc_pair([regi, valu]))
    return bytes(rslt)
