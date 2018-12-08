#pragma once

#include "CGunWeapon.hpp"

namespace urde {

class CWaveBeam final : public CGunWeapon {
  TCachedToken<CWeaponDescription> x21c_waveBeam;
  TCachedToken<CElectricDescription> x228_wave2nd1;
  TCachedToken<CElectricDescription> x234_wave2nd2;
  TCachedToken<CGenDescription> x240_wave2nd3;
  float x24c_effectTimer = 0.f;
  std::unique_ptr<CParticleElectric> x250_chargeElec;
  std::unique_ptr<CElementGen> x254_chargeFx;
  bool x258_24_loaded : 1;
  bool x258_25_effectTimerActive : 1;
  void ReInitVariables();

public:
  CWaveBeam(CAssetId characterId, EWeaponType type, TUniqueId playerId, EMaterialTypes playerMaterial,
            const zeus::CVector3f& scale);

  void PostRenderGunFx(const CStateManager& mgr, const zeus::CTransform& xf);
  void UpdateGunFx(bool shotSmoke, float dt, const CStateManager& mgr, const zeus::CTransform& xf);
  void Fire(bool underwater, float dt, EChargeState chargeState, const zeus::CTransform& xf, CStateManager& mgr,
            TUniqueId homingTarget, float chargeFactor1, float chargeFactor2);
  void EnableSecondaryFx(ESecondaryFxType type);
  void Update(float dt, CStateManager& mgr);
  void Load(CStateManager& mgr, bool subtypeBasePose);
  void Unload(CStateManager& mgr);
  bool IsLoaded() const;
};

} // namespace urde
