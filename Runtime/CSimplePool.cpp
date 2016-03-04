#include "CSimplePool.hpp"
#include "IVParamObj.hpp"
#include "CToken.hpp"

namespace urde
{

CSimplePool::CSimplePool(IFactory& factory)
: x30_factory(factory), x34_paramXfer(new TObjOwnerParam<IObjectStore*>(this))
{}

CToken CSimplePool::GetObj(const SObjectTag& tag, const CVParamTransfer& paramXfer)
{
    auto iter = std::find_if(x4_resources.begin(), x4_resources.end(),
    [&tag](std::pair<SObjectTag, CObjectReference*> pair) -> bool
    {
        return pair.first == tag;
    });

    if (iter != x4_resources.end())
        return CToken(iter->second);
    // TODO: There is some logic missing here, need to figure out what it's doing
    CObjectReference* ret = new CObjectReference(*this, x30_factory.Build(tag, paramXfer), tag, paramXfer);
    if (ret->GetObject())
    {
        x4_resources.push_back(std::make_pair<SObjectTag, CObjectReference*>((SObjectTag)tag, std::move(ret)));
        return CToken(ret);
    }

    delete ret;
    return CToken();
}

CToken CSimplePool::GetObj(const SObjectTag& tag)
{
    return GetObj(tag, x34_paramXfer);
}

CToken CSimplePool::GetObj(char const* resourceName)
{
    return GetObj(resourceName, x34_paramXfer);
}

CToken CSimplePool::GetObj(char const* resourceName, const CVParamTransfer& paramXfer)
{
    const SObjectTag* tag = x30_factory.GetResourceIdByName(resourceName);
    return GetObj(*tag, paramXfer);
}

bool CSimplePool::HasObject(const SObjectTag& tag) const
{
    auto iter = std::find_if(x4_resources.begin(), x4_resources.end(), [&tag](std::pair<SObjectTag, CObjectReference*> pair)->bool{
        return pair.first == tag;
    });

    return iter != x4_resources.end();
}

bool CSimplePool::ObjectIsLive(const SObjectTag&) const
{
    return false;
}

void CSimplePool::Flush()
{
}

void CSimplePool::ObjectUnreferenced(const SObjectTag& tag)
{
    auto iter = std::find_if(x4_resources.begin(), x4_resources.end(), [&tag](std::pair<SObjectTag, CObjectReference*> pair)->bool{
        return pair.first == tag;
    });
    if (iter != x4_resources.end())
        x4_resources.erase(iter);
}

}
