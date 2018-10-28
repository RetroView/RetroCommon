#include "CScriptGunTurret.hpp"
#include "GameGlobalObjects.hpp"
#include "CSimplePool.hpp"
#include "Particle/CGenDescription.hpp"
#include "Particle/CElementGen.hpp"
#include "Weapon/CGameProjectile.hpp"
#include "World/CGameLight.hpp"
#include "Collision/CCollisionActorManager.hpp"
#include "Collision/CCollisionActor.hpp"
#include "CPlayer.hpp"
#include "Character/CPASAnimParmData.hpp"
#include "Graphics/CBooRenderer.hpp"
#include "TCastTo.hpp"
namespace urde
{

static const CMaterialList skGunMaterialList = { EMaterialTypes::Solid, EMaterialTypes::Character,
                                                 EMaterialTypes::Orbit, EMaterialTypes::Target };
static const CMaterialList skTurretMaterialList = { EMaterialTypes::Character };

CScriptGunTurretData::CScriptGunTurretData(CInputStream& in, s32 propCount)
: x0_(in.readFloatBig()),
  x4_(in.readFloatBig()),
  x8_(in.readFloatBig()),
  xc_(in.readFloatBig()),
  x10_(in.readFloatBig()),
  x14_(in.readFloatBig()),
  x1c_(zeus::degToRad(in.readFloatBig())),
  x20_(zeus::degToRad(in.readFloatBig())),
  x24_(zeus::degToRad(in.readFloatBig())),
  x28_(zeus::degToRad(in.readFloatBig())),
  x2c_(in.readFloatBig()),
  x30_(in.readFloatBig()),
  x34_(in.readFloatBig()),
  x38_(in.readFloatBig()),
  x3c_(propCount >= 48 ? in.readBool() : false),
  x40_projectileRes(in),
  x44_projectileDamage(in),
  x60_(in.readUint32Big()),
  x64_(in.readUint32Big()),
  x68_(in.readUint32Big()),
  x6c_(in.readUint32Big()),
  x70_(in.readUint32Big()),
  x74_(in.readUint32Big()),
  x78_(propCount >= 44 ? in.readUint32Big() : -1),
  x7c_(CSfxManager::TranslateSFXID(in.readUint32Big() & 0xFFFF)),
  x7e_(CSfxManager::TranslateSFXID(in.readUint32Big() & 0xFFFF)),
  x80_unfreezeSound(CSfxManager::TranslateSFXID(in.readUint32Big() & 0xFFFF)),
  x82_(CSfxManager::TranslateSFXID(in.readUint32Big() & 0xFFFF)),
  x84_(CSfxManager::TranslateSFXID(in.readUint32Big() & 0xFFFF)),
  x86_(propCount >= 45 ? CSfxManager::TranslateSFXID(in.readUint32Big() & 0xFFFF) : -1),
  x88_(in.readUint32Big()),
  x8c_(in.readUint32Big()),
  x90_(in.readUint32Big()),
  x94_(in.readUint32Big()),
  x98_(in.readUint32Big()),
  x9c_(propCount >= 47 ? in.readFloatBig() : 3.f),
  xa0_(propCount >= 46 ? in.readBool() : false)
{
}

SBurst CScriptGunTurret::skOOVBurst4InfoTemplate[5] =
{
    {3,  1,  2, -1, -1, 0, 0, 0, 0, 0.15f, 0.05f},
    {3,  7,  6, -1, -1, 0, 0, 0, 0, 0.15f, 0.05f},
    {4,  3,  5, -1, -1, 0, 0, 0, 0, 0.15f, 0.05f},
    {60, 16, 4, -1, -1, 0, 0, 0, 0, 0.15f, 0.05f},
    {30,  4, 4, -1, -1, 0, 0, 0, 0, 0.15f, 0.05f},
};

SBurst CScriptGunTurret::skOOVBurst3InfoTemplate[5] =
{
    {30, 4, 5, 4, -1, 0, 0, 0, 0, 0.15f, 0.05f},
    {30, 2, 3, 4, -1, 0, 0, 0, 0, 0.15f, 0.05f},
    {30, 3, 4, 5, -1, 0, 0, 0, 0, 0.15f, 0.05f},
    {5, 16, 1, 2, -1, 0, 0, 0, 0, 0.15f, 0.05f},
    {5,  8, 7, 6, -1, 0, 0, 0, 0, 0.15f, 0.05f},
};

SBurst CScriptGunTurret::skOOVBurst2InfoTemplate[7] =
{
    {5,  16,  1, 2,  3, 0, 0, 0, 0, 0.15f, 0.05f},
    {5,   9,  8, 7,  6, 0, 0, 0, 0, 0.15f, 0.05f},
    {15,  2,  3, 4,  5, 0, 0, 0, 0, 0.15f, 0.05f},
    {15,  5,  4, 3,  2, 0, 0, 0, 0, 0.15f, 0.05f},
    {15, 10, 11, 4, 13, 0, 0, 0, 0, 0.15f, 0.05f},
    {15, 14, 13, 4, 11, 0, 0, 0, 0, 0.15f, 0.05f},
    {30,  2,  4, 4,  6, 0, 0, 0, 0, 0.15f, 0.05f},
};

SBurst CScriptGunTurret::skBurst4InfoTemplate[5] =
{
    {20, 16, 15, -1, -1, 0, 0, 0, 0, 0.15f, 0.05f},
    {20,  8,  9, -1, -1, 0, 0, 0, 0, 0.15f, 0.05f},
    {20, 13, 11, -1, -1, 0, 0, 0, 0, 0.15f, 0.05f},
    {20,  2,  6, -1, -1, 0, 0, 0, 0, 0.15f, 0.05f},
    {20,  3,  4, -1, -1, 0, 0, 0, 0, 0.15f, 0.05f},
};

SBurst CScriptGunTurret::skBurst3InfoTemplate[5] =
{
    {10, 14,  4, 10, -1, 0, 0, 0, 0, 0.15f, 0.05f},
    {10, 15, 13,  4, -1, 0, 0, 0, 0, 0.15f, 0.05f},
    {10,  9, 11,  4, -1, 0, 0, 0, 0, 0.15f, 0.05f},
    {35, 15, 13, 11, -1, 0, 0, 0, 0, 0.15f, 0.05f},
    {35,  9, 11, 13, -1, 0, 0, 0, 0, 0.15f, 0.05f},
};

SBurst CScriptGunTurret::skBurst2InfoTemplate[6] =
{
    {10, 14, 13,  4, 11, 0, 0, 0, 0, 0.15f, 0.05f},
    {30,  1, 15, 13, 11, 0, 0, 0, 0, 0.15f, 0.05f},
    {20, 16, 15, 14, 13, 0, 0, 0, 0, 0.15f, 0.05f},
    {10,  8,  9, 11,  4, 0, 0, 0, 0, 0.15f, 0.05f},
    {10,  1, 15, 13,  4, 0, 0, 0, 0, 0.15f, 0.05f},
    {20,  8,  9, 10, 11, 0, 0, 0, 0, 0.15f, 0.05f}
};

SBurst* CScriptGunTurret::skBursts[] =
{
    skOOVBurst4InfoTemplate,
    skOOVBurst3InfoTemplate,
    skOOVBurst2InfoTemplate,
    skBurst4InfoTemplate,
    skBurst3InfoTemplate,
    skBurst2InfoTemplate,
    nullptr
};

CScriptGunTurret::CScriptGunTurret(TUniqueId uid, std::string_view name, ETurretComponent comp, const CEntityInfo& info,
                                   const zeus::CTransform& xf, CModelData&& mData, const zeus::CAABox& aabb,
                                   const CHealthInfo& hInfo, const CDamageVulnerability& dVuln,
                                   const CActorParameters& aParms, const CScriptGunTurretData& turretData)
: CPhysicsActor(uid, true, name, info, xf, std::move(mData),
                comp == ETurretComponent::Base ? skTurretMaterialList : skGunMaterialList,
                aabb, SMoverData(1000.f), aParms, 0.3f, 0.1f)
, x258_type(comp)
, x264_healthInfo(hInfo)
, x26c_damageVuln(dVuln)
, x2d4_data(turretData)
, x37c_projectileInfo(turretData.GetProjectileRes(), turretData.GetProjectileDamage())
, x3a4_burstFire(skBursts, 1)
, x410_(g_SimplePool->GetObj({SBIG('PART'), turretData.x60_}))
, x41c_(g_SimplePool->GetObj({SBIG('PART'), turretData.x64_}))
, x428_(g_SimplePool->GetObj({SBIG('PART'), turretData.x68_}))
, x434_(g_SimplePool->GetObj({SBIG('PART'), turretData.x6c_}))
, x440_(g_SimplePool->GetObj({SBIG('PART'), turretData.x70_}))
, x44c_(g_SimplePool->GetObj({SBIG('PART'), turretData.x74_}))
{
    if (turretData.x78_.IsValid())
        x458_ = g_SimplePool->GetObj({SBIG('PART'), turretData.x78_});
    x468_.reset(new CElementGen(x410_));
    x470_.reset(new CElementGen(x41c_));
    x478_.reset(new CElementGen(x428_));
    x480_.reset(new CElementGen(x434_));
    x488_.reset(new CElementGen(x440_));
    x490_.reset(new CElementGen(x44c_));
    x4fc_ = xf.origin;
    x514_ = xf.frontVector();
    x544_ = xf.frontVector();
    x550_ = xf.rightVector();
    x560_24_ = false;
    x560_25_ = false;
    x560_26_ = false;
    x560_27_ = false;
    x560_28_ = false;
    x560_29_ = false;
    x560_30_ = true;
    x560_31_ = false;

    if (comp == ETurretComponent::Base && HasModelData() && GetModelData()->HasAnimData())
        ModelData()->EnableLooping(true);
}

void CScriptGunTurret::Accept(IVisitor& visitor)
{
    visitor.Visit(this);
}

void CScriptGunTurret::AcceptScriptMsg(EScriptObjectMessage msg, TUniqueId uid, CStateManager& mgr)
{
    CActor::AcceptScriptMsg(msg, uid, mgr);

    switch(msg)
    {
    case EScriptObjectMessage::Activate:
        if (x49c_collisionManager)
            x49c_collisionManager->SetActive(mgr, true);
        break;
    case EScriptObjectMessage::Deactivate:
        if (x49c_collisionManager)
            x49c_collisionManager->SetActive(mgr, false);
        break;
    case EScriptObjectMessage::Registered:
        if (x258_type == ETurretComponent::Gun)
        {
            if (x478_->SystemHasLight())
            {
                x498_lightId = mgr.AllocateUniqueId();
                mgr.AddObject(new CGameLight(x498_lightId, GetAreaIdAlways(), GetActive(),
                                             std::string("ParticleLight_") + GetName().data(), GetTransform(),
                                             GetUniqueId(), x478_->GetLight(), 0, 1, 0.f));
            }
            SetupCollisionManager(mgr);
        }
        else if (x258_type == ETurretComponent::Base)
        {
            zeus::CVector3f scale = GetModelData()->GetScale();
            if (x2d4_data.x88_.IsValid())
            {
                CModelData mData(CStaticRes(x2d4_data.x88_, scale));
                x4a4_.emplace(std::move(mData));
                x4f4_ = x4a4_->GetBounds().max.z - x4a4_->GetBounds().min.z;
            }
            sub80219b18(5, mgr);
        }
        break;
    case EScriptObjectMessage::Deleted:
    {
        if (x258_type == ETurretComponent::Gun)
        {
            if (x498_lightId != kInvalidUniqueId)
                mgr.FreeScriptObject(x498_lightId);
        }
        if (x50c_)
            CSfxManager::RemoveEmitter(x50c_);

        if (x49c_collisionManager)
            x49c_collisionManager->Destroy(mgr);
        break;
    }
    case EScriptObjectMessage::Start:
        if (x258_type == ETurretComponent::Base && x520_ == 5)
            x560_29_ = true;
        break;
    case EScriptObjectMessage::Stop:
        if (x258_type == ETurretComponent::Base && x520_ != 1 && x520_ != 2 && x520_ != 3)
            sub80219b18((!x560_28_ ? 3 : 4), mgr);
        break;
    case EScriptObjectMessage::Action:
    {
        if (x258_type == ETurretComponent::Gun)
            sub80217408(mgr);
        else if (x258_type == ETurretComponent::Base)
            sub802172b8(mgr);
        break;
    }
    case EScriptObjectMessage::SetToMax:
    {
        x560_25_ = false;
        SetMuted(false);
        break;
    }
    case EScriptObjectMessage::SetToZero:
    {
        x560_25_ = true;
        SetMuted(true);
        break;
    }
    case EScriptObjectMessage::InitializedInArea:
    {
        if (x258_type == ETurretComponent::Base)
        {
            for (const SConnection& conn : x20_conns)
            {
                if (conn.x0_state != EScriptObjectState::Play || conn.x4_msg != EScriptObjectMessage::Activate)
                    continue;

                if (TCastToConstPtr<CScriptGunTurret> gun = mgr.GetObjectById(mgr.GetIdForScript(conn.x8_objId)))
                {
                    x25c_ = mgr.GetIdForScript(conn.x8_objId);
                    x260_ = gun->GetHealthInfo(mgr)->GetHP();
                    return;
                }
            }
        }
        break;
    }
    case EScriptObjectMessage::Damage:
    {
        if (x258_type == ETurretComponent::Gun && GetHealthInfo(mgr)->GetHP() > 0.f)
        {
            if (TCastToConstPtr<CGameProjectile> proj = mgr.GetObjectById(uid))
            {
                if ((proj->GetAttribField() & EProjectileAttrib::Wave) == EProjectileAttrib::Wave)
                {
                    x520_ = 12;
                    RemoveMaterial(EMaterialTypes::Target, EMaterialTypes::Orbit, mgr);
                    mgr.GetPlayer().SetOrbitRequestForTarget(GetUniqueId(),
                                                             CPlayer::EPlayerOrbitRequest::ActivateOrbitSource, mgr);
                    x53c_ = 0.f;
                }
            }
        }
        break;
    }
    default:
        break;
    }
}

void CScriptGunTurret::Think(float dt, CStateManager& mgr)
{
    if (!GetActive())
        return;

    if (x258_type == ETurretComponent::Base)
    {
        if (!x560_25_)
        {
            sub80219a00(dt, mgr);
            sub802189c8();
            sub80217f5c(dt, mgr);
            zeus::CVector3f vec = sub80217e34(dt);
            SAdvancementDeltas advancementDeltas = UpdateAnimation(dt, mgr, true);
            SetTranslation(vec + advancementDeltas.x0_posDelta + GetTranslation());
            RotateToOR(advancementDeltas.xc_rotDelta, dt);
        } else
            Stop();

        sub80216288(dt);
    }
    else if (x258_type == ETurretComponent::Gun)
    {
        UpdatGunParticles(dt, mgr);
        SAdvancementDeltas deltas = UpdateAnimation(dt, mgr, true);
        MoveToOR(deltas.x0_posDelta, dt);
        RotateToOR(deltas.xc_rotDelta, dt);
        UpdateGunCollisionManager(dt, mgr);
        GetUnFreezeSoundId(dt, mgr);
    }
}

void CScriptGunTurret::Touch(CActor& act, CStateManager& mgr)
{
    if (x258_type != ETurretComponent::Gun)
        return;
    if (TCastToPtr<CGameProjectile> proj = act)
    {
        const CPlayer& player = mgr.GetPlayer();
        if (proj->GetOwnerId() == player.GetUniqueId())
        {
            const CDamageVulnerability* dVuln = GetDamageVulnerability();
            if (!x560_24_ && x520_ != 12 && (proj->GetAttribField() & EProjectileAttrib::Ice) == EProjectileAttrib::Ice
                && dVuln->WeaponHits(CWeaponMode::Ice(), false))
            {
                x560_24_ = true;
                SendScriptMsgs(EScriptObjectState::Zero, mgr, EScriptObjectMessage::None);
                x53c_ = mgr.GetActiveRandom()->Float() * x2d4_data.x38_ + x2d4_data.x34_;
                SetMuted(true);
            }
            SendScriptMsgs(EScriptObjectState::Damage, mgr, EScriptObjectMessage::None);
        }
    }
}

std::experimental::optional<zeus::CAABox> CScriptGunTurret::GetTouchBounds() const
{
    if (GetActive() && GetMaterialList().HasMaterial(EMaterialTypes::Solid))
        return {GetBoundingBox()};
    return {};
}

zeus::CVector3f CScriptGunTurret::GetOrbitPosition(const CStateManager& mgr) const
{
    return GetAimPosition(mgr, 0.f);
}

zeus::CVector3f CScriptGunTurret::GetAimPosition(const CStateManager &, float) const
{
    if (x258_type == ETurretComponent::Base)
        return GetTranslation() + x34_transform.rotate(GetLocatorTransform("Gun_SDK"sv).origin);

    return GetTranslation();
}

void CScriptGunTurret::SetupCollisionManager(CStateManager&)
{

}

void CScriptGunTurret::sub80219b18(s32 w1, CStateManager& mgr)
{

    if (w1 < 0 || w1 > 12)
        return;

    if (x520_ != -1)
        sub8021998c(2, mgr, 0.f);


    x520_ = w1;
    x524_ = 0.f;
    sub8021998c(0, mgr, 0.f);
}

void CScriptGunTurret::sub80217408(CStateManager&)
{

}

void CScriptGunTurret::sub802172b8(CStateManager&)
{

}

void CScriptGunTurret::AddToRenderer(const zeus::CFrustum& frustum, const CStateManager& mgr)
{
    CActor::AddToRenderer(frustum, mgr);

    if (x258_type != ETurretComponent::Gun)
        return;

    if (!x560_25_)
    {
        if (x520_ == 6 || (x520_ >= 9 && x520_ <= 12))
        {
            g_Renderer->AddParticleGen(*x478_);
            if (x520_ == 10 || x520_ == 12)
                g_Renderer->AddParticleGen(*x488_);
        }
        else if (x520_ == 5)
        {
            g_Renderer->AddParticleGen(*x468_);
        }
        else if (x520_ >= 1)
        {
            g_Renderer->AddParticleGen(*x470_);
        }
    }
    else
    {
        g_Renderer->AddParticleGen(*x480_);
    }
}

void CScriptGunTurret::Render(const CStateManager& mgr) const
{
    CPhysicsActor::Render(mgr);

    if (x258_type == ETurretComponent::Gun)
    {
        if (!x560_25_)
        {
            if (x520_ == 6 || (x520_ >= 9 && x520_ <= 12))
            {
                x478_->Render(x90_actorLights.get());
                if (x520_ == 10)
                    x488_->Render(x90_actorLights.get());
            }
            else if (x520_ == 5)
                x468_->Render(x90_actorLights.get());
            else if (x520_ >= 1)
                x470_->Render(x90_actorLights.get());
        }
    }
    else if (x258_type == ETurretComponent::Base)
    {
        if (x4a4_ && x4f8_ > 0.f)
        {
            zeus::CTransform xf = GetTransform();
            xf.origin = x4fc_ + (x4f4_ * 0.5f * zeus::CVector3f::skDown);
            CModelFlags flags;
            flags.x2_flags = 3;
            flags.x1_matSetIdx = 0;
            flags.x4_color = zeus::CColor::skWhite;
            x4a4_->Render(mgr, xf, x90_actorLights.get(), flags);
        }

    }
}

void CScriptGunTurret::UpdateGunCollisionManager(float dt, CStateManager& mgr)
{
    if (TCastToPtr<CCollisionActor> colAct = mgr.ObjectById(x4a0_))
        colAct->SetActive(mgr.GetPlayer().GetMorphballTransitionState() == CPlayer::EPlayerMorphBallState::Unmorphed);

    x49c_collisionManager->Update(dt, mgr, CCollisionActorManager::EUpdateOptions::Zero);
}

void CScriptGunTurret::GetUnFreezeSoundId(float dt, CStateManager& mgr)
{
    if (x560_25_)
    {
        if (x53c_ <= 0.f)
        {
            SendScriptMsgs(EScriptObjectState::UnFrozen, mgr, EScriptObjectMessage::None);
            CSfxManager::AddEmitter(x2d4_data.x80_unfreezeSound, GetTranslation(), zeus::CVector3f::skUp, false, false,
                                    0x7f, GetAreaIdAlways());
            SetMuted(false);
        }
        else if (x2d4_data.x3c_)
            x53c_ -= dt;
    }
    else
        x53c_ = 0.f;
}

void CScriptGunTurret::UpdatGunParticles(float dt, CStateManager& mgr)
{
    CGameLight* light = nullptr;
    if (x498_lightId != kInvalidUniqueId)
        light = TCastToPtr<CGameLight>(mgr.ObjectById(x498_lightId));

    if (!x560_25_)
    {
        zeus::CTransform lightXf = GetLocatorTransform("light_LCTR"sv);
        zeus::CVector3f pos = x34_transform.rotate(lightXf.origin);
        pos += GetTranslation();
        if (light)
            light->SetActive(true);

        if (x520_ == 6 || (x520_ >= 9 && x520_ <= 12))
        {
            bool doEmission = false;
            if (x520_ != 10 && x520_ != 12)
                doEmission = true;

            x468_->SetParticleEmission(false);
            x470_->SetParticleEmission(false);
            x478_->SetParticleEmission(true);
            x480_->SetParticleEmission(false);
            x488_->SetParticleEmission(doEmission);
            x478_->SetOrientation(GetTransform().getRotation());
            x478_->SetGlobalTranslation(pos);
            x478_->SetGlobalScale(GetModelData()->GetScale());
            x478_->Update(dt);
            if (x478_->SystemHasLight())
                light->SetLight(x478_->GetLight());
            else
                light->SetActive(false);

            if (doEmission)
            {
                zeus::CTransform blastXf = GetLocatorTransform("Blast_LCTR"sv);
                zeus::CVector3f blastPos = GetTransform().rotate(blastXf.origin);
                blastPos += GetTranslation();
                x488_->SetOrientation(GetTransform().getRotation());
                x488_->SetGlobalTranslation(blastPos);
                x488_->SetGlobalScale(GetModelData()->GetScale());
                x488_->Update(dt);
            }
        }
        else if (x520_ == 5)
        {
            x468_->SetParticleEmission(true);
            x470_->SetParticleEmission(false);
            x478_->SetParticleEmission(false);
            x480_->SetParticleEmission(false);
            x488_->SetParticleEmission(false);
            x490_->SetParticleEmission(false);
            x468_->SetOrientation(GetTransform().getRotation());
            x468_->SetGlobalTranslation(pos);
            x468_->SetGlobalScale(GetModelData()->GetScale());
        }
        else if (x520_ > 0 && x520_ < 5)
        {
            x468_->SetParticleEmission(false);
            x470_->SetParticleEmission(true);
            x478_->SetParticleEmission(false);
            x480_->SetParticleEmission(false);
            x488_->SetParticleEmission(false);
            x490_->SetParticleEmission(false);
            x470_->SetOrientation(GetTransform().getRotation());
            x470_->SetGlobalTranslation(pos);
            x470_->SetGlobalScale(GetModelData()->GetScale());
            x470_->Update(dt);
            if (light && x470_->SystemHasLight())
                light->SetLight(x470_->GetLight());
        }
        else
        {
            x468_->SetParticleEmission(false);
            x470_->SetParticleEmission(false);
            x478_->SetParticleEmission(false);
            x480_->SetParticleEmission(false);
            x488_->SetParticleEmission(false);
            x490_->SetParticleEmission(false);
            x480_->SetOrientation(GetTransform().getRotation());
            x480_->SetGlobalTranslation(GetTranslation());
            x480_->SetGlobalScale(GetModelData()->GetScale());
            x480_->Update(dt);
            if (light)
                light->SetActive(false);
        }
    }
    else
    {
        x468_->SetParticleEmission(false);
        x470_->SetParticleEmission(false);
        x478_->SetParticleEmission(false);
        x480_->SetParticleEmission(false);
        x488_->SetParticleEmission(false);
        x490_->SetParticleEmission(false);
        x480_->SetOrientation(GetTransform().getRotation());
        x480_->SetGlobalTranslation(GetTranslation());
        x480_->SetGlobalScale(GetModelData()->GetScale());
        x480_->Update(dt);
        if (light)
            light->SetActive(false);
    }
}

void CScriptGunTurret::sub80219a00(float dt, CStateManager& mgr)
{
    sub80219b18(1, mgr);
    x524_ += dt;
    sub80217124(mgr);
    if (x25c_ != kInvalidUniqueId)
    {
        if (TCastToPtr<CScriptGunTurret> gunTurret = mgr.ObjectById(x25c_))
        {
            if (gunTurret->x520_ != 12)
                gunTurret->x520_ = x520_;
            else if (x520_ != 12)
            {
                sub80219b18(12, mgr);
                gunTurret->RemoveMaterial(EMaterialTypes::Target, EMaterialTypes::Orbit, mgr);
                mgr.GetPlayer().SetOrbitRequestForTarget(GetUniqueId(),
                                                         CPlayer::EPlayerOrbitRequest::ActivateOrbitSource, mgr);
            }
        }
    }
}

void CScriptGunTurret::sub802189c8()
{
    if (!HasModelData() || !GetModelData()->HasAnimData())
        return;

    if (x520_ > 12)
        return;

    static const u32 animIds[13] = {5, 7, 9, 0, 1, 0, 1, 2, 3, 1, 1, 1, 1};
    CPASAnimParmData parmData = CPASAnimParmData(5, CPASAnimParm::FromEnum(0), CPASAnimParm::FromEnum(animIds[x520_]));
    auto pair = GetModelData()->GetAnimationData()->GetCharacterInfo().GetPASDatabase().FindBestAnimation(parmData, -1);

    if (pair.first > 0.f && pair.second != x540_)
    {
        ModelData()->AnimationData()->SetAnimation(CAnimPlaybackParms(pair.second, -1, 1.f, true), false);
        ModelData()->AnimationData()->EnableLooping(true);
        x540_ = pair.second;
    }

}

void CScriptGunTurret::sub8021998c(s32 w1, CStateManager& mgr, float dt)
{
    switch(x520_)
    {
    case 3:
    case 4:
        sub80219938(w1, mgr);
        break;
    case 5:
        sub802196c4(w1, mgr, dt);
        break;
    case 6:
        sub802195bc(w1, mgr, dt);
        break;
    case 7:
    case 8:
        sub8021942c(w1, mgr, dt);
        break;
    case 9:
    case 10:
        sub80218f50(w1, mgr, dt);
        break;
    case 11:
        sub80218e34(w1, mgr, dt);
        break;
    case 12:
        sub80218bb4(w1, mgr, dt);
        break;
    default:
        break;
    }
}

void CScriptGunTurret::sub80219938(s32 w1, CStateManager& mgr)
{
    if (w1 == 1)
    {
        float f1 = x524_;
        float f0 = x2d4_data.x0_;
        if (f1 >= f0 && x560_28_)
            w1 = 2;

        sub80219b18(w1, mgr);
    }
}

void CScriptGunTurret::sub802196c4(s32 w1, CStateManager& mgr, float dt)
{
    if (w1 == 0)
    {
        x528_ = 0.f;
        x560_27_ = false;
        if (TCastToPtr<CScriptGunTurret> gunTurret = mgr.ObjectById(x25c_))
            x260_ = gunTurret->HealthInfo(mgr)->GetHP();
    }
    else if (w1 == 1)
    {
    }
    else if (w1 == 2)
    {
        x560_28_ = true;
        x468_->SetParticleEmission(false);

        if (TCastToPtr<CScriptGunTurret> gunTurret = mgr.ObjectById(x25c_))
            x260_ = gunTurret->GetHealthInfo(mgr)->GetHP();
    }
}

void CScriptGunTurret::sub802195bc(s32 w1, CStateManager& mgr, float dt)
{
    if (w1 == 0)
    {
        x52c_ = 0.f;
    }
    else if (w1 == 1)
    {
        x52c_ += dt;
        if (x52c_ < x2d4_data.x10_)
            return;

        if (sub80217ad8(mgr) && sub802179a4(mgr))
        {
            sub80219b18(9, mgr);
            CSfxManager::AddEmitter(x2d4_data.x7e_, GetTranslation(), zeus::CVector3f::skUp, false, false, 0x7f,
                                    GetAreaIdAlways());
        }
        else
        {
            sub80219b18(7, mgr);
            x530_ = 0.f;
        }
    }
}

void CScriptGunTurret::sub8021942c(s32 state, CStateManager& mgr, float dt)
{
    if (state == 0)
    {
        x52c_ = 0.f;
    }
    else if (state == 1)
    {
        if (sub80217ad8(mgr) && sub802179a4(mgr))
        {
            sub80219b18(9, mgr);
            CSfxManager::AddEmitter(x2d4_data.x7e_, GetTranslation(), zeus::CVector3f::skUp, false, false, 0x7f,
                                    GetAreaIdAlways());
        }
        else
        {
            x52c_ += dt;
            x530_ += dt;
            if (x530_ >= x2d4_data.x18_ && !x4a4_ && !x2d4_data.xa0_)
                sub80219b18(5, mgr);
            else if (x52c_ >= x2d4_data.x14_)
                sub80219b18(x520_ != 7 ? 7 : 8, mgr);
        }
    }
}

void CScriptGunTurret::sub80218f50(s32 state, CStateManager& mgr, float dt)
{
    if (state == 0)
    {
        x52c_ = 0.f;
    }
    else if (state == 1)
    {
        if (!x560_26_)
        {
            if (sub802179a4(mgr))
            {
                sub80218830(dt, mgr);
                if (x25c_ != kInvalidUniqueId)
                {
                    if (TCastToPtr<CScriptGunTurret> gun = mgr.ObjectById(x25c_))
                    {
                        zeus::CVector3f vec = x404_;
                        if (sub80217ad8(mgr))
                        {
                            zeus::CTransform blastXf = gun->GetLocatorTransform("Blast_LCTR"sv);
                            zeus::CVector3f rotatedBlastVec = GetTransform().rotate(blastXf.origin) + GetTranslation();
                            x404_ = mgr.GetPlayer().GetAimPosition(mgr, 0.f);
                            vec = x37c_projectileInfo.PredictInterceptPos(rotatedBlastVec,
                                                                          mgr.GetPlayer().GetAimPosition(mgr, dt),
                                                                          mgr.GetPlayer(), false);
                        }

                        zeus::CVector3f compensated = x3a4_burstFire.GetDistanceCompensatedError(
                            (x404_ - gun->GetTranslation()).magnitude(), 20.f);

                        compensated = gun->GetTransform().rotate(compensated);

                        gun->x404_ = x404_ + (vec - x404_) + compensated;
                    }
                }

                zeus::CVector3f diffVec = x404_ - GetTranslation();
                if (diffVec.canBeNormalized())
                {
                    zeus::CVector3f normDiff = diffVec.normalized();
                    float angDif = zeus::CVector3f::getAngleDiff(normDiff, GetTransform().frontVector());
                    zeus::CQuaternion quat = zeus::CQuaternion::lookAt(GetTransform().frontVector(), normDiff,
                                                                       std::min(angDif, (dt * x2d4_data.x28_)));

                    quat.setImaginary(GetTransform().transposeRotate(quat.getImaginary()));
                    RotateInOneFrameOR(quat, dt);
                }
            }

            if (sub80217950(mgr))
            {
                SendScriptMsgs(EScriptObjectState::Attack, mgr, EScriptObjectMessage::None);
                x560_26_ = true;
            }

            x52c_ = 0.f;
        }
        else
        {
            x52c_ += dt;
            if (x52c_ >= 10.f)
                sub80219b18(11, mgr);
        }
    }
    else if (state == 2)
    {
        x560_30_ = true;
    }
}

void CScriptGunTurret::sub80218e34(s32, CStateManager&, float)
{

}

void CScriptGunTurret::sub80218bb4(s32, CStateManager&, float)
{

}

bool CScriptGunTurret::sub80217ad8(CStateManager&)
{
    return false;
}

bool CScriptGunTurret::sub802179a4(CStateManager&)
{
    return false;
}

zeus::CVector3f CScriptGunTurret::sub80217e34(float dt)
{
    if (!x4a4_)
        return {};

    if (x520_ >= 7 && x520_ < 12)
        x4f8_ = std::min(0.9f, x4f8_ + 1.5f * dt);
    else if ((x520_ >= 0 && x520_ < 3) || x520_ == 5 || x520_ == 13)
        x4f8_ = std::max(0.f, x4f8_ - 1.f * dt);

    return (x4fc_ + (x2d4_data.x8c_ * x4f8_ * zeus::CVector3f::skDown)) - GetTranslation();
}

void CScriptGunTurret::sub80217f5c(float dt, CStateManager& mgr)
{
    /* TODO: Finish */
    if (x25c_ == kInvalidUniqueId)
        return;

    if (TCastToPtr<CScriptGunTurret> gun = mgr.ObjectById(x25c_))
    {
        zeus::CTransform xf = GetLocatorTransform("Gun_SDK"sv);
        xf = GetTransform() * xf;

        switch(x520_)
        {
        case 11:
        {
            zeus::CVector3f frontVec = GetTransform().frontVector();
            zeus::CVector3f gunFrontVec = gun->GetTransform().frontVector();
            zeus::CQuaternion quat = zeus::CQuaternion::lookAt(gunFrontVec, frontVec, 0.3f * dt * x2d4_data.x28_);
            zeus::CVector3f xposVec = gun->GetTransform().transposeRotate(quat.getImaginary());
            quat.setImaginary(xposVec);
            gun->RotateInOneFrameOR(quat, dt);
            RotateInOneFrameOR(quat, dt);
            break;
        }
        case 12:
            break;
        default:
            gun->SetTransform(xf);
            break;
        }
    }
}

void CScriptGunTurret::sub80216288(float)
{

}

void CScriptGunTurret::sub80217124(CStateManager&)
{

}

void CScriptGunTurret::sub80218830(float dt, CStateManager& mgr)
{
    if (mgr.GetCameraManager()->IsInCinematicCamera())
    {
        x534_ = mgr.GetActiveRandom()->Float() * x2d4_data.xc_ + x2d4_data.x8_;
        x538_ = 0.5f * x534_;
    }

    if (x534_ > 0.f)
    {
        x534_ -= dt;
        if (x534_ < x538_ && x520_ != 10)
        {
            CSfxManager::AddEmitter(x2d4_data.x84_, GetTranslation(), zeus::CVector3f::skUp, false, false, 0x7f, GetAreaIdAlways());
            sub80219b18(10, mgr);
            return;
        }

        if (x520_ != 9)
            sub80219b18(9, mgr);

        if (x2d4_data.x18_ == 0)
        {
            sub80216594(mgr);
            x534_ = mgr.GetActiveRandom()->Float() * x2d4_data.xc_ + x2d4_data.x8_;
            x538_ = 0.5f * x534_;
            return;
        }

        x3a4_burstFire.Update(mgr, dt);
    }
}

void CScriptGunTurret::sub80216594(CStateManager&)
{

}

bool CScriptGunTurret::sub80217950(CStateManager& mgr)
{
    if (x520_ == 9 && x534_ <= 0.f && x3a4_burstFire.GetX14_24())
        return sub80217ad8(mgr);

    return false;
}


}
