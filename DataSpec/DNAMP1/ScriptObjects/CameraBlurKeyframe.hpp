#ifndef _DNAMP1_CAMERABLURKEYFRAME_HPP_
#define _DNAMP1_CAMERABLURKEYFRAME_HPP_

#include "../../DNACommon/DNACommon.hpp"
#include "IScriptObject.hpp"
#include "Parameters.hpp"

namespace DataSpec
{
namespace DNAMP1
{
struct CameraBlurKeyframe : IScriptObject
{
    DECL_YAML
    String<-1> name;
    Value<bool> active;
    Value<atUint32> btype;
    Value<float> amount;
    Value<atUint32> unk;
    Value<float> timeIn;
    Value<float> timeOut;
};
}
}

#endif
