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

#include "sav/SavBS.hpp"
#include "pkx/PB8.hpp"
#include "sav/Item.hpp"
#include "utils/crypto.hpp"
#include "utils/endian.hpp"
#include "utils/utils.hpp"
#include <algorithm>
#include <ranges>

namespace pksm
{
    namespace
    {
        // The revision each layout's save starts with, as PKHeX's Gem8Version names them
        constexpr u32 REVISION_1_0 = 0x25;
        constexpr u32 REVISION_1_1 = 0x2C;
        constexpr u32 REVISION_1_2 = 0x32;
        constexpr u32 REVISION_1_3 = 0x34;

        constexpr u16 MAX_SPECIES = 493;
        constexpr u32 MAX_MONEY   = 999999;
        constexpr u16 NO_SORT     = 0;

        constexpr u32 BOX_NAME_SIZE  = 0x22;
        constexpr u32 BOX_UNLOCKED   = 0x61D;
        constexpr u32 BOX_CURRENT    = 0x61E;
        constexpr u32 BOX_WALLPAPERS = 0x620;
        constexpr u32 PARTY_COUNT    = 6 * PB8::PARTY_LENGTH;

        // Pokédex (ZUKAN_WORK): a u32 state per species, four u32 booleans per species for
        // shiny male, shiny female, male and female, the form booleans, then a u32 of language
        // bits per species.
        constexpr u32 DEX_SEEN         = 2;
        constexpr u32 DEX_CAUGHT       = 3;
        constexpr u32 DEX_MALE_SHINY   = 4 * MAX_SPECIES;
        constexpr u32 DEX_FEMALE_SHINY = DEX_MALE_SHINY + 4 * MAX_SPECIES;
        constexpr u32 DEX_MALE         = DEX_FEMALE_SHINY + 4 * MAX_SPECIES;
        constexpr u32 DEX_FEMALE       = DEX_MALE + 4 * MAX_SPECIES;
        constexpr u32 DEX_FORMS        = DEX_FEMALE + 4 * MAX_SPECIES;

        // The species with forms, in the order their form booleans follow each other. Each has
        // its list for normal ones and then one as long for shiny ones.
        constexpr std::pair<u16, u8> DEX_FORM_COUNTS[] = {
            {201, 28}, // Unown
            {351, 4 }, // Castform
            {386, 4 }, // Deoxys
            {412, 3 }, // Burmy
            {413, 3 }, // Wormadam
            {414, 3 }, // Mothim
            {421, 2 }, // Cherrim
            {422, 2 }, // Shellos
            {423, 2 }, // Gastrodon
            {479, 6 }, // Rotom
            {487, 2 }, // Giratina
            {492, 2 }, // Shaymin
            {493, 18}, // Arceus
        };

        constexpr u32 dexLanguages(void)
        {
            u32 offset = DEX_FORMS;
            for (const auto& entry : DEX_FORM_COUNTS)
            {
                offset += 2 * 4 * entry.second;
            }
            return offset;
        }

        constexpr u32 DEX_LANGUAGES = dexLanguages();
        static_assert(DEX_LANGUAGES + 4 * MAX_SPECIES == 0x30B0);

        // Japanese is bit 0; the unused language 6 has no bit
        int dexLanguageBit(Language lang)
        {
            const int value = int(u8(lang));
            if (value == 0 || lang == Language::UNUSED || value > int(u8(Language::CHT)))
            {
                return -1;
            }
            return value >= int(u8(Language::SPA)) ? value - 2 : value - 1;
        }

        constexpr int medicineItems[] = {17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
            31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 54};

        constexpr int ballItems[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 492,
            493, 494, 495, 496, 497, 498, 499, 500, 576, 851};

        constexpr int battleItems[] = {55, 56, 57, 58, 59, 60, 61, 62, 63};

        constexpr int berryItems[] = {149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160,
            161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177,
            178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
            195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211,
            212, 686};

        constexpr int normalItems[] = {45, 46, 47, 48, 49, 50, 51, 52, 53, 72, 73, 74, 75, 76, 77,
            78, 79, 80, 81, 82, 83, 84, 85, 93, 94, 107, 108, 109, 110, 111, 112, 135, 136, 213,
            214, 215, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231,
            232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248,
            249, 250, 251, 252, 253, 254, 255, 256, 257, 258, 259, 260, 261, 262, 263, 264, 265,
            266, 267, 268, 269, 270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 280, 281, 282,
            283, 284, 285, 286, 287, 288, 289, 290, 291, 292, 293, 294, 295, 296, 297, 298, 299,
            300, 301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313, 314, 315, 316,
            317, 318, 319, 320, 321, 322, 323, 324, 325, 326, 327, 537, 565, 566, 567, 568, 569,
            570, 644, 645, 849, 1231, 1232, 1233, 1234, 1235, 1236, 1237, 1238, 1239, 1240, 1241,
            1242, 1243, 1244, 1245, 1246, 1247, 1248, 1249, 1250, 1251, 1606};

        // TM01 to TM100; the last eight were HMs in Diamond and Pearl
        constexpr int tmItems[] = {328, 329, 330, 331, 332, 333, 334, 335, 336, 337, 338, 339,
            340, 341, 342, 343, 344, 345, 346, 347, 348, 349, 350, 351, 352, 353, 354, 355, 356,
            357, 358, 359, 360, 361, 362, 363, 364, 365, 366, 367, 368, 369, 370, 371, 372, 373,
            374, 375, 376, 377, 378, 379, 380, 381, 382, 383, 384, 385, 386, 387, 388, 389, 390,
            391, 392, 393, 394, 395, 396, 397, 398, 399, 400, 401, 402, 403, 404, 405, 406, 407,
            408, 409, 410, 411, 412, 413, 414, 415, 416, 417, 418, 419, 420, 421, 422, 423, 424,
            425, 426, 427};

        constexpr int treasureItems[] = {86, 87, 88, 89, 90, 91, 92, 99, 100, 101, 102, 103, 104,
            105, 106, 795, 796, 1808, 1809, 1810, 1811, 1812, 1813, 1814, 1815, 1816, 1817, 1818,
            1819, 1820, 1821};

        constexpr int keyItems[] = {428, 431, 432, 433, 438, 439, 440, 443, 445, 446, 447, 448,
            449, 450, 451, 452, 453, 454, 455, 459, 460, 461, 462, 463, 464, 466, 467, 631, 632,
            1267, 1278, 1822};

        std::span<const int> pouchItems(Sav::Pouch pouch)
        {
            switch (pouch)
            {
                case Sav::Pouch::NormalItem:
                    return normalItems;
                case Sav::Pouch::KeyItem:
                    return keyItems;
                case Sav::Pouch::TM:
                    return tmItems;
                case Sav::Pouch::Medicine:
                    return medicineItems;
                case Sav::Pouch::Berry:
                    return berryItems;
                case Sav::Pouch::Ball:
                    return ballItems;
                case Sav::Pouch::Battle:
                    return battleItems;
                case Sav::Pouch::Treasure:
                    return treasureItems;
                default:
                    return {};
            }
        }

        bool isPB8Sized(const PKX& pk)
        {
            return pk.generation() == Generation::EIGHT &&
                   (pk.getLength() == PB8::BOX_LENGTH || pk.getLength() == PB8::PARTY_LENGTH);
        }
    }

    bool SavBS::isBDSP(const std::shared_ptr<u8[]>& dt, size_t length)
    {
        const u32 revision = LittleEndian::convertTo<u32>(dt.get());
        switch (length)
        {
            case SIZE_G8BDSP_0:
                return revision == REVISION_1_0;
            case SIZE_G8BDSP_1:
                return revision == REVISION_1_1;
            case SIZE_G8BDSP_2:
                return revision == REVISION_1_2;
            case SIZE_G8BDSP_3:
                return revision == REVISION_1_3;
            default:
                return false;
        }
    }

    SavBS::SavBS(const std::shared_ptr<u8[]>& dt, size_t length) : Sav(dt, length)
    {
        game  = Game::BDSP;
        Box   = OFS_BOXES;
        Party = OFS_PARTY;
    }

    std::array<u8, 16> SavBS::computeHash(void) const
    {
        // The hash is taken with its own bytes zeroed
        std::array<u8, 16> stored;
        std::copy(data.get() + OFS_HASH, data.get() + OFS_HASH + 16, stored.begin());
        std::fill_n(data.get() + OFS_HASH, 16, 0);
        auto hash = crypto::md5(std::span<const u8>(data.get(), length));
        std::ranges::copy(stored, data.get() + OFS_HASH);
        return hash;
    }

    void SavBS::finishEditing(void)
    {
        std::ranges::copy(computeHash(), data.get() + OFS_HASH);
    }

    bool SavBS::checksumsValid(void) const
    {
        return std::ranges::equal(computeHash(), std::span<const u8>(data.get() + OFS_HASH, 16));
    }

    u16 SavBS::TID(void) const
    {
        return LittleEndian::convertTo<u16>(status() + 0x1C);
    }

    void SavBS::TID(u16 v)
    {
        LittleEndian::convertFrom<u16>(status() + 0x1C, v);
    }

    u16 SavBS::SID(void) const
    {
        return LittleEndian::convertTo<u16>(status() + 0x1E);
    }

    void SavBS::SID(u16 v)
    {
        LittleEndian::convertFrom<u16>(status() + 0x1E, v);
    }

    GameVersion SavBS::version(void) const
    {
        // The save keeps its own ROM code: 0 Brilliant Diamond, 1 Shining Pearl
        return status()[0x2B] == 1 ? GameVersion::SP : GameVersion::BD;
    }

    void SavBS::version(GameVersion v)
    {
        if (v == GameVersion::BD || v == GameVersion::SP)
        {
            status()[0x2B] = v == GameVersion::SP ? 1 : 0;
        }
    }

    Gender SavBS::gender(void) const
    {
        return status()[0x24] == 1 ? Gender::Male : Gender::Female;
    }

    void SavBS::gender(Gender v)
    {
        status()[0x24] = v == Gender::Male ? 1 : 0;
    }

    Language SavBS::language(void) const
    {
        return Language(u8(LittleEndian::convertTo<u32>(data.get() + OFS_CONFIG + 4)));
    }

    void SavBS::language(Language v)
    {
        LittleEndian::convertFrom<u32>(data.get() + OFS_CONFIG + 4, u32(u8(v)));
    }

    std::string SavBS::otName(void) const
    {
        return StringUtils::getString(data.get(), OFS_STATUS, 13);
    }

    void SavBS::otName(const std::string_view& v)
    {
        StringUtils::setString(data.get(), v, OFS_STATUS, 13);
    }

    u32 SavBS::money(void) const
    {
        return LittleEndian::convertTo<u32>(status() + 0x20);
    }

    void SavBS::money(u32 v)
    {
        LittleEndian::convertFrom<u32>(status() + 0x20, std::min(v, MAX_MONEY));
    }

    u8 SavBS::badges(void) const
    {
        return status()[0x29];
    }

    u16 SavBS::playedHours(void) const
    {
        return LittleEndian::convertTo<u16>(data.get() + OFS_PLAY_TIME);
    }

    void SavBS::playedHours(u16 v)
    {
        LittleEndian::convertFrom<u16>(data.get() + OFS_PLAY_TIME, v);
    }

    u8 SavBS::playedMinutes(void) const
    {
        return data[OFS_PLAY_TIME + 2];
    }

    void SavBS::playedMinutes(u8 v)
    {
        data[OFS_PLAY_TIME + 2] = v;
    }

    u8 SavBS::playedSeconds(void) const
    {
        return data[OFS_PLAY_TIME + 3];
    }

    void SavBS::playedSeconds(u8 v)
    {
        data[OFS_PLAY_TIME + 3] = v;
    }

    u8 SavBS::currentBox(void) const
    {
        return boxLayout()[BOX_CURRENT];
    }

    void SavBS::currentBox(u8 v)
    {
        boxLayout()[BOX_CURRENT] = v;
    }

    u8 SavBS::unlockedBoxes(void) const
    {
        return boxLayout()[BOX_UNLOCKED];
    }

    void SavBS::unlockedBoxes(u8 v)
    {
        boxLayout()[BOX_UNLOCKED] = std::min<u8>(v, maxBoxes());
    }

    std::string SavBS::boxName(u8 box) const
    {
        return StringUtils::getString(data.get(), OFS_BOX_LAYOUT + box * BOX_NAME_SIZE, 17);
    }

    void SavBS::boxName(u8 box, const std::string_view& name)
    {
        StringUtils::setString(data.get(), name, OFS_BOX_LAYOUT + box * BOX_NAME_SIZE, 17);
    }

    u8 SavBS::boxWallpaper(u8 box) const
    {
        // Kept one higher than the wallpaper's number
        const u8 stored = boxLayout()[BOX_WALLPAPERS + box];
        return stored == 0 ? 0 : stored - 1;
    }

    void SavBS::boxWallpaper(u8 box, u8 v)
    {
        boxLayout()[BOX_WALLPAPERS + box] = v + 1;
    }

    u32 SavBS::boxOffset(u8 box, u8 slot) const
    {
        // Box slots hold whole party records
        return OFS_BOXES + PB8::PARTY_LENGTH * (u32(box) * 30 + slot);
    }

    u32 SavBS::partyOffset(u8 slot) const
    {
        return OFS_PARTY + PB8::PARTY_LENGTH * slot;
    }

    u8 SavBS::partyCount(void) const
    {
        return data[OFS_PARTY + PARTY_COUNT];
    }

    void SavBS::partyCount(u8 count)
    {
        data[OFS_PARTY + PARTY_COUNT] = count;
    }

    std::unique_ptr<PKX> SavBS::pkm(u8 slot) const
    {
        return PKX::getPKM<PB8>(data.get() + partyOffset(slot), PB8::PARTY_LENGTH);
    }

    std::unique_ptr<PKX> SavBS::pkm(u8 box, u8 slot) const
    {
        return PKX::getPKM<PB8>(data.get() + boxOffset(box, slot), PB8::PARTY_LENGTH);
    }

    void SavBS::pkm(const PKX& pk, u8 box, u8 slot, bool applyTrade)
    {
        if (isPB8Sized(pk))
        {
            auto pb8 = pk.partyClone();
            if (applyTrade)
            {
                trade(*pb8);
            }
            // Stored encrypted; the game reads plain data as a Bad Egg
            pb8->encrypt();
            std::ranges::copy(pb8->rawData(), data.get() + boxOffset(box, slot));
        }
    }

    void SavBS::pkm(const PKX& pk, u8 slot)
    {
        if (isPB8Sized(pk))
        {
            auto pb8 = pk.partyClone();
            pb8->encrypt();
            std::ranges::copy(pb8->rawData(), data.get() + partyOffset(slot));
        }
    }

    void SavBS::trade(PKX& pk, const Date& date) const
    {
        if (!isPB8Sized(pk))
        {
            return;
        }

        const bool fromOT = pk.otName() == otName() && pk.TID() == TID() && pk.SID() == SID() &&
                            pk.otGender() == gender();
        if (pk.egg())
        {
            if (!fromOT)
            {
                // A link trade's egg, as the game marks it
                pk.metLocation(30001);
                pk.metDate(date);
            }
            return;
        }

        if (fromOT)
        {
            pk.currentHandler(PKXHandler::OT);
        }
        else
        {
            pk.currentHandler(PKXHandler::NonOT);
            pk.currentFriendship(pk.baseFriendship());
            PK8& pk8 = static_cast<PK8&>(pk);
            pk8.htName(otName());
            pk8.htGender(gender());
            pk8.htLanguage(language());
        }
    }

    std::unique_ptr<PKX> SavBS::emptyPkm() const
    {
        return PKX::getPKM<PB8>(nullptr, PB8::BOX_LENGTH);
    }

    void SavBS::dex(const PKX& pk)
    {
        const u16 species = u16(pk.species());
        if (species == 0 || species > MAX_SPECIES || pk.egg())
        {
            return;
        }

        u8* dex            = data.get() + OFS_ZUKAN;
        const u32 index    = species - 1;
        const bool shiny   = pk.shiny();
        const auto setTrue = [&](u32 offset) { LittleEndian::convertFrom<u32>(dex + offset, 1); };

        LittleEndian::convertFrom<u32>(dex + 4 * index, DEX_CAUGHT);

        // Genderless Pokémon set both
        if (pk.gender() != Gender::Female)
        {
            setTrue((shiny ? DEX_MALE_SHINY : DEX_MALE) + 4 * index);
        }
        if (pk.gender() != Gender::Male)
        {
            setTrue((shiny ? DEX_FEMALE_SHINY : DEX_FEMALE) + 4 * index);
        }

        const int languageBit = dexLanguageBit(pk.language());
        if (languageBit >= 0)
        {
            u8* languages = dex + DEX_LANGUAGES + 4 * index;
            LittleEndian::convertFrom<u32>(
                languages, LittleEndian::convertTo<u32>(languages) | (1u << languageBit));
        }

        u32 formOffset = DEX_FORMS;
        for (const auto& [formSpecies, count] : DEX_FORM_COUNTS)
        {
            if (formSpecies == species)
            {
                const u16 form = pk.alternativeForm();
                if (form < count)
                {
                    setTrue(formOffset + (shiny ? 4 * count : 0) + 4 * form);
                }
                break;
            }
            formOffset += 2 * 4 * count;
        }
    }

    int SavBS::dexSeen(void) const
    {
        int seen = 0;
        for (u16 i = 0; i < MAX_SPECIES; i++)
        {
            if (LittleEndian::convertTo<u32>(data.get() + OFS_ZUKAN + 4 * i) >= DEX_SEEN)
            {
                seen++;
            }
        }
        return seen;
    }

    int SavBS::dexCaught(void) const
    {
        int caught = 0;
        for (u16 i = 0; i < MAX_SPECIES; i++)
        {
            if (LittleEndian::convertTo<u32>(data.get() + OFS_ZUKAN + 4 * i) >= DEX_CAUGHT)
            {
                caught++;
            }
        }
        return caught;
    }

    void SavBS::cryptBoxData(bool crypted)
    {
        for (u8 box = 0; box < maxBoxes(); box++)
        {
            for (u8 slot = 0; slot < 30; slot++)
            {
                std::unique_ptr<PKX> pb8 = PKX::getPKM<PB8>(
                    data.get() + boxOffset(box, slot), PB8::PARTY_LENGTH, true);
                if (!crypted)
                {
                    pb8->encrypt();
                }
            }
        }
    }

    void SavBS::item(const Item& item, Pouch pouch, u16 slot)
    {
        const auto items = pouchItems(pouch);
        if (slot >= items.size())
        {
            return;
        }

        const u16 id = u16(items[slot]);
        u8* record   = itemRecord(id);
        Item8b stored(record, id);
        const u16 count = std::min(item.count(), maxCount(pouch));

        if (count == 0)
        {
            // An item the player no longer has keeps no record, as the game leaves it
            std::fill_n(record, ITEM_SIZE, 0);
            return;
        }

        if (stored.sortOrder() == NO_SORT)
        {
            // A new item goes after the others in its pouch, marked new
            u16 last = 0;
            for (const int other : items)
            {
                last = std::max(last, Item8b(itemRecord(u16(other)), u16(other)).sortOrder());
            }
            stored.sortOrder(last + 1);
            stored.newFlag(true);
            stored.favoriteFlag(false);
        }
        else if (auto bytes = item.bytes();
                 item.generation() == Generation::EIGHT && bytes.size() == ITEM_SIZE)
        {
            // The marks travel with an item of these games the player already has; Sword and
            // Shield's are 4 bytes
            const Item8b item8b(bytes.data(), id);
            stored.newFlag(item8b.newFlag());
            stored.favoriteFlag(item8b.favoriteFlag());
        }
        stored.count(count);

        auto bytes = stored.bytes();
        std::copy(bytes.begin(), bytes.end(), record);
    }

    std::unique_ptr<Item> SavBS::item(Pouch pouch, u16 slot) const
    {
        const auto items = pouchItems(pouch);
        if (slot >= items.size())
        {
            return std::make_unique<Item8b>();
        }
        const u16 id = u16(items[slot]);
        return std::make_unique<Item8b>(itemRecord(id), id);
    }

    SmallVector<std::pair<Sav::Pouch, int>, 15> SavBS::pouches(void) const
    {
        // In the game's own order
        return {
            std::pair{Pouch::NormalItem, int(std::size(normalItems))  },
            std::pair{Pouch::Medicine,   int(std::size(medicineItems))},
            std::pair{Pouch::Ball,       int(std::size(ballItems))    },
            std::pair{Pouch::TM,         int(std::size(tmItems))      },
            std::pair{Pouch::Berry,      int(std::size(berryItems))   },
            std::pair{Pouch::Battle,     int(std::size(battleItems))  },
            std::pair{Pouch::Treasure,   int(std::size(treasureItems))},
            std::pair{Pouch::KeyItem,    int(std::size(keyItems))     },
        };
    }

    SmallVector<std::pair<Sav::Pouch, std::span<const int>>, 15> SavBS::validItems(void) const
    {
        return {
            std::pair{Pouch::NormalItem, std::span<const int>(normalItems)  },
            std::pair{Pouch::Medicine,   std::span<const int>(medicineItems)},
            std::pair{Pouch::Ball,       std::span<const int>(ballItems)    },
            std::pair{Pouch::TM,         std::span<const int>(tmItems)      },
            std::pair{Pouch::Berry,      std::span<const int>(berryItems)   },
            std::pair{Pouch::Battle,     std::span<const int>(battleItems)  },
            std::pair{Pouch::Treasure,   std::span<const int>(treasureItems)},
            std::pair{Pouch::KeyItem,    std::span<const int>(keyItems)     },
        };
    }
}
