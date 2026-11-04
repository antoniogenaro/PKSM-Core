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

#ifndef SAVBS_HPP
#define SAVBS_HPP

#include "sav/Sav.hpp"

namespace pksm
{
    // Brilliant Diamond and Shining Pearl. Unlike the other Switch games the save is one plain
    // structure at fixed offsets, signed with an MD5 of the whole file kept 0x10 bytes before the
    // end of the 1.0 layout; later updates only add data after it.
    class SavBS : public Sav
    {
    private:
        static constexpr u32 OFS_ITEMS      = 0x0563C;
        static constexpr u32 OFS_PARTY      = 0x14098;
        static constexpr u32 OFS_BOX_LAYOUT = 0x148AA;
        static constexpr u32 OFS_BOXES      = 0x14EF4;
        static constexpr u32 OFS_CONFIG     = 0x79B74;
        static constexpr u32 OFS_STATUS     = 0x79BB4;
        static constexpr u32 OFS_PLAY_TIME  = 0x79C04;
        static constexpr u32 OFS_ZUKAN      = 0x7A328;
        static constexpr u32 OFS_HASH       = 0xE9818;

        static constexpr u32 ITEM_SIZE = 16;

        [[nodiscard]] u8* status(void) const { return data.get() + OFS_STATUS; }

        [[nodiscard]] u8* boxLayout(void) const { return data.get() + OFS_BOX_LAYOUT; }

        [[nodiscard]] u8* itemRecord(u16 id) const { return data.get() + OFS_ITEMS + id * ITEM_SIZE; }

        [[nodiscard]] std::array<u8, 16> computeHash(void) const;

    public:
        static constexpr size_t SIZE_G8BDSP_0 = 0xE9828;
        static constexpr size_t SIZE_G8BDSP_1 = 0xEDC20;
        static constexpr size_t SIZE_G8BDSP_2 = 0xEED8C;
        static constexpr size_t SIZE_G8BDSP_3 = 0xEF0A4;

        // Whether the revision at the start of the save is the one that layout of this size has
        [[nodiscard]] static bool isBDSP(const std::shared_ptr<u8[]>& dt, size_t length);

        SavBS(const std::shared_ptr<u8[]>& dt, size_t length);

        void finishEditing(void) override;

        void beginEditing(void) override {}

        [[nodiscard]] bool checksumsValid(void) const override;

        [[nodiscard]] u16 TID(void) const override;
        void TID(u16 v) override;
        [[nodiscard]] u16 SID(void) const override;
        void SID(u16 v) override;
        [[nodiscard]] GameVersion version(void) const override;
        void version(GameVersion v) override;
        [[nodiscard]] Gender gender(void) const override;
        void gender(Gender v) override;
        [[nodiscard]] Language language(void) const override;
        void language(Language v) override;
        [[nodiscard]] std::string otName(void) const override;
        void otName(const std::string_view& v) override;
        [[nodiscard]] u32 money(void) const override;
        void money(u32 v) override;

        [[nodiscard]] u32 BP(void) const override { return 0; }

        void BP(u32) override {}

        [[nodiscard]] u8 badges(void) const override;
        [[nodiscard]] u16 playedHours(void) const override;
        void playedHours(u16 v) override;
        [[nodiscard]] u8 playedMinutes(void) const override;
        void playedMinutes(u8 v) override;
        [[nodiscard]] u8 playedSeconds(void) const override;
        void playedSeconds(u8 v) override;

        [[nodiscard]] u8 subRegion(void) const override { return 0; }

        void subRegion(u8) override {}

        [[nodiscard]] u8 country(void) const override { return 0; }

        void country(u8) override {}

        [[nodiscard]] u8 consoleRegion(void) const override { return 0; }

        void consoleRegion(u8) override {}

        [[nodiscard]] u8 currentBox(void) const override;
        void currentBox(u8 v) override;
        [[nodiscard]] u8 unlockedBoxes(void) const override;
        void unlockedBoxes(u8 v) override;

        [[nodiscard]] u8 legendBoxUnlockSize(void) const override { return 0; }

        [[nodiscard]] u32 boxOffset(u8 box, u8 slot) const override;
        [[nodiscard]] u32 partyOffset(u8 slot) const override;

        [[nodiscard]] std::unique_ptr<PKX> pkm(u8 slot) const override;
        void pkm(const PKX& pk, u8 slot) override;
        [[nodiscard]] std::unique_ptr<PKX> pkm(u8 box, u8 slot) const override;
        void pkm(const PKX& pk, u8 box, u8 slot, bool applyTrade) override;
        void trade(PKX& pk, const Date& date = Date::today()) const override;
        [[nodiscard]] std::unique_ptr<PKX> emptyPkm() const override;

        void dex(const PKX& pk) override;
        [[nodiscard]] int dexSeen(void) const override;
        [[nodiscard]] int dexCaught(void) const override;

        [[nodiscard]] int currentGiftAmount(void) const override { return 0; }

        [[nodiscard]] std::unique_ptr<WCX> mysteryGift(int) const override { return nullptr; }

        void mysteryGift(const WCX&, int&) override {}

        void cryptBoxData(bool crypted) override;
        [[nodiscard]] std::string boxName(u8 box) const override;
        void boxName(u8 box, const std::string_view& name) override;
        [[nodiscard]] u8 boxWallpaper(u8 box) const override;
        void boxWallpaper(u8 box, u8 v) override;
        [[nodiscard]] u8 partyCount(void) const override;
        void partyCount(u8 count) override;

        [[nodiscard]] int maxBoxes(void) const override { return 40; }

        [[nodiscard]] size_t maxWondercards(void) const override { return 0; }

        [[nodiscard]] Generation generation(void) const override { return Generation::EIGHT; }

        void item(const Item& item, Pouch pouch, u16 slot) override;
        [[nodiscard]] std::unique_ptr<Item> item(Pouch pouch, u16 slot) const override;
        [[nodiscard]] SmallVector<std::pair<Pouch, int>, 15> pouches(void) const override;
        [[nodiscard]] SmallVector<std::pair<Pouch, std::span<const int>>, 15> validItems(
            void) const override;

        [[nodiscard]] bool pouchIndexedByItem(Pouch) const override { return true; }
    };
}

#endif
