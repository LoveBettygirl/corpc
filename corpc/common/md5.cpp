#include "corpc/common/md5.h"

const uint32_t MD5::A = 0x67452301;
const uint32_t MD5::B = 0xEFCDAB89;
const uint32_t MD5::C = 0x98BADCFE;
const uint32_t MD5::D = 0x10325476;

const uint32_t MD5::T[64] = {
    0xD76AA478,0xE8C7B756,0x242070DB,0xC1BDCEEE,0xF57C0FAF,0x4787C62A,0xA8304613,0xFD469501,
    0x698098D8,0x8B44F7AF,0xFFFF5BB1,0x895CD7BE,0x6B901122,0xFD987193,0xA679438E,0x49B40821,
    0xF61E2562,0xC040B340,0x265E5A51,0xE9B6C7AA,0xD62F105D,0x02441453,0xD8A1E681,0xE7D3FBC8,
    0x21E1CDE6,0xC33707D6,0xF4D50D87,0x455A14ED,0xA9E3E905,0xFCEFA3F8,0x676F02D9,0x8D2A4C8A,
    0xFFFA3942,0x8771F681,0x6D9D6122,0xFDE5380C,0xA4BEEA44,0x4BDECFA9,0xF6BB4B60,0xBEBFBC70,
    0x289B7EC6,0xEAA127FA,0xD4EF3085,0x04881D05,0xD9D4D039,0xE6DB99E5,0x1FA27CF8,0xC4AC5665,
    0xF4292244,0x432AFF97,0xAB9423A7,0xFC93A039,0x655B59C3,0x8F0CCC92,0xFFEFF47D,0x85845DD1,
    0x6FA87E4F,0xFE2CE6E0,0xA3014314,0x4E0811A1,0xF7537E82,0xBD3AF235,0x2AD7D2BB,0xEB86D391
};

const uint32_t MD5::S[64] = {
    7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,
    5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
    4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,
    6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21
};

const uint32_t MD5::X[64] = {
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
    1,6,11,0,5,10,15,4,9,14,3,8,13,2,7,12,
    5,8,11,14,1,4,7,10,13,0,3,6,9,12,15,2,
    0,7,14,5,12,3,10,1,8,15,6,13,4,11,2,9
};

MD5::MD5()
{
	resetVector();
}

void MD5::calculateBlock(uint8_t *input)
{
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t *x = (uint32_t*)input;	// 将64字节的数据块划分为16个4字节大小的子分组
    int i = 0;
    /* 共4轮运算，每一轮16次 */
    /* 第1轮运算*/
    for (; i < 16; i += 4) {
        FF(a, b, c, d, x[X[i]], i);
        FF(d, a, b, c, x[X[i + 1]], i + 1);
        FF(c, d, a, b, x[X[i + 2]], i + 2);
        FF(b, c, d, a, x[X[i + 3]], i + 3);
    }

    /* 第2轮运算*/
    for (; i < 32; i += 4) {
        GG(a, b, c, d, x[X[i]], i);
        GG(d, a, b, c, x[X[i + 1]], i + 1);
        GG(c, d, a, b, x[X[i + 2]], i + 2);
        GG(b, c, d, a, x[X[i + 3]], i + 3);
    }

    /* 第3轮运算*/
    for (; i < 48; i += 4) {
        HH(a, b, c, d, x[X[i]], i);
        HH(d, a, b, c, x[X[i + 1]], i + 1);
        HH(c, d, a, b, x[X[i + 2]], i + 2);
        HH(b, c, d, a, x[X[i + 3]], i + 3);
    }

    /* 第4轮运算*/
    for (; i < 64; i += 4) {
        II(a, b, c, d, x[X[i]], i);
        II(d, a, b, c, x[X[i + 1]], i + 1);
        II(c, d, a, b, x[X[i + 2]], i + 2);
        II(b, c, d, a, x[X[i + 3]], i + 3);
    }

    // 将a、b、c、d 中的运算结果加到初始向量上 
    state[0] += a;  state[1] += b;  state[2] += c;  state[3] += d;
}

void MD5::string2BitStream(const std::string &src)
{
    for (int i = 0; i < src.length(); i++) {
        data.push_back((uint8_t)src[i]);
    }
}

void MD5::padding()
{
    int fileBitSize = data.size() << 3;
    int newSize = fileBitSize;
    // 一直补充直到：比特流长度 % 512 == 448
    if (newSize % 512 != 448) {
        data.push_back((uint8_t)0x80);
        newSize += 8;
        for (int i = newSize; newSize % 512 != 448; newSize += 8)
            data.push_back(0);
    }
    // 补充64位文件长度
    uint64_t fileBitSize64 = fileBitSize;
    uint8_t *temp = (uint8_t*)&fileBitSize64;
    for (int i = 0; i < 8; i++) {
        data.push_back(temp[i]);
    }
}

void MD5::compute()
{
    for (int i = 0; i < data.size(); i += 64) {
        calculateBlock(&data[i]);
    }
}

std::vector<uint8_t> MD5::genDigest()
{
    std::vector<uint8_t> result;
    for (int i = 0; i < 4; i++) {
        // trick: 由于先前的计算都是按照小端模式进行计算
        // 因此转为字符串的时候要转换成大端模式
        uint32_t n = state[i];
        while (n != 0) {
            result.push_back(n & 0xFF);
            n >>= 8;
        }
    }
    return result;
}

std::vector<uint8_t> MD5::digest(const std::string &src)
{
    resetVector();
    string2BitStream(src);
    padding();
    compute();
    return genDigest();
}

bool MD5::judgeHexString(const std::string &src)
{
    for (int i = 0; i < src.size(); i++) {
        if (isalpha(src[i])) {
            char c = tolower(src[i]);
            if (c != 'a' && c != 'b' && c != 'c' && c != 'd' && c != 'e' && c != 'f')
                return false;
        }
        else if (isdigit(src[i]))
            continue;
        else
            return false;
    }
    return true;
}

void MD5::resetVector()
{
    data.clear();
    state[0] = A;
    state[1] = B;
    state[2] = C;
    state[3] = D;
}