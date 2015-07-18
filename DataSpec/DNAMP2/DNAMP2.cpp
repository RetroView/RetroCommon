#define NOD_ATHENA 1
#include "DNAMP2.hpp"
#include "STRG.hpp"
#include "MLVL.hpp"

namespace Retro
{
namespace DNAMP2
{

LogVisor::LogModule Log("Retro::DNAMP2");

PAKBridge::PAKBridge(HECL::Database::Project& project, const NOD::DiscBase::IPartition::Node& node)
: m_project(project), m_node(node), m_pak(true)
{
    NOD::AthenaPartReadStream rs(node.beginReadStream());
    m_pak.read(rs);
}

std::string PAKBridge::getLevelString() const
{
    std::string retval;
    for (const DNAMP1::PAK::Entry& entry : m_pak.m_entries)
    {
        if (entry.type == Retro::MLVL)
        {
            PAKEntryReadStream rs = entry.beginReadStream(m_node);
            MLVL mlvl;
            mlvl.read(rs);
            const DNAMP1::PAK::Entry* nameEnt = m_pak.lookupEntry(mlvl.worldNameId);
            if (nameEnt)
            {
                PAKEntryReadStream rs = nameEnt->beginReadStream(m_node);
                STRG mlvlName;
                mlvlName.read(rs);
                if (retval.size())
                    retval += _S(", ");
                retval += mlvlName.getUTF8(ENGL, 0);
            }
        }
    }
    return retval;
}

ResExtractor PAKBridge::LookupExtractor(const DNAMP1::PAK::Entry& entry)
{
    if (entry.type == Retro::STRG)
        return {STRG::Extract<STRG>, ".as"};
    return {};
}

bool PAKBridge::extractResources(const HECL::ProjectPath& workingOut,
                                 const HECL::ProjectPath& cookedOut,
                                 bool force)
{
    for (const std::pair<UniqueID32, DNAMP1::PAK::Entry*>& item : m_pak.m_idMap)
    {
        PAKEntryReadStream s;
        ResExtractor extractor = LookupExtractor(*item.second);
        if (extractor.func)
        {
            HECL::ProjectPath workPath(workingOut, m_pak.bestEntryName(*item.second) + extractor.fileExt);
            if (force || workPath.getPathType() == HECL::ProjectPath::PT_NONE)
            {
                s = item.second->beginReadStream(m_node);
                extractor.func(s, workPath);
            }
        }
        HECL::ProjectPath cookPath(cookedOut, m_pak.bestEntryName(*item.second));
        if (force || cookPath.getPathType() == HECL::ProjectPath::PT_NONE)
        {
            if (!s)
                s = item.second->beginReadStream(m_node);
            FILE* fout = HECL::Fopen(cookPath.getAbsolutePath().c_str(), _S("wb"));
            fwrite(s.data(), 1, s.length(), fout);
            fclose(fout);
        }
    }
    return true;
}

}
}
