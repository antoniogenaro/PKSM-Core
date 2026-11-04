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

#ifndef PB8_HPP
#define PB8_HPP

#include "pkx/PK8.hpp"

namespace pksm
{
    // Brilliant Diamond and Shining Pearl's Pokémon. They are laid out as Sword and Shield's
    // are; what differs is the personal table their stats and forms come from and the blank
    // Pokémon, whose met and egg locations read "none" (65535) rather than 0.
    class PB8 : public PK8
    {
    public:
        static constexpr u16 NO_LOCATION = 0xFFFF;

        PB8(PrivateConstructor, u8* dt, bool party = false, bool directAccess = false);

        [[nodiscard]] std::string_view extension() const override { return ".pb8"; }

        [[nodiscard]] std::unique_ptr<PKX> clone(void) const override;

        [[nodiscard]] u16 formSpecies(void) const override;

        u8 baseHP(void) const override { return PersonalBDSP::baseHP(formSpecies()); }

        u8 baseAtk(void) const override { return PersonalBDSP::baseAtk(formSpecies()); }

        u8 baseDef(void) const override { return PersonalBDSP::baseDef(formSpecies()); }

        u8 baseSpe(void) const override { return PersonalBDSP::baseSpe(formSpecies()); }

        u8 baseSpa(void) const override { return PersonalBDSP::baseSpa(formSpecies()); }

        u8 baseSpd(void) const override { return PersonalBDSP::baseSpd(formSpecies()); }

        Type type1(void) const override { return PersonalBDSP::type1(formSpecies()); }

        Type type2(void) const override { return PersonalBDSP::type2(formSpecies()); }

        u8 genderType(void) const override { return PersonalBDSP::gender(formSpecies()); }

        u8 baseFriendship(void) const override
        {
            return PersonalBDSP::baseFriendship(formSpecies());
        }

        u8 expType(void) const override { return PersonalBDSP::expType(formSpecies()); }

        Ability abilities(u8 n) const override { return PersonalBDSP::ability(formSpecies(), n); }

        u16 formStatIndex(void) const override
        {
            return PersonalBDSP::formStatIndex(formSpecies());
        }
    };
}

#endif
