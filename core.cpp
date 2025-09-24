#include "core.h"
#include <cmath>
#include <algorithm>

// ---- Вспомогательная математика ----

void Core::identity(double T[4][4]) {
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c)
      T[r][c] = (r == c) ? 1.0 : 0.0;
}

void Core::mul(const double L[4][4], const double R[4][4], double Out[4][4]) {
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      double sum = 0.0;
      for (int k = 0; k < 4; ++k) sum += L[r][k] * R[k][c];
      Out[r][c] = sum;
    }
  }
}

// Классический DH: A = Rz(theta)*Tz(d)*Tx(a)*Rx(alpha)
void Core::makeA(double theta_rad, double a_m, double d_m, double alpha_rad, double A[4][4]) {
  const double ct = std::cos(theta_rad),  st = std::sin(theta_rad);
  const double ca = std::cos(alpha_rad),  sa = std::sin(alpha_rad);

  // Стандартная форма
  // [ ct  -st*ca   st*sa   a*ct ]
  // [ st   ct*ca  -ct*sa   a*st ]
  // [  0     sa      ca      d  ]
  // [  0      0       0      1  ]
  A[0][0] =  ct;   A[0][1] = -st*ca;  A[0][2] =  st*sa;  A[0][3] = a_m*ct;
  A[1][0] =  st;   A[1][1] =  ct*ca;  A[1][2] = -ct*sa;  A[1][3] = a_m*st;
  A[2][0] =  0.0;  A[2][1] =  sa;     A[2][2] =  ca;     A[2][3] = d_m;
  A[3][0] =  0.0;  A[3][1] =  0.0;    A[3][2] =  0.0;    A[3][3] = 1.0;
}

Snapshot Core::normalizeUnits(const Snapshot& s) {
  Snapshot out;
  out.reserve(s.size());
  constexpr double DEG2RAD = 3.14159265358979323846 / 180.0;

  for (const auto& j : s) {
    // theta: deg -> rad; alpha: уже rad; a,d: метры
    out.push_back(JointDH{
      j.theta_deg * DEG2RAD,
      j.a_m,
      j.d_m,
      j.alpha_rad
    });
  }
  return out;
}

void Core::composeAll(const Snapshot& s, std::vector<std::array<double,16>>& transforms) {
  transforms.clear();
  if (s.empty()) return;

  double cumulative[4][4];     // текущая накопленная T0->i
  identity(cumulative);

  for (const auto& joint : s) {
    double local[4][4];
    makeA(joint.theta_deg, joint.a_m, joint.d_m, joint.alpha_rad, local);

    double next[4][4];
    mul(cumulative, local, next);

    // сохранить как плоский массив
    std::array<double,16> flat{};
    int idx = 0;
    for (int r = 0; r < 4; ++r)
      for (int c = 0; c < 4; ++c)
        flat[idx++] = next[r][c];
    transforms.push_back(flat);

    // шаг вперёд
    for (int r = 0; r < 4; ++r)
      for (int c = 0; c < 4; ++c)
        cumulative[r][c] = next[r][c];
  }
}

Interp Core::interpretOne(const std::array<double,16>& M) {
  // Из 4x4 вытаскиваем положение и три столбца поворотной матрицы (X, Y, Z)
  const auto at = [&](int r, int c)->double { return M[r*4 + c]; };

  // Позиция
  const double px = at(0,3), py = at(1,3), pz = at(2,3);

  // Столбцы R: X = col0, Y = col1, Z = col2
  // Берём сырые значения
  double xx = at(0,0), xy = at(1,0), xz = at(2,0);
  double yx, yy, yz; // объявим, но пересчитаем ниже
  double zx = at(0,2), zy = at(1,2), zz = at(2,2);

  // --- Орто-нормировка на всякий случай (числовая устойчивость) ---
  // Нормируем Z
  auto norm = [](double& a, double& b, double& c){
    const double n = std::sqrt(a*a + b*b + c*c);
    if (n > 1e-12) { a/=n; b/=n; c/=n; }
  };

  // Z
  norm(zx, zy, zz);

  // Проецируем X на плоскость, ортогональную Z, затем нормируем
  // X = X - (X·Z) Z
  const double x_dot_z = xx*zx + xy*zy + xz*zz;
  xx -= x_dot_z * zx;  xy -= x_dot_z * zy;  xz -= x_dot_z * zz;
  norm(xx, xy, xz);

  // Восстанавливаем Y как Z × X, чтобы гарантировать правый ортонормированный базис
  yx = zy*xz - zz*xy;
  yy = zz*xx - zx*xz;
  yz = zx*xy - zy*xx;
  norm(yx, yy, yz);

  // Возвращаем полный базис + позицию
  Interp out{};
  out.x = px; out.y = py; out.z = pz;
  out.xx = xx; out.xy = xy; out.xz = xz;
  out.yx = yx; out.yy = yy; out.yz = yz;
  out.zx = zx; out.zy = zy; out.zz = zz;
  return out;
}

Results Core::interpretAll(const std::vector<std::array<double,16>>& transforms) {
  Results r;
  r.reserve(transforms.size());
  for (const auto& t : transforms) r.push_back(interpretOne(t));
  return r;
}

// ---- Публичный фасад ----
Results Core::computeForwardKinematics() const {
  // 1) Нормализуем единицы (theta->rad)
  const Snapshot s = normalizeUnits(input());

  // 2) Композиция всех T0->i
  std::vector<std::array<double,16>> transforms;
  composeAll(s, transforms);

  // 3) Интерпретация
  return interpretAll(transforms);
}

