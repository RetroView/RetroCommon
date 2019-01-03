#include "boo/System.hpp"
#include "GameGlobalObjects.hpp"
#include "CBooRenderer.hpp"
#include "CModel.hpp"
#include "Particle/CParticleGen.hpp"
#include "Particle/CGenDescription.hpp"
#include "Particle/CDecal.hpp"
#include "Particle/CElementGen.hpp"
#include "CMetroidModelInstance.hpp"
#include "Collision/CAreaOctTree.hpp"
#include "zeus/CUnitVector.hpp"
#include "Graphics/CSkinnedModel.hpp"
#include "zeus/CVector3d.hpp"
#include "World/CActor.hpp"

#define FOGVOL_RAMP_RES 256
#define FOGVOL_FAR 750.0
#define FOGVOL_NEAR 0.2
#define SPHERE_RAMP_RES 32

namespace urde {

static logvisor::Module Log("CBooRenderer");

static rstl::reserved_vector<CDrawable, 512> sDataHolder;
static rstl::reserved_vector<rstl::reserved_vector<CDrawable*, 128>, 50> sBucketsHolder;
static rstl::reserved_vector<CDrawablePlaneObject, 8> sPlaneObjectDataHolder;
static rstl::reserved_vector<u16, 8> sPlaneObjectBucketHolder;

rstl::reserved_vector<u16, 50> Buckets::sBucketIndex;
rstl::reserved_vector<CDrawable, 512>* Buckets::sData = nullptr;
rstl::reserved_vector<rstl::reserved_vector<CDrawable*, 128>, 50>* Buckets::sBuckets = nullptr;
rstl::reserved_vector<CDrawablePlaneObject, 8>* Buckets::sPlaneObjectData = nullptr;
rstl::reserved_vector<u16, 8>* Buckets::sPlaneObjectBucket = nullptr;
const float Buckets::skWorstMinMaxDistance[2] = {99999.f, -99999.f};
float Buckets::sMinMaxDistance[2];

void Buckets::Clear() {
  sData->clear();
  sBucketIndex.clear();
  sPlaneObjectData->clear();
  sPlaneObjectBucket->clear();
  for (rstl::reserved_vector<CDrawable*, 128>& bucket : *sBuckets)
    bucket.clear();
  sMinMaxDistance[0] = skWorstMinMaxDistance[0];
  sMinMaxDistance[1] = skWorstMinMaxDistance[1];
}

void Buckets::Sort() {
  float delta = std::max(1.f, sMinMaxDistance[1] - sMinMaxDistance[0]);
  float pitch = 49.f / delta;
  for (auto it = sPlaneObjectData->begin(); it != sPlaneObjectData->end(); ++it)
    if (sPlaneObjectBucket->size() != sPlaneObjectBucket->capacity())
      sPlaneObjectBucket->push_back(s16(it - sPlaneObjectData->begin()));

  u32 precision = 50;
  if (sPlaneObjectBucket->size()) {
    std::sort(sPlaneObjectBucket->begin(), sPlaneObjectBucket->end(),
              [](u16 a, u16 b) { return (*sPlaneObjectData)[a].GetDistance() > (*sPlaneObjectData)[b].GetDistance(); });
    precision = 50 / u32(sPlaneObjectBucket->size() + 1);
    pitch = 1.f / (delta / float(precision - 2));

    int accum = 0;
    for (u16 idx : *sPlaneObjectBucket) {
      ++accum;
      CDrawablePlaneObject& planeObj = (*sPlaneObjectData)[idx];
      planeObj.x24_targetBucket = u16(precision * accum);
    }
  }

  for (CDrawable& drawable : *sData) {
    int slot;
    float relDist = drawable.GetDistance() - sMinMaxDistance[0];
    if (sPlaneObjectBucket->empty()) {
      slot = zeus::clamp(1, int(relDist * pitch), 49);
    } else {
      slot = zeus::clamp(0, int(relDist * pitch), int(precision) - 2);
      for (u16 idx : *sPlaneObjectBucket) {
        CDrawablePlaneObject& planeObj = (*sPlaneObjectData)[idx];
        bool partial, full;
        if (planeObj.x3c_25_zOnly) {
          partial = drawable.GetBounds().max.z() > planeObj.GetPlane().d();
          full = drawable.GetBounds().min.z() > planeObj.GetPlane().d();
        } else {
          partial = planeObj.GetPlane().pointToPlaneDist(
                        drawable.GetBounds().closestPointAlongVector(planeObj.GetPlane().normal())) > 0.f;
          full = planeObj.GetPlane().pointToPlaneDist(
                     drawable.GetBounds().furthestPointAlongVector(planeObj.GetPlane().normal())) > 0.f;
        }
        bool cont;
        if (drawable.GetType() == EDrawableType::Particle)
          cont = planeObj.x3c_24_invertTest ? !partial : full;
        else
          cont = planeObj.x3c_24_invertTest ? (!partial || !full) : (partial || full);
        if (!cont)
          break;
        slot += precision;
      }
    }

    if (slot == -1)
      slot = 49;
    rstl::reserved_vector<CDrawable*, 128>& bucket = (*sBuckets)[slot];
    if (bucket.size() < bucket.capacity())
      bucket.push_back(&drawable);
    // else
    //    Log.report(logvisor::Fatal, "Full bucket!!!");
  }

  u16 bucketIdx = u16(sBuckets->size());
  for (auto it = sBuckets->rbegin(); it != sBuckets->rend(); ++it) {
    --bucketIdx;
    sBucketIndex.push_back(bucketIdx);
    rstl::reserved_vector<CDrawable*, 128>& bucket = *it;
    if (bucket.size()) {
      std::sort(bucket.begin(), bucket.end(), [](CDrawable* a, CDrawable* b) {
        if (a->GetDistance() == b->GetDistance())
          return a->GetExtraSort() > b->GetExtraSort();
        return a->GetDistance() > b->GetDistance();
      });
    }
  }

  for (auto it = sPlaneObjectBucket->rbegin(); it != sPlaneObjectBucket->rend(); ++it) {
    CDrawablePlaneObject& planeObj = (*sPlaneObjectData)[*it];
    rstl::reserved_vector<CDrawable*, 128>& bucket = (*sBuckets)[planeObj.x24_targetBucket];
    bucket.push_back(&planeObj);
  }
}

void Buckets::InsertPlaneObject(float closeDist, float farDist, const zeus::CAABox& aabb, bool invertTest,
                                const zeus::CPlane& plane, bool zOnly, EDrawableType dtype, const void* data) {
  if (sPlaneObjectData->size() == sPlaneObjectData->capacity())
    return;
  sPlaneObjectData->push_back(CDrawablePlaneObject(dtype, closeDist, farDist, aabb, invertTest, plane, zOnly, data));
}

void Buckets::Insert(const zeus::CVector3f& pos, const zeus::CAABox& aabb, EDrawableType dtype, const void* data,
                     const zeus::CPlane& plane, u16 extraSort) {
  if (sData->size() != sData->capacity()) {
    float dist = plane.pointToPlaneDist(pos);
    sData->push_back(CDrawable(dtype, extraSort, dist, aabb, data));
    if (sMinMaxDistance[0] > dist)
      sMinMaxDistance[0] = dist;
    if (sMinMaxDistance[1] < dist)
      sMinMaxDistance[1] = dist;
  } else {
    Log.report(logvisor::Fatal, "Rendering buckets filled to capacity");
  }
}

void Buckets::Shutdown() {
  sData = nullptr;
  sBuckets = nullptr;
  sPlaneObjectData = nullptr;
  sPlaneObjectBucket = nullptr;
}

void Buckets::Init() {
  sData = &sDataHolder;
  sBuckets = &sBucketsHolder;
  sBuckets->resize(50);
  sPlaneObjectData = &sPlaneObjectDataHolder;
  sPlaneObjectBucket = &sPlaneObjectBucketHolder;
  sMinMaxDistance[0] = skWorstMinMaxDistance[0];
  sMinMaxDistance[1] = skWorstMinMaxDistance[1];
}

CBooRenderer::CAreaListItem::CAreaListItem(const std::vector<CMetroidModelInstance>* geom,
                                           const CAreaRenderOctTree* octTree,
                                           std::vector<TCachedToken<CTexture>>&& textures,
                                           std::vector<CBooModel*>&& models, int areaIdx, const SShader* shaderSet)
: x0_geometry(geom)
, x4_octTree(octTree)
, x8_textures(std::move(textures))
, x10_models(std::move(models))
, x18_areaIdx(areaIdx)
, m_shaderSet(shaderSet) {}

CBooRenderer::CAreaListItem::~CAreaListItem() {}

static inline bool TestBit(const u32* words, int bit) { return (words[bit / 32] & (1 << (bit & 0x1f))) != 0; }

void CBooRenderer::ActivateLightsForModel(CAreaListItem* item, CBooModel& model) {
  std::vector<CLight> thisLights;
  thisLights.reserve(4);

  if (x300_dynamicLights.size()) {
    u32 lightOctreeWordCount = 0;
    u32* lightOctreeWords = nullptr;
    if (item && model.x44_areaInstanceIdx != -1) {
      lightOctreeWordCount = item->x4_octTree->x14_bitmapWordCount;
      lightOctreeWords = item->x1c_lightOctreeWords.data();
    }

    float lightRads[4] = {-1.f, -1.f, -1.f, -1.f};
    CLight* lightRefs[4] = {};
    auto it = x300_dynamicLights.begin();
    for (int i = 0; i < 4 && it != x300_dynamicLights.end(); ++it, lightOctreeWords += lightOctreeWordCount) {
      CLight& refLight = *it;
      if (lightOctreeWords && !TestBit(lightOctreeWords, model.x44_areaInstanceIdx))
        continue;

      bool foundLight = false;
      for (int j = 0; j < i; ++j) {
        if (lightRefs[j] == &refLight)
          continue;
        float radius = model.x20_aabb.intersectionRadius(zeus::CSphere(refLight.GetPosition(), refLight.GetRadius()));
        if (radius < 0.f)
          break;
        if (lightRads[j] <= radius)
          break;
        lightRads[j] = radius;
        lightRefs[j] = &refLight;
        thisLights.push_back(refLight);
        foundLight = true;
      }

      if (foundLight)
        continue;

      float radius = model.x20_aabb.intersectionRadius(zeus::CSphere(refLight.GetPosition(), refLight.GetRadius()));
      if (radius < 0.f)
        continue;
      lightRads[i] = radius;
      lightRefs[i] = &refLight;
      thisLights.push_back(refLight);
      ++i;
    }
  }

  model.ActivateLights(thisLights);
}

void CBooRenderer::RenderBucketItems(CAreaListItem* item) {
  CModelFlags flags;
  flags.m_noZWrite = true;
  flags.m_extendedShader = EExtendedShader::Lighting;

  for (u16 idx : Buckets::sBucketIndex) {
    rstl::reserved_vector<CDrawable*, 128>& bucket = (*Buckets::sBuckets)[idx];
    for (CDrawable* drawable : bucket) {
      switch (drawable->GetType()) {
      case EDrawableType::Particle: {
        static_cast<CParticleGen*>((void*)drawable->GetData())->Render();
        break;
      }
      case EDrawableType::WorldSurface: {
        // SetupRendererStates();
        CBooSurface* surf = static_cast<CBooSurface*>((void*)drawable->GetData());
        CBooModel* model = surf->m_parent;
        if (model) {
          ActivateLightsForModel(item, *model);
          model->DrawSurface(*surf, flags);
        }
        break;
      }
      default: {
        if (xa8_drawableCallback) {
          xa8_drawableCallback(drawable->GetData(), xac_callbackContext, int(drawable->GetType()) - 2);
        }
        break;
      }
      }
    }
  }
}

void CBooRenderer::HandleUnsortedModel(CAreaListItem* item, CBooModel& model, const CModelFlags& flags) {
  // ActivateLightsForModel(item, model);
  CBooSurface* surf = model.x38_firstUnsortedSurface;
  while (surf) {
    model.DrawSurface(*surf, flags);
    surf = surf->m_next;
  }
}

static const struct FogVolumeControl {

  u32 xfc_[12][2] = {{0, 1}, {1, 3}, {3, 2}, {2, 0}, {4, 5}, {5, 7}, {7, 6}, {6, 4}, {0, 4}, {1, 5}, {3, 7}, {2, 6}};
  u32 x15c_[8] = {};
  // GXVtxDescList x17c_; {{POS, DIRECT}, {TEX0, DIRECT}}

} s_FogVolumeCtrl = {};

static const int OrthogonalAxis[3][2] = {{1, 2}, {0, 2}, {0, 1}};

static float GetPlaneInterpolant(const zeus::CPlane& plane, const zeus::CVector3f& vert1,
                                 const zeus::CVector3f& vert2) {
  return zeus::clamp(0.f, -plane.pointToPlaneDist(vert1) / (vert2 - vert1).dot(plane.normal()), 1.f);
}

void CBooRenderer::CalcDrawFogFan(const zeus::CPlane* planes, int numPlanes, const zeus::CVector3f* verts, int numVerts,
                                  int iteration, int level, CFogVolumePlaneShader& fogVol) {
  if (level == iteration) {
    CalcDrawFogFan(planes, numPlanes, verts, numVerts, iteration, level + 1, fogVol);
    return;
  }

  if (level == numPlanes) {
    fogVol.addFan(verts, numVerts);
    return;
  }

  const zeus::CPlane& plane = planes[level];
  u32 insidePlaneCount = 0;
  bool outsidePlane[20];
  for (int i = 0; i < numVerts; ++i)
    outsidePlane[insidePlaneCount++] = plane.normal().dot(verts[i]) < plane.d();

  u32 numUseVerts = 0;
  zeus::CVector3f useVerts[20];
  for (int i = 0; i < numVerts; ++i) {
    int nextIdx = (i + 1) % numVerts;
    int insidePair = int(outsidePlane[i]) | (int(outsidePlane[nextIdx]) << 1);
    if (!(insidePair & 0x1))
      useVerts[numUseVerts++] = verts[i];
    if (insidePair == 1 || insidePair == 2) {
      /* Inside/outside transition; clip verts to other plane boundary */
      const zeus::CVector3f vert1 = verts[i];
      const zeus::CVector3f vert2 = verts[nextIdx];
      float interp = GetPlaneInterpolant(plane, vert1, vert2);
      if (interp > 0.f || interp < 1.f)
        useVerts[numUseVerts++] = (vert1 * (1.f - interp)) + (vert2 * interp);
    }
  }

  if (numUseVerts >= 3)
    CalcDrawFogFan(planes, numPlanes, useVerts, numUseVerts, iteration, level + 1, fogVol);
}

void CBooRenderer::DrawFogSlices(const zeus::CPlane* planes, int numPlanes, int iteration,
                                 const zeus::CVector3f& center, float longestAxis, CFogVolumePlaneShader& fogVol) {
  u32 vertCount = 0;
  zeus::CVector3d verts[4];
  u32 vert2Count = 0;
  zeus::CVector3f verts2[4];
  const zeus::CPlane& plane = planes[iteration];
  int longestNormAxis = std::fabs(plane[1]) > std::fabs(plane[0]);
  if (std::fabs(plane[2]) > std::fabs(plane[longestNormAxis]))
    longestNormAxis = 2;

  zeus::CVector3d pointOnPlane = center - (plane.pointToPlaneDist(center) * plane.normal());

  float deltaSign = plane[longestNormAxis] >= 0.f ? -1.f : 1.f;
  if (longestNormAxis == 1)
    deltaSign = -deltaSign;

  zeus::CVector3d vec1;
  zeus::CVector3d vec2;

  vec1[OrthogonalAxis[longestNormAxis][0]] = longestAxis;
  vec2[OrthogonalAxis[longestNormAxis][1]] = deltaSign * longestAxis;

  verts[vertCount++] = pointOnPlane - vec1 - vec2;
  verts[vertCount++] = pointOnPlane + vec1 - vec2;
  verts[vertCount++] = pointOnPlane + vec1 + vec2;
  verts[vertCount++] = pointOnPlane - vec1 + vec2;

  zeus::CVector3d planeNormal = plane.normal();
  for (const zeus::CVector3d& vert : verts)
    verts2[vert2Count++] = vert - (planeNormal * zeus::CVector3f(planeNormal.dot(vert) - plane.d()));

  CalcDrawFogFan(planes, numPlanes, verts2, vert2Count, iteration, 0, fogVol);
}

void CBooRenderer::RenderFogVolumeModel(const zeus::CAABox& aabb, const CModel* model, const zeus::CTransform& modelMtx,
                                        const zeus::CTransform& viewMtx, const CSkinnedModel* sModel, int pass,
                                        CFogVolumePlaneShader* fvs) {
  if (!model && !sModel) {
    if (pass == 0) {
      zeus::CAABox xfAABB = aabb.getTransformedAABox(modelMtx);
      zeus::CUnitVector3f viewNormal(viewMtx.basis[1]);
      zeus::CPlane planes[7] = {{zeus::CVector3f::skRight, xfAABB.min.x()},
                                {zeus::CVector3f::skLeft, -xfAABB.max.x()},
                                {zeus::CVector3f::skForward, xfAABB.min.y()},
                                {zeus::CVector3f::skBack, -xfAABB.max.y()},
                                {zeus::CVector3f::skUp, xfAABB.min.z()},
                                {zeus::CVector3f::skDown, -xfAABB.max.z()},
                                {viewNormal, viewNormal.dot(viewMtx.origin) + 0.2f + 0.1f}};

      CGraphics::SetModelMatrix(zeus::CTransform::Identity());

      float longestAxis = std::max(std::max(xfAABB.max.x() - xfAABB.min.x(), xfAABB.max.y() - xfAABB.min.y()),
                                   xfAABB.max.z() - xfAABB.min.z()) *
                          2.f;

      fvs->reset(7 * 6);
      for (int i = 0; i < 7; ++i)
        DrawFogSlices(planes, 7, i, xfAABB.center(), longestAxis, *fvs);
      fvs->draw(0);
    } else {
      fvs->draw(pass);
    }
  } else {
    CModelFlags flags;
    switch (pass) {
    case 0:
    default:
      flags.m_extendedShader = EExtendedShader::SolidColorFrontfaceCullLEqualAlphaOnly;
      flags.x4_color = zeus::CColor(1.f, 1.f, 1.f, 1.f);
      break;
    case 1:
      flags.m_extendedShader = EExtendedShader::SolidColorFrontfaceCullAlwaysAlphaOnly;
      flags.x4_color = zeus::CColor(1.f, 1.f, 1.f, 1.f);
      break;
    case 2:
      flags.m_extendedShader = EExtendedShader::SolidColorBackfaceCullLEqualAlphaOnly;
      flags.x4_color = zeus::CColor(1.f, 1.f, 1.f, 0.f);
      break;
    case 3:
      flags.m_extendedShader = EExtendedShader::SolidColorBackfaceCullGreaterAlphaOnly;
      flags.x4_color = zeus::CColor(1.f, 1.f, 1.f, 0.f);
      break;
    }

    if (sModel) {
      sModel->Draw(flags);
    } else {
      model->UpdateLastFrame();
      model->Draw(flags);
    }
  }
}

void CBooRenderer::SetupRendererStates() const {
  CGraphics::SetModelMatrix(zeus::CTransform::Identity());
  CGraphics::g_ColorRegs[1] = x2fc_tevReg1Color;
}

void CBooRenderer::ReallyRenderFogVolume(const zeus::CColor& color, const zeus::CAABox& aabb, const CModel* model,
                                         const CSkinnedModel* sModel) {
  zeus::CMatrix4f proj = CGraphics::GetPerspectiveProjectionMatrix(false);
  zeus::CVector4f points[8];

  for (int i = 0; i < 8; ++i) {
    zeus::CVector3f xfPt = CGraphics::g_GXModelView * aabb.getPoint(i);
    points[i] = proj * zeus::CVector4f(xfPt);
  }

  zeus::CVector2i vpMax(0, 0);
  zeus::CVector2i vpMin(g_Viewport.x8_width, g_Viewport.xc_height);

  bool vpInvalid = true;
  for (int i = 0; i < 20; ++i) {
    zeus::CVector3f overW;
    if (i < 8) {
      overW = points[i].toVec3f() * (1.f / points[i].w());
    } else {
      const zeus::CVector4f& pt1 = points[s_FogVolumeCtrl.xfc_[i - 8][0]];
      const zeus::CVector4f& pt2 = points[s_FogVolumeCtrl.xfc_[i - 8][1]];

      bool eq1 = (pt1.z() / pt1.w()) == 1.f;
      bool eq2 = (pt2.z() / pt2.w()) == 1.f;
      if (eq1 == eq2)
        continue;

      float interp = -(pt1.w() - 1.f) / (pt2.w() - pt1.w());
      if (interp <= 0.f || interp >= 1.f)
        continue;

      float wRecip = 1.f / (interp * (pt2.w() - pt1.w()) + pt1.w());
      zeus::CVector3f pt1_3 = pt1.toVec3f();
      zeus::CVector3f pt2_3 = pt2.toVec3f();
      overW = (pt1_3 + interp * (pt2_3 - pt1_3)) * wRecip;
    }

    // if (overW.z > 1.001f)
    //    continue;

    int vpX = zeus::clamp(0, int(g_Viewport.x8_width * overW.x() * 0.5f + (g_Viewport.x8_width / 2)),
                          int(g_Viewport.x8_width));
    int vpY = zeus::clamp(0, int(g_Viewport.xc_height * overW.y() * 0.5f + (g_Viewport.xc_height / 2)),
                          int(g_Viewport.xc_height));
    vpMax.x = std::max(vpMax.x, vpX);
    vpMin.x = std::min(vpMin.x, vpX);
    vpMax.y = std::max(vpMax.y, vpY);
    vpMin.y = std::min(vpMin.y, vpY);
    vpInvalid = false;
  }

  zeus::CVector2i vpSize = {vpMax.x - vpMin.x, vpMax.y - vpMin.y};
  if (vpSize.x <= 0 || vpSize.y <= 0)
    return;

  SClipScreenRect rect = {};
  rect.x4_left = vpMin.x;
  rect.x8_top = vpMin.y;
  rect.xc_width = vpSize.x;
  rect.x10_height = vpSize.y;

  rect.x4_left = 0;
  rect.x8_top = 0;
  rect.xc_width = g_Viewport.x8_width;
  rect.x10_height = g_Viewport.xc_height;

  // CGraphics::SetScissor(vpMin.x, vpMin.y, vpSize.x, vpSize.y);
  zeus::CAABox marginAABB((CGraphics::g_GXModelView * aabb.min) - 1.f, (CGraphics::g_GXModelView * aabb.max) + 1.f);
  bool camInModel = marginAABB.pointInside(CGraphics::g_ViewMatrix.origin) && (model || sModel);

  CFogVolumePlaneShader* fvs;
  if (!model && !sModel) {
    fvs = &*((m_nextFogVolumePlaneShader == m_fogVolumePlaneShaders.end())
                 ? m_fogVolumePlaneShaders.insert(m_fogVolumePlaneShaders.end(), CFogVolumePlaneShader())
                 : m_nextFogVolumePlaneShader++);
  } else {
    fvs = nullptr;
  }

  RenderFogVolumeModel(aabb, model, CGraphics::g_GXModelMatrix, CGraphics::g_ViewMatrix, sModel, 0, fvs);
  if (camInModel)
    RenderFogVolumeModel(aabb, model, CGraphics::g_GXModelMatrix, CGraphics::g_ViewMatrix, sModel, 1, fvs);

  CGraphics::ResolveSpareDepth(rect, 0);

  RenderFogVolumeModel(aabb, model, CGraphics::g_GXModelMatrix, CGraphics::g_ViewMatrix, sModel, 2, fvs);
  if (camInModel)
    RenderFogVolumeModel(aabb, model, CGraphics::g_GXModelMatrix, CGraphics::g_ViewMatrix, sModel, 3, fvs);

  CGraphics::ResolveSpareDepth(rect, 1);

  auto fvf = (m_nextFogVolumeFilter == m_fogVolumeFilters.end())
                 ? m_fogVolumeFilters.insert(m_fogVolumeFilters.end(), CFogVolumeFilter())
                 : m_nextFogVolumeFilter++;
  fvf->draw2WayPass(color);
  if (camInModel)
    fvf->draw1WayPass(color);

  // CGraphics::SetScissor(g_Viewport.x0_left, g_Viewport.x4_top, g_Viewport.x8_width, g_Viewport.xc_height);
}

void CBooRenderer::GenerateFogVolumeRampTex(boo::IGraphicsDataFactory::Context& ctx) {
  u16 data[FOGVOL_RAMP_RES][FOGVOL_RAMP_RES] = {};
  for (int y = 0; y < FOGVOL_RAMP_RES; ++y) {
    for (int x = 0; x < FOGVOL_RAMP_RES; ++x) {
      int tmp = y << 16 | x << 8 | 0x7f;
      double a =
          zeus::clamp(0.0,
                      (-150.0 / (tmp / double(0xffffff) * (FOGVOL_FAR - FOGVOL_NEAR) - FOGVOL_FAR) - FOGVOL_NEAR) *
                          3.0 / (FOGVOL_FAR - FOGVOL_NEAR),
                      1.0);
      data[y][x] = u16((a * a + a) / 2.0 * 65535);
    }
  }
  x1b8_fogVolumeRamp =
      ctx.newStaticTexture(FOGVOL_RAMP_RES, FOGVOL_RAMP_RES, 1, boo::TextureFormat::I16, boo::TextureClampMode::Repeat,
                           data[0], FOGVOL_RAMP_RES * FOGVOL_RAMP_RES * 2);
}

void CBooRenderer::GenerateSphereRampTex(boo::IGraphicsDataFactory::Context& ctx) {
  u8 data[SPHERE_RAMP_RES][SPHERE_RAMP_RES] = {};
  float halfRes = SPHERE_RAMP_RES / 2.f;
  for (int y = 0; y < SPHERE_RAMP_RES; ++y) {
    for (int x = 0; x < SPHERE_RAMP_RES; ++x) {
      zeus::CVector2f vec((x - halfRes) / halfRes, (y - halfRes) / halfRes);
      data[y][x] = 255 - zeus::clamp(0.f, vec.canBeNormalized() ? vec.magnitude() : 0.f, 1.f) * 255;
    }
  }
  x220_sphereRamp = ctx.newStaticTexture(SPHERE_RAMP_RES, SPHERE_RAMP_RES, 1, boo::TextureFormat::I8,
                                         boo::TextureClampMode::Repeat, data[0], SPHERE_RAMP_RES * SPHERE_RAMP_RES);
}

void CBooRenderer::GenerateScanLinesVBO(boo::IGraphicsDataFactory::Context& ctx) {
  std::vector<zeus::CVector3f> verts;
  verts.reserve(670);

  for (int i = 0; i < 112; ++i) {
    verts.push_back(zeus::CVector3f(-1.f, (i * (4.f / 448.f) + (1.f / 448.f)) * 2.f - 1.f, 0.f));
    if (i != 0)
      verts.push_back(verts.back());
    verts.push_back(zeus::CVector3f(-1.f, (i * (4.f / 448.f) - (1.f / 448.f)) * 2.f - 1.f, 0.f));
    verts.push_back(zeus::CVector3f(1.f, (i * (4.f / 448.f) + (1.f / 448.f)) * 2.f - 1.f, 0.f));
    verts.push_back(zeus::CVector3f(1.f, (i * (4.f / 448.f) - (1.f / 448.f)) * 2.f - 1.f, 0.f));
    if (i != 111)
      verts.push_back(verts.back());
  }

  m_scanLinesEvenVBO = ctx.newStaticBuffer(boo::BufferUse::Vertex, verts.data(), sizeof(zeus::CVector3f), verts.size());

  verts.clear();

  for (int i = 0; i < 112; ++i) {
    verts.push_back(zeus::CVector3f(-1.f, (i * (4.f / 448.f) + (3.f / 448.f)) * 2.f - 1.f, 0.f));
    if (i != 0)
      verts.push_back(verts.back());
    verts.push_back(zeus::CVector3f(-1.f, (i * (4.f / 448.f) + (1.f / 448.f)) * 2.f - 1.f, 0.f));
    verts.push_back(zeus::CVector3f(1.f, (i * (4.f / 448.f) + (3.f / 448.f)) * 2.f - 1.f, 0.f));
    verts.push_back(zeus::CVector3f(1.f, (i * (4.f / 448.f) + (1.f / 448.f)) * 2.f - 1.f, 0.f));
    if (i != 111)
      verts.push_back(verts.back());
  }

  m_scanLinesOddVBO = ctx.newStaticBuffer(boo::BufferUse::Vertex, verts.data(), sizeof(zeus::CVector3f), verts.size());
}

void CBooRenderer::LoadThermoPalette() {
  m_thermoPaletteTex = xc_store.GetObj("TXTR_ThermoPalette");
  CTexture* thermoTexObj = m_thermoPaletteTex.GetObj();
  if (thermoTexObj)
    x288_thermoPalette = thermoTexObj->GetPaletteTexture();
}

void CBooRenderer::LoadBallFade() {
  m_ballFadeTex = xc_store.GetObj("TXTR_BallFade");
  CTexture* ballFadeTexObj = m_ballFadeTex.GetObj();
  if (ballFadeTexObj)
    m_ballFade = ballFadeTexObj->GetBooTexture();
}

CBooRenderer::CBooRenderer(IObjectStore& store, IFactory& resFac)
: x8_factory(resFac), xc_store(store), x2a8_thermalRand(20) {
  g_Renderer = this;
  xee_24_ = true;

  m_staticEntropy = store.GetObj("RandomStaticEntropy");

  CGraphics::CommitResources([&](boo::IGraphicsDataFactory::Context& ctx) {
    u8 clearPixel[] = {0, 0, 0, 0};
    m_clearTexture = ctx.newStaticTexture(1, 1, 1, boo::TextureFormat::RGBA8,
                                          boo::TextureClampMode::Repeat, clearPixel, 4).get();
    u8 blackPixel[] = {0, 0, 0, 255};
    m_blackTexture = ctx.newStaticTexture(1, 1, 1, boo::TextureFormat::RGBA8,
                                          boo::TextureClampMode::Repeat, blackPixel, 4).get();
    u8 whitePixel[] = {255, 255, 255, 255};
    m_whiteTexture = ctx.newStaticTexture(1, 1, 1, boo::TextureFormat::RGBA8,
                                          boo::TextureClampMode::Repeat, whitePixel, 4).get();

    GenerateFogVolumeRampTex(ctx);
    GenerateSphereRampTex(ctx);
    m_ballShadowId = ctx.newRenderTexture(m_ballShadowIdW, m_ballShadowIdH, boo::TextureClampMode::Repeat, 1, 0);
    x14c_reflectionTex = ctx.newRenderTexture(256, 256, boo::TextureClampMode::ClampToBlack, 1, 0);
    GenerateScanLinesVBO(ctx);
    return true;
  } BooTrace);
  LoadThermoPalette();
  LoadBallFade();
  m_thermHotFilter.emplace();

  Buckets::Init();

  m_nextFogVolumePlaneShader = m_fogVolumePlaneShaders.end();
  m_nextFogVolumeFilter = m_fogVolumeFilters.end();
}

CBooRenderer::~CBooRenderer() { g_Renderer = nullptr; }

void CBooRenderer::AddWorldSurfaces(CBooModel& model) {
  CBooSurface* surf = model.x3c_firstSortedSurface;
  while (surf) {
    const MaterialSet::Material& mat = model.GetMaterialByIndex(surf->m_data.matIdx);
    zeus::CAABox aabb = surf->GetBounds();
    zeus::CVector3f pt = aabb.closestPointAlongVector(xb0_viewPlane.normal());
    Buckets::Insert(pt, aabb, EDrawableType::WorldSurface, surf, xb0_viewPlane,
                    mat.heclIr.m_blendSrc == boo::BlendFactor::SrcAlpha &&
                        mat.heclIr.m_blendDst == boo::BlendFactor::InvSrcAlpha);
    surf = surf->m_next;
  }
}

std::list<CBooRenderer::CAreaListItem>::iterator
CBooRenderer::FindStaticGeometry(const std::vector<CMetroidModelInstance>* geometry) {
  return std::find_if(x1c_areaListItems.begin(), x1c_areaListItems.end(),
                      [&](CAreaListItem& item) -> bool { return item.x0_geometry == geometry; });
}

void CBooRenderer::AddStaticGeometry(const std::vector<CMetroidModelInstance>* geometry,
                                     const CAreaRenderOctTree* octTree, int areaIdx, const SShader* shaderSet) {
  auto search = FindStaticGeometry(geometry);
  if (search == x1c_areaListItems.end()) {
    std::vector<TCachedToken<CTexture>> textures;
    std::vector<CBooModel*> models;
    if (geometry->size()) {
      (*geometry)[0].m_instance->MakeTexturesFromMats(textures, xc_store);
      models.reserve(geometry->size());
      int instIdx = 0;
      for (const CMetroidModelInstance& inst : *geometry) {
        models.push_back(inst.m_instance.get());
        models.back()->x44_areaInstanceIdx = instIdx++;
      }
    }
    x1c_areaListItems.emplace_back(geometry, octTree, std::move(textures), std::move(models), areaIdx, shaderSet);
  }
}

void CBooRenderer::EnablePVS(const CPVSVisSet& set, u32 areaIdx) {
  xc8_pvs.emplace(set);
  xe0_pvsAreaIdx = areaIdx;
}

void CBooRenderer::DisablePVS() { xc8_pvs = std::experimental::nullopt; }

void CBooRenderer::UpdateAreaUniforms(int areaIdx, bool shadowRender, bool activateLights) {
  SetupRendererStates();

  CModelFlags flags;
  int bufIdx;
  if (shadowRender) {
    flags.m_extendedShader = EExtendedShader::SolidColor;
    flags.x4_color = zeus::CColor::skBlack;
    bufIdx = 1;
  } else {
    flags.m_extendedShader = EExtendedShader::Lighting;
    bufIdx = 0;
  }

  for (CAreaListItem& item : x1c_areaListItems) {
    if (areaIdx != -1 && item.x18_areaIdx != areaIdx)
      continue;

    item.m_shaderSet->m_geomLayout->Update(flags, nullptr, nullptr, &item.m_shaderSet->m_matSet,
                                           item.m_shaderSet->m_geomLayout->m_sharedBuffer[bufIdx], nullptr);

    for (auto it = item.x10_models.begin(); it != item.x10_models.end(); ++it) {
      CBooModel* model = *it;
      if (model->TryLockTextures()) {
        if (activateLights)
          ActivateLightsForModel(&item, *model);
        model->UpdateUniformData(flags, nullptr, nullptr, bufIdx);
      }
    }
  }
}

void CBooRenderer::RemoveStaticGeometry(const std::vector<CMetroidModelInstance>* geometry) {
  auto search = FindStaticGeometry(geometry);
  if (search != x1c_areaListItems.end())
    x1c_areaListItems.erase(search);
}

void CBooRenderer::DrawAreaGeometry(int areaIdx, int mask, int targetMask) {
  x318_30_inAreaDraw = true;
  // SetupRendererStates();
  CModelFlags flags;

  for (CAreaListItem& item : x1c_areaListItems) {
    if (areaIdx != -1 || item.x18_areaIdx == areaIdx) {
      CPVSVisSet* pvs = xc8_pvs ? &*xc8_pvs : nullptr;
      if (xe0_pvsAreaIdx != item.x18_areaIdx)
        pvs = nullptr;
      int modelIdx = 0;
      for (auto it = item.x10_models.begin(); it != item.x10_models.end(); ++it, ++modelIdx) {
        CBooModel* model = *it;
        if (pvs) {
          bool visible = pvs->GetVisible(modelIdx) != EPVSVisSetState::EndOfTree;
          if ((xc4_pvsMode == EPVSMode::PVS && !visible) || (xc4_pvsMode == EPVSMode::PVSAndMask && visible))
            continue;
        }
        if ((model->x41_mask & mask) != targetMask)
          continue;
        if (!x44_frustumPlanes.aabbFrustumTest(model->x20_aabb))
          continue;

        for (const CBooSurface* surf = model->x38_firstUnsortedSurface; surf; surf = surf->m_next)
          model->DrawSurface(*surf, flags);
        for (const CBooSurface* surf = model->x3c_firstSortedSurface; surf; surf = surf->m_next)
          model->DrawSurface(*surf, flags);
      }
    }
  }

  x318_30_inAreaDraw = false;
}

void CBooRenderer::DrawUnsortedGeometry(int areaIdx, int mask, int targetMask, bool shadowRender) {
  // SetupRendererStates();
  CModelFlags flags;
  flags.m_extendedShader = shadowRender ? EExtendedShader::SolidColor : EExtendedShader::Lighting;

  CAreaListItem* lastOctreeItem = nullptr;

  for (CAreaListItem& item : x1c_areaListItems) {
    if (areaIdx != -1 && item.x18_areaIdx != areaIdx)
      continue;

    if (item.x4_octTree)
      lastOctreeItem = &item;

    CPVSVisSet* pvs = nullptr;
    if (xc8_pvs)
      pvs = &*xc8_pvs;
    if (xe0_pvsAreaIdx != item.x18_areaIdx)
      pvs = nullptr;

    u32 idx = 0;
    for (auto it = item.x10_models.begin(); it != item.x10_models.end(); ++it, ++idx) {
      CBooModel* model = *it;
      if (pvs) {
        bool vis = pvs->GetVisible(idx) != EPVSVisSetState::EndOfTree;
        switch (xc4_pvsMode) {
        case EPVSMode::PVS: {
          if (!vis) {
            model->x40_25_modelVisible = false;
            continue;
          }
          break;
        }
        case EPVSMode::PVSAndMask: {
          if (!vis && (model->x41_mask & mask) != targetMask) {
            model->x40_25_modelVisible = false;
            continue;
          }
        }
        default:
          break;
        }
      }

      if ((model->x41_mask & mask) != targetMask) {
        model->x40_25_modelVisible = false;
        continue;
      }

      if (!x44_frustumPlanes.aabbFrustumTest(model->x20_aabb)) {
        model->x40_25_modelVisible = false;
        continue;
      }

      if (x318_25_drawWireframe) {
        model->x40_25_modelVisible = false;
        // HandleUnsortedModelWireframe();
        continue;
      }

      model->x40_25_modelVisible = true;
      HandleUnsortedModel(lastOctreeItem, *model, flags);
    }
  }

  // SetupCGraphicsStates();
}

void CBooRenderer::DrawSortedGeometry(int areaIdx, int mask, int targetMask) {
  // SetupRendererStates();

  CAreaListItem* lastOctreeItem = nullptr;

  for (CAreaListItem& item : x1c_areaListItems) {
    if (areaIdx != -1 && item.x18_areaIdx != areaIdx)
      continue;

    if (item.x4_octTree)
      lastOctreeItem = &item;

    for (auto it = item.x10_models.begin(); it != item.x10_models.end(); ++it) {
      CBooModel* model = *it;
      if (model->x40_25_modelVisible)
        AddWorldSurfaces(*model);
    }
  }

  Buckets::Sort();
  RenderBucketItems(lastOctreeItem);

  // SetupCGraphicsStates();
  // DrawRenderBucketsDebug();
  Buckets::Clear();
}

void CBooRenderer::DrawStaticGeometry(int modelCount, int mask, int targetMask) {
  DrawUnsortedGeometry(modelCount, mask, targetMask);
  DrawSortedGeometry(modelCount, mask, targetMask);
}

void CBooRenderer::DrawModelFlat(const CModel& model, const CModelFlags& flags, bool unsortedOnly) {
  model.GetInstance().DrawFlat(unsortedOnly ? CBooModel::ESurfaceSelection::UnsortedOnly
                                            : CBooModel::ESurfaceSelection::All,
                               flags.m_extendedShader);
}

void CBooRenderer::PostRenderFogs() {
  for (const auto& warp : x2c4_spaceWarps)
    DrawSpaceWarp(warp.first, warp.second);
  x2c4_spaceWarps.clear();

  x2ac_fogVolumes.sort([](const CFogVolumeListItem& a, const CFogVolumeListItem& b) {
    zeus::CAABox aabbA = a.x34_aabb.getTransformedAABox(a.x0_transform);
    bool insideA =
        aabbA.pointInside(zeus::CVector3f(CGraphics::g_ViewPoint.x(), CGraphics::g_ViewPoint.y(), aabbA.min.z()));

    zeus::CAABox aabbB = b.x34_aabb.getTransformedAABox(b.x0_transform);
    bool insideB =
        aabbB.pointInside(zeus::CVector3f(CGraphics::g_ViewPoint.x(), CGraphics::g_ViewPoint.y(), aabbB.min.z()));

    if (insideA != insideB)
      return insideA;

    float dotA = aabbA.furthestPointAlongVector(CGraphics::g_ViewMatrix.basis[1]).dot(CGraphics::g_ViewMatrix.basis[1]);
    float dotB = aabbB.furthestPointAlongVector(CGraphics::g_ViewMatrix.basis[1]).dot(CGraphics::g_ViewMatrix.basis[1]);
    return dotA < dotB;
  });
  for (const CFogVolumeListItem& fog : x2ac_fogVolumes) {
    CGraphics::SetModelMatrix(fog.x0_transform);
    ReallyRenderFogVolume(fog.x30_color, fog.x34_aabb, fog.x4c_model.GetObj(), fog.x5c_skinnedModel);
  }
  x2ac_fogVolumes.clear();
}

void CBooRenderer::AddParticleGen(const CParticleGen& gen) {
  if (auto bounds = gen.GetBounds()) {
    zeus::CVector3f pt = bounds.value().closestPointAlongVector(xb0_viewPlane.normal());
    Buckets::Insert(pt, bounds.value(), EDrawableType::Particle, &gen, xb0_viewPlane, 0);
  }
}

void CBooRenderer::AddParticleGen(const CParticleGen& gen, const zeus::CVector3f& pos, const zeus::CAABox& bounds) {
  Buckets::Insert(pos, bounds, EDrawableType::Particle, &gen, xb0_viewPlane, 0);
}

void CBooRenderer::AddPlaneObject(const void* obj, const zeus::CAABox& aabb, const zeus::CPlane& plane, int type) {
  zeus::CVector3f closePoint = aabb.closestPointAlongVector(xb0_viewPlane.normal());
  zeus::CVector3f farPoint = aabb.furthestPointAlongVector(xb0_viewPlane.normal());
  float closeDist = xb0_viewPlane.pointToPlaneDist(closePoint);
  float farDist = xb0_viewPlane.pointToPlaneDist(farPoint);
  if (closeDist >= 0.f || farDist >= 0.f) {
    bool zOnly = plane.normal() == zeus::CVector3f::skUp;
    bool invert;
    if (zOnly)
      invert = CGraphics::g_ViewMatrix.origin.z() >= plane.d();
    else
      invert = plane.pointToPlaneDist(CGraphics::g_ViewMatrix.origin) >= 0.f;
    Buckets::InsertPlaneObject(closeDist, farDist, aabb, invert, plane, zOnly, EDrawableType(type + 2), obj);
  }
}

void CBooRenderer::AddDrawable(const void* obj, const zeus::CVector3f& pos, const zeus::CAABox& aabb, int mode,
                               EDrawableSorting sorting) {
  if (sorting == EDrawableSorting::UnsortedCallback)
    xa8_drawableCallback(obj, xac_callbackContext, mode);
  else
    Buckets::Insert(pos, aabb, EDrawableType(mode + 2), obj, xb0_viewPlane, 0);
}

void CBooRenderer::SetDrawableCallback(TDrawableCallback cb, const void* ctx) {
  xa8_drawableCallback = cb;
  xac_callbackContext = ctx;
}

void CBooRenderer::SetWorldViewpoint(const zeus::CTransform& xf) {
  CGraphics::SetViewPointMatrix(xf);
  xb0_viewPlane = zeus::CPlane(xf.basis[1], xf.basis[1].dot(xf.origin));
}

void CBooRenderer::SetPerspective(float fovy, float width, float height, float znear, float zfar) {
  CGraphics::SetPerspective(fovy, width / height, znear, zfar);
}

void CBooRenderer::SetPerspective(float fovy, float aspect, float znear, float zfar) {
  CGraphics::SetPerspective(fovy, aspect, znear, zfar);
}

std::pair<zeus::CVector2f, zeus::CVector2f> CBooRenderer::SetViewportOrtho(bool centered, float znear, float zfar) {
  float left = centered ? g_Viewport.x0_left - g_Viewport.x10_halfWidth : 0.f;
  float bottom = centered ? g_Viewport.x4_top - g_Viewport.x14_halfHeight : 0.f;
  float top = centered ? g_Viewport.x0_left + g_Viewport.x14_halfHeight : g_Viewport.xc_height;
  float right = centered ? g_Viewport.x4_top + g_Viewport.x10_halfWidth : g_Viewport.x8_width;

  CGraphics::SetOrtho(left, right, top, bottom, znear, zfar);
  CGraphics::SetViewPointMatrix(zeus::CTransform::Identity());
  CGraphics::SetModelMatrix(zeus::CTransform::Identity());

  return {{left, bottom}, {right, top}};
}

void CBooRenderer::SetClippingPlanes(const zeus::CFrustum& frustum) { x44_frustumPlanes = frustum; }

void CBooRenderer::SetViewport(int l, int b, int w, int h) {
  CGraphics::SetViewport(l, b, w, h);
  CGraphics::SetScissor(l, b, w, h);
}

void CBooRenderer::SetDebugOption(EDebugOption, int) {}

void CBooRenderer::BeginScene() {
  CGraphics::SetViewport(0, 0, g_Viewport.x8_width, g_Viewport.xc_height);
  CGraphics::SetPerspective(75.f, CGraphics::g_ProjAspect, 1.f, 4096.f);
  CGraphics::SetModelMatrix(zeus::CTransform::Identity());
#if 0
    if (x310_phazonSuitMaskCountdown != 0)
    {
        --x310_phazonSuitMaskCountdown;
        if (x310_phazonSuitMaskCountdown == 0)
            x314_phazonSuitMask.reset();
    }
#endif
  x318_27_currentRGBA6 = x318_26_requestRGBA6;
  if (!x318_31_persistRGBA6)
    x318_26_requestRGBA6 = false;
  // GXSetPixelFmt(x318_27_currentRGBA6);
  CGraphics::BeginScene();
  m_nextFogVolumePlaneShader = m_fogVolumePlaneShaders.begin();
  m_nextFogVolumeFilter = m_fogVolumeFilters.begin();
}

void CBooRenderer::EndScene() {
  CGraphics::EndScene();
  if (x2dc_reflectionAge >= 2) {
    // Delete reflection tex x14c_
  } else {
    ++x2dc_reflectionAge;
  }
}

void CBooRenderer::SetAmbientColor(const zeus::CColor& color) { CGraphics::SetAmbientColor(color); }

void CBooRenderer::DrawString(const char*, int, int) {}

u32 CBooRenderer::GetFPS() { return 0; }

void CBooRenderer::CacheReflection(TReflectionCallback cb, void* ctx, bool clearAfter) {
  if (!x318_24_refectionDirty)
    return;
  x318_24_refectionDirty = false;
  x2dc_reflectionAge = 0;

  BindReflectionDrawTarget();
  SViewport backupVp = g_Viewport;
  SetViewport(0, 0, 256, 256);
  CGraphics::g_BooMainCommandQueue->clearTarget();
  cb(ctx, CBooModel::g_ReflectViewPos);
  boo::SWindowRect rect(0, 0, 256, 256);
  CGraphics::g_BooMainCommandQueue->resolveBindTexture(x14c_reflectionTex, rect, false, 0, true, false);
  BindMainDrawTarget();
  SetViewport(backupVp.x0_left, backupVp.x4_top, backupVp.x8_width, backupVp.xc_height);
}

void CBooRenderer::DrawSpaceWarp(const zeus::CVector3f& pt, float strength) {
  m_spaceWarpFilter.setStrength(strength);
  m_spaceWarpFilter.draw(pt);
}

void CBooRenderer::DrawThermalModel(const CModel& model, const zeus::CColor& mulCol, const zeus::CColor& addCol) {
  CModelFlags flags;
  flags.m_extendedShader = EExtendedShader::Thermal;
  flags.x4_color = mulCol;
  flags.addColor = addCol;
  model.UpdateLastFrame();
  model.Draw(flags);
}

void CBooRenderer::DrawXRayOutline(const zeus::CAABox& aabb) {
  CModelFlags flags;
  flags.m_extendedShader = EExtendedShader::ForcedAlpha;

  for (CAreaListItem& item : x1c_areaListItems) {
    if (item.x4_octTree) {
      std::vector<u32> bitmap;
      item.x4_octTree->FindOverlappingModels(bitmap, aabb);

      for (u32 c = 0; c < item.x4_octTree->x14_bitmapWordCount; ++c) {
        for (u32 b = 0; b < 32; ++b) {
          if (bitmap[c] & (1 << b)) {
            CBooModel* model = item.x10_models[c * 32 + b];
            model->UpdateUniformData(flags, nullptr, nullptr);
            const CBooSurface* surf = model->x38_firstUnsortedSurface;
            while (surf) {
              if (surf->GetBounds().intersects(aabb))
                model->DrawSurface(*surf, flags);
              surf = surf->m_next;
            }
          }
        }
      }
    }
  }
}

void CBooRenderer::SetWireframeFlags(int) {}

void CBooRenderer::SetWorldFog(ERglFogMode mode, float startz, float endz, const zeus::CColor& color) {
  if (x318_28_disableFog)
    mode = ERglFogMode::None;
  CGraphics::SetFog(mode, startz, endz, color);
}

void CBooRenderer::RenderFogVolume(const zeus::CColor& color, const zeus::CAABox& aabb,
                                   const TLockedToken<CModel>* model, const CSkinnedModel* sModel) {
  if (!x318_28_disableFog)
    x2ac_fogVolumes.emplace_back(CGraphics::g_GXModelMatrix, color, aabb, model, sModel);
}

void CBooRenderer::SetThermal(bool thermal, float level, const zeus::CColor& color) {
  x318_29_thermalVisor = thermal;
  x2f0_thermalVisorLevel = level;
  x2f4_thermColor = color;
  CDecal::SetMoveRedToAlphaBuffer(false);
  CElementGen::SetMoveRedToAlphaBuffer(false);
}

void CBooRenderer::SetThermalColdScale(float scale) { x2f8_thermColdScale = zeus::clamp(0.f, scale, 1.f); }

void CBooRenderer::DoThermalBlendCold() {
  zeus::CColor a = zeus::CColor::lerp(x2f4_thermColor, zeus::CColor::skWhite, x2f8_thermColdScale);
  m_thermColdFilter.setColorA(a);
  float bFac = 0.f;
  float bAlpha = 1.f;
  if (x2f8_thermColdScale < 0.5f) {
    bAlpha = x2f8_thermColdScale * 2.f;
    bFac = (1.f - bAlpha) / 8.f;
  }
  zeus::CColor b{bFac, bFac, bFac, bAlpha};
  m_thermColdFilter.setColorB(b);
  zeus::CColor c = zeus::CColor::lerp(zeus::CColor::skBlack, zeus::CColor::skWhite,
                                      zeus::clamp(0.f, (x2f8_thermColdScale - 0.25f) * 4.f / 3.f, 1.f));
  m_thermColdFilter.setColorC(c);

  m_thermColdFilter.setScale(x2f8_thermColdScale);

  m_thermColdFilter.setShift(x2a8_thermalRand.Next() % 32);
  m_thermColdFilter.draw();
  CElementGen::SetMoveRedToAlphaBuffer(true);
  CDecal::SetMoveRedToAlphaBuffer(true);
}

void CBooRenderer::DoThermalBlendHot() { m_thermHotFilter->draw(); }

u32 CBooRenderer::GetStaticWorldDataSize() { return 0; }

void CBooRenderer::PrepareDynamicLights(const std::vector<CLight>& lights) {
  x300_dynamicLights = lights;
  for (CAreaListItem& area : x1c_areaListItems) {
    if (const CAreaRenderOctTree* arot = area.x4_octTree) {
      area.x1c_lightOctreeWords.clear();
      area.x1c_lightOctreeWords.resize(arot->x14_bitmapWordCount * lights.size());
      u32* wordPtr = area.x1c_lightOctreeWords.data();
      for (const CLight& light : lights) {
        float radius = light.GetRadius();
        zeus::CVector3f vMin = light.GetPosition() - radius;
        zeus::CVector3f vMax = light.GetPosition() + radius;
        zeus::CAABox aabb(vMin, vMax);
        arot->FindOverlappingModels(wordPtr, aabb);
        wordPtr += arot->x14_bitmapWordCount;
      }
    }
  }
}

void CBooRenderer::SetGXRegister1Color(const zeus::CColor& color) { CGraphics::g_ColorRegs[1] = color; }

void CBooRenderer::SetWorldLightFadeLevel(float level) { x2fc_tevReg1Color = zeus::CColor(level, level, level, 1.f); }

void CBooRenderer::ReallyDrawPhazonSuitIndirectEffect(const zeus::CColor& vertColor, /*const CTexture& maskTex,*/
                                                      const CTexture& indTex, const zeus::CColor& modColor, float scale,
                                                      float offX, float offY) {
  float qScale = scale / 8.f; // Adjustment for URDE
  m_phazonSuitFilter.draw(modColor, scale, offX * qScale, offY * qScale);
}

void CBooRenderer::ReallyDrawPhazonSuitEffect(const zeus::CColor& modColor /*, const CTexture& maskTex*/) {
  m_phazonSuitFilter.draw(modColor, 0.f, 0.f, 0.f);
}

void CBooRenderer::DoPhazonSuitIndirectAlphaBlur(float blurRadius /*, float f2*/,
                                                 const TLockedToken<CTexture>& indTex) {
  m_phazonSuitFilter.drawBlurPasses(blurRadius, indTex.IsLoaded() ? indTex.GetObj() : nullptr);
}

void CBooRenderer::DrawPhazonSuitIndirectEffect(const zeus::CColor& nonIndirectMod,
                                                const TLockedToken<CTexture>& indTex, const zeus::CColor& indirectMod,
                                                float blurRadius, float scale, float offX, float offY) {
  /* Indirect background already in binding 0 */

  /* Resolve alpha channel of just-drawn phazon suit into binding 1 */
  SClipScreenRect rect(g_Viewport);
  CGraphics::ResolveSpareTexture(rect, 1);

  /* Perform blur filter and resolve into binding 2 */
  DoPhazonSuitIndirectAlphaBlur(blurRadius, indTex);

  /* Draw effect; subtracting binding 1 from binding 2 for the filter 'cutout' */
  if (indTex && indTex.IsLoaded())
    ReallyDrawPhazonSuitIndirectEffect(zeus::CColor::skWhite, *indTex, indirectMod, scale, offX, offY);
  else
    ReallyDrawPhazonSuitEffect(nonIndirectMod);
}

void CBooRenderer::AllocatePhazonSuitMaskTexture() {
  x318_26_requestRGBA6 = true;
  x310_phazonSuitMaskCountdown = 2;
}

void CBooRenderer::FindOverlappingWorldModels(std::vector<u32>& modelBits, const zeus::CAABox& aabb) const {
  u32 bitmapWords = 0;
  for (const CAreaListItem& item : x1c_areaListItems)
    if (item.x4_octTree)
      bitmapWords += item.x4_octTree->x14_bitmapWordCount;

  if (!bitmapWords) {
    modelBits.clear();
    return;
  }

  modelBits.clear();
  modelBits.resize(bitmapWords);

  u32 curWord = 0;
  for (const CAreaListItem& item : x1c_areaListItems) {
    if (!item.x4_octTree)
      continue;

    item.x4_octTree->FindOverlappingModels(modelBits.data() + curWord, aabb);

    u32 wordModel = 0;
    for (int i = 0; i < item.x4_octTree->x14_bitmapWordCount; ++i, wordModel += 32) {
      u32& word = modelBits[curWord + i];
      if (!word)
        continue;
      for (int j = 0; j < 32; ++j) {
        if ((1 << j) & word) {
          const zeus::CAABox& modelAABB = item.x10_models[wordModel + j]->x20_aabb;
          if (!modelAABB.intersects(aabb))
            word &= ~(1 << j);
        }
      }
    }

    curWord += item.x4_octTree->x14_bitmapWordCount;
  }
}

int CBooRenderer::DrawOverlappingWorldModelIDs(int alphaVal, const std::vector<u32>& modelBits,
                                               const zeus::CAABox& aabb) const {
  SetupRendererStates();
  const_cast<CBooRenderer&>(*this).UpdateAreaUniforms(-1, false, false);

  CModelFlags flags;
  flags.m_extendedShader = EExtendedShader::SolidColor; // Do solid color draw

  u32 curWord = 0;
  for (const CAreaListItem& item : x1c_areaListItems) {
    if (!item.x4_octTree)
      continue;

    u32 wordModel = 0;
    for (int i = 0; i < item.x4_octTree->x14_bitmapWordCount; ++i, wordModel += 32) {
      const u32& word = modelBits[curWord + i];
      if (!word)
        continue;
      for (int j = 0; j < 32; ++j) {
        if ((1 << j) & word) {
          if (alphaVal > 255)
            return alphaVal;

          flags.x4_color.a() = alphaVal / 255.f;
          const CBooModel& model = *item.x10_models[wordModel + j];
          const_cast<CBooModel&>(model).VerifyCurrentShader(0);
          for (const CBooSurface* surf = model.x38_firstUnsortedSurface; surf; surf = surf->m_next)
            if (surf->GetBounds().intersects(aabb))
              model.DrawSurface(*surf, flags);
          alphaVal += 4;
        }
      }
    }

    curWord += item.x4_octTree->x14_bitmapWordCount;
  }

  return alphaVal;
}

void CBooRenderer::DrawOverlappingWorldModelShadows(int alphaVal, const std::vector<u32>& modelBits,
                                                    const zeus::CAABox& aabb, float alpha) const {
  CModelFlags flags;
  flags.x4_color.a() = alpha;
  flags.m_extendedShader = EExtendedShader::MorphBallShadow; // Do shadow draw

  u32 curWord = 0;
  for (const CAreaListItem& item : x1c_areaListItems) {
    if (!item.x4_octTree)
      continue;

    u32 wordModel = 0;
    for (int i = 0; i < item.x4_octTree->x14_bitmapWordCount; ++i, wordModel += 32) {
      const u32& word = modelBits[curWord + i];
      if (!word)
        continue;
      for (int j = 0; j < 32; ++j) {
        if ((1 << j) & word) {
          if (alphaVal > 255)
            return;

          flags.x4_color.r() = alphaVal / 255.f;
          const CBooModel& model = *item.x10_models[wordModel + j];
          const_cast<CBooModel&>(model).VerifyCurrentShader(0);
          for (const CBooSurface* surf = model.x38_firstUnsortedSurface; surf; surf = surf->m_next)
            if (surf->GetBounds().intersects(aabb))
              model.DrawSurface(*surf, flags);
          alphaVal += 4;
        }
      }
    }

    curWord += item.x4_octTree->x14_bitmapWordCount;
  }
}

} // namespace urde
