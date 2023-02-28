import struct

# AYYMIDI Protocol
# implementation: DRAFT V1.0

MFGID_AYYM_MSG = 0x7A

AYYM_CMD_WRITESEQ = 0x0
AYYM_CMD_WRITEREV = 0x1
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
    if len(accum) == 2:
        return bytes(regi_enc_pair(accum))
    elif len(accum) > 2:
        seq_fwd = all(n - p == 1 for (p, n) in zip(accum[0::2], accum[2::2]))
        seq_bwd = all(p - n == 1 for (p, n) in zip(accum[0::2], accum[2::2]))
        if seq_fwd:
            rslt = [
                (AYYM_CMD_WRITESEQ << 5) | ((accum[0] & 0xF) << 1), 
            ]
            for valu in accum[1::2]: # then repeatedly 2 bytes of top/bottom nibble of each sequential register
                rslt.append((valu & 0xF0) >> 4)
                rslt.append(valu & 0x0F)
            return bytes(rslt)
        elif seq_bwd:
            rslt = [
                (AYYM_CMD_WRITEREV << 5) | ((accum[0] & 0xF) << 1), 
            ]
            for valu in accum[1::2]: # then repeatedly 2 bytes of top/bottom nibble of each sequential register
                rslt.append((valu & 0xF0) >> 4)
                rslt.append(valu & 0x0F)
            return bytes(rslt)
        else:
            # No specific sequence, so just write in pairs
            rslt = []
            for regi, valu in zip(accum[0::2], accum[1::2]):
                rslt.extend(regi_enc_pair([regi, valu]))
            return bytes(rslt)


def wrap_sysex_payload_bytes_old(accum):
    rslt = []
    rslt.append(len(accum))
    i = 0
    topbits = 0x0
    for b in accum:
        rslt.append(b & 0x7F)
        topbits <<= 1
        topbits |= (b & 0x80) >> 7
        i += 1
        if i == 7: # every 7 bytes, add 1 byte that contains top bits of each byte (because midi range 00..7F)
            rslt.append(topbits)
            topbits = 0
            i = 0
    rslt.append(topbits)
    cksum = 0
    for b in accum:
        cksum += b
    cksum &= 0x7F
    rslt.append(cksum)
    return rslt
