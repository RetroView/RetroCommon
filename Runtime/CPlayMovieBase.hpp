#pragma once

#include "CIOWin.hpp"
#include "Graphics/CMoviePlayer.hpp"

namespace urde {

class CPlayMovieBase : public CIOWin {
  CMoviePlayer x18_moviePlayer;

public:
  CPlayMovieBase(const char* iowName, const char* path) : CIOWin(iowName), x18_moviePlayer(path, 0.0, false, false) {}
  EMessageReturn OnMessage(const CArchitectureMessage&, CArchitectureQueue&) { return EMessageReturn::Normal; }
  void Draw() const {}
};

} // namespace urde
