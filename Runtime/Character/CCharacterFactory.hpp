#ifndef __PSHAG_CCHARACTERFACTORY_HPP__
#define __PSHAG_CCHARACTERFACTORY_HPP__

#include "../IFactory.hpp"

namespace urde
{

class CCharacterFactory : public IFactory
{
public:
    class CDummyFactory : public IFactory
    {
    };
};

}

#endif // __PSHAG_CCHARACTERFACTORY_HPP__
