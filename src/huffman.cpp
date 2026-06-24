#include "huffman.h"
#include <cmath>
#include <cstdint>

static const uint8_t DC_LUMA_BITS[16] = {
    0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0
};
static const uint8_t DC_LUMA_VALS[] = {
    0,1,2,3,4,5,6,7,8,9,10,11
};

static const uint8_t DC_CHROMA_BITS[16] = {
    0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0
};
static const uint8_t DC_CHROMA_VALS[] = {
    0,1,2,3,4,5,6,7,8,9,10,11
};

static const uint8_t AC_LUMA_BITS[16] = {
    0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,125
};
static const uint8_t AC_LUMA_VALS[] = {
    0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,
    0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,
    0x22,0x71,0x14,0x32,0x81,0x91,0xa1,0x08,
    0x23,0x42,0xb1,0xc1,0x15,0x52,0xd1,0xf0,
    0x24,0x33,0x62,0x72,0x82,0x09,0x0a,0x16,
    0x17,0x18,0x19,0x1a,0x25,0x26,0x27,0x28,
    0x29,0x2a,0x34,0x35,0x36,0x37,0x38,0x39,
    0x3a,0x43,0x44,0x45,0x46,0x47,0x48,0x49,
    0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,
    0x5a,0x63,0x64,0x65,0x66,0x67,0x68,0x69,
    0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,
    0x7a,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
    0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,
    0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,
    0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,
    0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,
    0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,0xd4,
    0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe1,0xe2,
    0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,
    0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,
    0xf9,0xfa
};

static const uint8_t AC_CHROMA_BITS[16] = {
    0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,119
};
static const uint8_t AC_CHROMA_VALS[] = {
    0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,
    0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,
    0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,
    0xa1,0xb1,0xc1,0x09,0x23,0x33,0x52,0xf0,
    0x15,0x62,0x72,0xd1,0x0a,0x16,0x24,0x34,
    0xe1,0x25,0xf1,0x17,0x18,0x19,0x1a,0x26,
    0x27,0x28,0x29,0x2a,0x35,0x36,0x37,0x38,
    0x39,0x3a,0x43,0x44,0x45,0x46,0x47,0x48,
    0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,
    0x59,0x5a,0x63,0x64,0x65,0x66,0x67,0x68,
    0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,
    0x79,0x7a,0x82,0x83,0x84,0x85,0x86,0x87,
    0x88,0x89,0x8a,0x92,0x93,0x94,0x95,0x96,
    0x97,0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,
    0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,
    0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,
    0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,
    0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,
    0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,
    0xea,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,
    0xf9,0xfa
};

struct HuffTable {
    uint32_t code[256];
    uint8_t  size[256];
};

static HuffTable buildHuffTable(const uint8_t bits[16], const uint8_t* vals) {
    HuffTable ht{};
    uint32_t code = 0;
    int idx = 0;
    for (int len = 1; len <= 16; len++) {
        for (int i = 0; i < bits[len - 1]; i++) {
            ht.code[vals[idx]] = code;
            ht.size[vals[idx]] = len;
            code++;
            idx++;
        }
        code <<= 1;
    }
    return ht;
}

struct HuffDecodeTable {
    uint32_t codes[16][256];
    uint8_t  syms [16][256];
    int      count[16];

    void build(const uint8_t bits[16], const uint8_t* vals) {
        uint32_t code = 0;
        int idx = 0;
        for (int len = 0; len < 16; len++) {
            count[len] = bits[len];
            for (int i = 0; i < bits[len]; i++) {
                codes[len][i] = code;
                syms [len][i] = vals[idx++];
                code++;
            }
            code <<= 1;
        }
    }

    int decode(struct BitReader& br) const;
};

static int category(int val) {
    if (val == 0) return 0;
    val = std::abs(val);
    int cat = 0;
    while (val > 0) { val >>= 1; cat++; }
    return cat;
}

static uint32_t vliEncode(int val) {
    if (val < 0) val = val - 1;
    return static_cast<uint32_t>(val) & 0xFFFF;
}

static int vliDecode(int cat, uint32_t bits) {
    if (cat == 0) return 0;
    int threshold = 1 << (cat - 1);
    if (static_cast<int>(bits) >= threshold)
        return static_cast<int>(bits);
    else
        return static_cast<int>(bits) - (2 * threshold - 1);
}

struct BitWriter {
    std::vector<uint8_t>& out;
    uint32_t buf  = 0;
    int      bits = 0;

    void write(uint32_t code, int len) {
        buf  = (buf << len) | (code & ((1u << len) - 1));
        bits += len;
        while (bits >= 8) {
            bits -= 8;
            uint8_t byte = (buf >> bits) & 0xFF;
            out.push_back(byte);
            if (byte == 0xFF) out.push_back(0x00);
        }
    }

    void flush() {
        if (bits > 0) {
            uint8_t byte = static_cast<uint8_t>((buf << (8 - bits)) & 0xFF);
            out.push_back(byte);
            if (byte == 0xFF) out.push_back(0x00);
        }
        buf = bits = 0;
    }
};

struct BitReader {
    const std::vector<uint8_t>& data;
    size_t   pos  = 0;
    uint32_t buf  = 0;
    int      bits = 0;

    uint8_t nextByte() {
        if (pos >= data.size()) return 0;
        uint8_t b = data[pos++];
        if (b == 0xFF && pos < data.size() && data[pos] == 0x00)
            pos++;
        return b;
    }

    uint32_t peek(int n) {
        while (bits < n) {
            buf = (buf << 8) | nextByte();
            bits += 8;
        }
        return (buf >> (bits - n)) & ((1u << n) - 1);
    }

    void consume(int n) { bits -= n; }

    uint32_t read(int n) {
        uint32_t val = peek(n);
        consume(n);
        return val;
    }
};

int HuffDecodeTable::decode(BitReader& br) const {
    for (int len = 0; len < 16; len++) {
        uint32_t candidate = br.peek(len + 1);
        for (int i = 0; i < count[len]; i++) {
            if (codes[len][i] == candidate) {
                br.consume(len + 1);
                return syms[len][i];
            }
        }
    }
    return -1;
}

static void encodeBlock(
    const int*       block,
    int&             prevDC,
    const HuffTable& dcTab,
    const HuffTable& acTab,
    BitWriter&       bw)
{
    int dcDiff = block[0] - prevDC;
    prevDC = block[0];

    int cat = category(dcDiff);
    bw.write(dcTab.code[cat], dcTab.size[cat]);
    if (cat > 0) bw.write(vliEncode(dcDiff), cat);

    int zeros = 0;
    for (int i = 1; i < 64; i++) {
        int val = block[i];

        if (val == 0) { zeros++; continue; }

        while (zeros >= 16) {
            bw.write(acTab.code[0xF0], acTab.size[0xF0]);
            zeros -= 16;
        }

        int s = category(val);
        uint8_t sym = static_cast<uint8_t>((zeros << 4) | s);
        bw.write(acTab.code[sym], acTab.size[sym]);
        bw.write(vliEncode(val), s);
        zeros = 0;
    }

    bw.write(acTab.code[0x00], acTab.size[0x00]);
}

static void decodeBlock(
    int*                   block,
    int&                   prevDC,
    const HuffDecodeTable& dcTab,
    const HuffDecodeTable& acTab,
    BitReader&             br)
{
    int cat = dcTab.decode(br);
    int dcDiff = (cat > 0) ? vliDecode(cat, br.read(cat)) : 0;
    prevDC  += dcDiff;
    block[0] = prevDC;

    int i = 1;
    while (i < 64) {
        int sym = acTab.decode(br);
        if (sym == 0x00) {
            while (i < 64) block[i++] = 0;
            break;
        }
        if (sym == 0xF0) {
            for (int z = 0; z < 16 && i < 64; z++) block[i++] = 0;
            continue;
        }
        int run  = (sym >> 4) & 0xF;
        int size =  sym       & 0xF;
        for (int z = 0; z < run && i < 64; z++) block[i++] = 0;
        if (i < 64) block[i++] = vliDecode(size, br.read(size));
    }
}

std::vector<uint8_t> huffman::encode(
    const std::vector<int>& yQuant,
    const std::vector<int>& cbQuant,
    const std::vector<int>& crQuant,
    int width, int height)
{
    const int chromaWidth  = (width  / 2 / 8) * 8;
    const int chromaHeight = (height / 2 / 8) * 8;

    HuffTable dcLuma   = buildHuffTable(DC_LUMA_BITS,   DC_LUMA_VALS);
    HuffTable acLuma   = buildHuffTable(AC_LUMA_BITS,   AC_LUMA_VALS);
    HuffTable dcChroma = buildHuffTable(DC_CHROMA_BITS, DC_CHROMA_VALS);
    HuffTable acChroma = buildHuffTable(AC_CHROMA_BITS, AC_CHROMA_VALS);

    std::vector<uint8_t> bitstream;
    BitWriter bw{bitstream};

    int prevDC = 0;
    int yBlocks = (width / 8) * (height / 8);
    for (int b = 0; b < yBlocks; b++)
        encodeBlock(yQuant.data() + b * 64, prevDC, dcLuma, acLuma, bw);

    prevDC = 0;
    int cBlocks = (chromaWidth / 8) * (chromaHeight / 8);
    for (int b = 0; b < cBlocks; b++)
        encodeBlock(cbQuant.data() + b * 64, prevDC, dcChroma, acChroma, bw);

    prevDC = 0;
    for (int b = 0; b < cBlocks; b++)
        encodeBlock(crQuant.data() + b * 64, prevDC, dcChroma, acChroma, bw);

    bw.flush();
    return bitstream;
}

void huffman::decode(
    const std::vector<uint8_t>& bitstream,
    std::vector<int>& yQuant,
    std::vector<int>& cbQuant,
    std::vector<int>& crQuant,
    int width, int height)
{
    const int chromaWidth  = (width  / 2 / 8) * 8;
    const int chromaHeight = (height / 2 / 8) * 8;

    HuffDecodeTable dcLuma,   acLuma;
    HuffDecodeTable dcChroma, acChroma;
    dcLuma.build  (DC_LUMA_BITS,   DC_LUMA_VALS);
    acLuma.build  (AC_LUMA_BITS,   AC_LUMA_VALS);
    dcChroma.build(DC_CHROMA_BITS, DC_CHROMA_VALS);
    acChroma.build(AC_CHROMA_BITS, AC_CHROMA_VALS);

    BitReader br{bitstream};

    int prevDC = 0;
    int yBlocks = (width / 8) * (height / 8);
    yQuant.resize(yBlocks * 64);
    for (int b = 0; b < yBlocks; b++)
        decodeBlock(yQuant.data() + b * 64, prevDC, dcLuma, acLuma, br);

    prevDC = 0;
    int cBlocks = (chromaWidth / 8) * (chromaHeight / 8);
    cbQuant.resize(cBlocks * 64);
    for (int b = 0; b < cBlocks; b++)
        decodeBlock(cbQuant.data() + b * 64, prevDC, dcChroma, acChroma, br);

    prevDC = 0;
    crQuant.resize(cBlocks * 64);
    for (int b = 0; b < cBlocks; b++)
        decodeBlock(crQuant.data() + b * 64, prevDC, dcChroma, acChroma, br);
}
