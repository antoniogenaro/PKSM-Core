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

#ifndef TRAINERID_HPP
#define TRAINERID_HPP

#include "enums/Generation.hpp"
#include "utils/coretypes.h"
#include <algorithm>
#include <utility>

// Trainer IDs as the games show them. Up to generation VI the trainer ID and the secret ID are
// shown as stored. From generation VII on, the game shows the two 16-bit values as one 32-bit
// number split in decimal: its last six digits are the trainer ID, the rest the secret ID.
namespace pksm::TrainerID
{
    inline constexpr u32 MAX_SIX_DIGIT_TID = 999999;
    // 0xFFFFFFFF is 4294967295, so with any trainer ID the secret ID fits up to 4294.
    inline constexpr u32 MAX_SIX_DIGIT_SID = 4294;

    [[nodiscard]] constexpr bool sixDigits(Generation generation) noexcept
    {
        return generation == Generation::SEVEN || generation == Generation::LGPE ||
               generation == Generation::EIGHT || generation == Generation::NINE;
    }

    [[nodiscard]] constexpr u32 shownTID(Generation generation, u16 tid, u16 sid) noexcept
    {
        if (generation == Generation::UNUSED)
        {
            return 0;
        }
        return sixDigits(generation) ? (u32(sid) << 16 | tid) % 1000000 : tid;
    }

    [[nodiscard]] constexpr u32 shownSID(Generation generation, u16 tid, u16 sid) noexcept
    {
        if (generation == Generation::UNUSED || generation == Generation::ONE ||
            generation == Generation::TWO)
        {
            return 0;
        }
        return sixDigits(generation) ? (u32(sid) << 16 | tid) / 1000000 : sid;
    }

    // The stored trainer ID and secret ID, in that order, for shown ones. Shown IDs too large
    // for the generation are clamped.
    [[nodiscard]] constexpr std::pair<u16, u16> stored(
        Generation generation, u32 shownTid, u32 shownSid) noexcept
    {
        if (!sixDigits(generation))
        {
            return {u16(std::min<u32>(shownTid, 0xFFFF)), u16(std::min<u32>(shownSid, 0xFFFF))};
        }
        const u64 wanted = u64(std::min(shownSid, MAX_SIX_DIGIT_SID)) * 1000000 +
                           std::min(shownTid, MAX_SIX_DIGIT_TID);
        const u32 full   = u32(std::min<u64>(wanted, 0xFFFFFFFF));
        return {u16(full & 0xFFFF), u16(full >> 16)};
    }
}

#endif
