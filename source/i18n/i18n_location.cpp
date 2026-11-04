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

#include "enums/GameVersion.hpp"
#include "enums/Generation.hpp"
#include "i18n_internal.hpp"

namespace i18n
{
    struct Locations
    {
        std::map<u16, std::string> locations2;
        std::map<u16, std::string> locations3;
        std::map<u16, std::string> locations4;
        std::map<u16, std::string> locations5;
        std::map<u16, std::string> locations6;
        std::map<u16, std::string> locations7;
        std::map<u16, std::string> locationsLGPE;
        std::map<u16, std::string> locations8;
        std::map<u16, std::string> locationsPLA;
        std::map<u16, std::string> locationsBDSP;
        std::map<u16, std::string> locations9;
        std::map<u16, std::string> locationsZA;

        void clear() noexcept
        {
            locations2.clear();
            locations3.clear();
            locations4.clear();
            locations5.clear();
            locations6.clear();
            locations7.clear();
            locationsLGPE.clear();
            locations8.clear();
            locationsPLA.clear();
            locationsBDSP.clear();
            locations9.clear();
            locationsZA.clear();
        }

        // The list of the games of a generation. Generation EIGHT is Sword and Shield's,
        // and NINE is Scarlet and Violet's.
        const std::map<u16, std::string>* forGeneration(pksm::Generation gen) const
        {
            switch (gen)
            {
                case pksm::Generation::TWO:
                    return &locations2;
                case pksm::Generation::THREE:
                    return &locations3;
                case pksm::Generation::FOUR:
                    return &locations4;
                case pksm::Generation::FIVE:
                    return &locations5;
                case pksm::Generation::SIX:
                    return &locations6;
                case pksm::Generation::SEVEN:
                    return &locations7;
                case pksm::Generation::LGPE:
                    return &locationsLGPE;
                case pksm::Generation::EIGHT:
                    return &locations8;
                case pksm::Generation::NINE:
                    return &locations9;
                case pksm::Generation::UNUSED:
                case pksm::Generation::ONE:
                    break;
            }
            return nullptr;
        }

        // The list of a game, for the games that share a generation with others.
        const std::map<u16, std::string>* forGame(pksm::GameVersion version) const
        {
            if (version == pksm::GameVersion::PLA)
            {
                return &locationsPLA;
            }
            if (version == pksm::GameVersion::BD || version == pksm::GameVersion::SP)
            {
                return &locationsBDSP;
            }
            if (version == pksm::GameVersion::ZA)
            {
                return &locationsZA;
            }
            return forGeneration(pksm::Generation(version));
        }
    };

    std::unordered_map<pksm::Language, Locations> locationss = std::invoke(
        []
        {
            std::unordered_map<pksm::Language, Locations> ret;
            MAP(MAKE_GENERIC_LANGMAP, LANGUAGES_TO_USE)
            return ret;
        });

    void initLocation(pksm::Language lang)
    {
        Locations tmp;
        load(lang, "/locations2.txt", tmp.locations2);
        load(lang, "/locations3.txt", tmp.locations3);
        load(lang, "/locations4.txt", tmp.locations4);
        load(lang, "/locations5.txt", tmp.locations5);
        load(lang, "/locations6.txt", tmp.locations6);
        load(lang, "/locations7.txt", tmp.locations7);
        load(lang, "/locationsLGPE.txt", tmp.locationsLGPE);
        load(lang, "/locations8.txt", tmp.locations8);
        load(lang, "/locationsPLA.txt", tmp.locationsPLA);
        load(lang, "/locationsBDSP.txt", tmp.locationsBDSP);
        load(lang, "/locations9.txt", tmp.locations9);
        load(lang, "/locationsZA.txt", tmp.locationsZA);
        locationss.insert_or_assign(lang, std::move(tmp));
    }

    void exitLocation(pksm::Language lang)
    {
        locationss[lang].clear();
    }

    namespace
    {
        const std::string& find(const std::map<u16, std::string>* names, u16 v)
        {
            if (names)
            {
                if (auto found = names->find(v); found != names->end())
                {
                    return found->second;
                }
            }
            return emptyString;
        }
    }

    const std::string& location(pksm::Language lang, pksm::Generation gen, u16 v)
    {
        checkInitialized(lang);
        return find(locationss[lang].forGeneration(gen), v);
    }

    const std::string& location(pksm::Language lang, pksm::GameVersion version, u16 v)
    {
        checkInitialized(lang);
        return find(locationss[lang].forGame(version), v);
    }

    const std::map<u16, std::string>& rawLocations(pksm::Language lang, pksm::Generation g)
    {
        checkInitialized(lang);
        const auto* names = locationss[lang].forGeneration(g);
        return names ? *names : emptyU16Map;
    }

    const std::map<u16, std::string>& rawLocations(pksm::Language lang, pksm::GameVersion version)
    {
        checkInitialized(lang);
        const auto* names = locationss[lang].forGame(version);
        return names ? *names : emptyU16Map;
    }
}
