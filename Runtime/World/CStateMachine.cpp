#include "CStateMachine.hpp"
#include "CAi.hpp"

namespace urde
{
CStateMachine::CStateMachine(CInputStream& in)
{
#if 0
    u32 stateCount = in.readUint32Big();

    x0_states.reserve(stateCount);

    for (u32 i = 0; i < stateCount; ++i)
    {
        std::string name = in.readString(31);
        CAiStateFunc func = CAi::GetStateFunc(name.c_str());
        x0_states.emplace_back(func, name);
    }

    x10_triggers.reserve(in.readUint32Big());

    for (u32 i = 0; i < stateCount; ++i)
    {
        x0_states[i].SetNumTriggers(in.readUint32Big());
        if (x0_states[i].GetNumTriggers() == 0)
            continue;
        for (u32 j = 0; j < x0_states[i].GetNumTriggers(); ++j)
            x10_triggers.emplace_back();

        for (u32 j = 0; j < x0_states[i].GetNumTriggers(); ++j)
        {
            u32 triggerCount = in.readUint32Big();
            u32 r19 = triggerCount - 1;
            for (u32 k = 0; k < triggerCount; ++k)
            {
                std::string name = in.readString(31);
                CAiTriggerFunc func = CAi::GetTrigerFunc(name.c_str());
                float f31 = in.readFloatBig();
            }
        }
    }
#endif
}

s32 CStateMachine::GetStateIndex(std::string_view state) const
{
    auto it = std::find_if(x0_states.begin(), x0_states.end(), [&state](const CAiState& st) -> bool {
        return (strncmp(st.GetName(), state.data(), 31) == 0);
    });
    if (it == x0_states.end())
        return 0;

    return it - x0_states.begin();
}

const std::vector<CAiState>& CStateMachine::GetStateVector() const { return x0_states; }

float CStateMachineState::GetTime() const { return x8_time; }

void CStateMachineState::SetState(CStateManager &, CAi &, s32 idx)
{
}

void CStateMachineState::SetState(CStateManager& mgr, CAi& ai, const CStateMachine* machine, std::string_view state)
{
    if (!machine)
        return;

    if (!x0_machine)
        x0_machine = machine;

    s32 idx = machine->GetStateIndex(state);
    SetState(mgr, ai, idx);
}

const std::vector<CAiState>* CStateMachineState::GetStateVector() const
{
    if (!x0_machine)
        return nullptr;

    return &x0_machine->GetStateVector();
}

void CStateMachineState::Setup(const CStateMachine* machine)
{
    x0_machine = machine;
    x4_state = nullptr;
    x8_time = 0.f;
    xc_random = 0.f;
    x10_ = 0.f;
}

std::string CStateMachineState::GetName() const { return {}; }

CFactoryFnReturn FAiFiniteStateMachineFactory(const SObjectTag &tag, CInputStream &in, const CVParamTransfer &vparms, CObjectReference *)
{
    return TToken<CStateMachine>::GetIObjObjectFor(std::make_unique<CStateMachine>(in));
}

}
