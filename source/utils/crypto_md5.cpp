/*
 *   This file is part of PKSM-Core
 *   Copyright (C) 2016-2022 Bernardo Giordano, Admiral Fish, piepie62
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
 *       * Requiring preservation of specified reasonable legal notices or
 *         author attributions in that material or in the Appropriate Legal
 *         Notices displayed by works containing it.
 *       * Prohibiting misrepresentation of the origin of that material,
 *         or requiring that modified versions of such material be marked in
 *         reasonable ways as different from the original version.
 */

#include "utils/crypto.hpp"
#include <bit>

namespace pksm::crypto
{
    namespace
    {
        constexpr u32 SHIFTS[64] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 5,
            9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11,
            16, 23, 4, 11, 16, 23, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

        constexpr u32 SINES[64] = {0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf,
            0x4787c62a, 0xa8304613, 0xfd469501, 0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
            0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821, 0xf61e2562, 0xc040b340, 0x265e5a51,
            0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8, 0x21e1cde6, 0xc33707d6,
            0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a, 0xfffa3942,
            0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
            0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8,
            0xc4ac5665, 0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92,
            0xffeff47d, 0x85845dd1, 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82,
            0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

        void block(u32 state[4], const u8* chunk)
        {
            u32 m[16];
            for (int i = 0; i < 16; i++)
            {
                m[i] = u32(chunk[i * 4]) | (u32(chunk[i * 4 + 1]) << 8) |
                       (u32(chunk[i * 4 + 2]) << 16) | (u32(chunk[i * 4 + 3]) << 24);
            }

            u32 a = state[0], b = state[1], c = state[2], d = state[3];
            for (int i = 0; i < 64; i++)
            {
                u32 f;
                int g;
                if (i < 16)
                {
                    f = (b & c) | (~b & d);
                    g = i;
                }
                else if (i < 32)
                {
                    f = (d & b) | (~d & c);
                    g = (5 * i + 1) % 16;
                }
                else if (i < 48)
                {
                    f = b ^ c ^ d;
                    g = (3 * i + 5) % 16;
                }
                else
                {
                    f = c ^ (b | ~d);
                    g = (7 * i) % 16;
                }
                f = f + a + SINES[i] + m[g];
                a = d;
                d = c;
                c = b;
                b = b + std::rotl(f, int(SHIFTS[i]));
            }
            state[0] += a;
            state[1] += b;
            state[2] += c;
            state[3] += d;
        }
    }

    std::array<u8, 16> md5(std::span<const u8> data)
    {
        u32 state[4] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};

        size_t whole = data.size() - data.size() % 64;
        for (size_t i = 0; i < whole; i += 64)
        {
            block(state, data.data() + i);
        }

        // The rest, a 1 bit, zeros and the length in bits fill one or two last blocks.
        u8 tail[128] = {};
        size_t rest  = data.size() - whole;
        for (size_t i = 0; i < rest; i++)
        {
            tail[i] = data[whole + i];
        }
        tail[rest]       = 0x80;
        size_t tailSize  = rest < 56 ? 64 : 128;
        u64 bits         = u64(data.size()) * 8;
        for (int i = 0; i < 8; i++)
        {
            tail[tailSize - 8 + i] = u8(bits >> (8 * i));
        }
        for (size_t i = 0; i < tailSize; i += 64)
        {
            block(state, tail + i);
        }

        std::array<u8, 16> digest;
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                digest[i * 4 + j] = u8(state[i] >> (8 * j));
            }
        }
        return digest;
    }
}
