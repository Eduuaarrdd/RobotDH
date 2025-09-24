#include "presets.h"
#include <cmath>
#include <QStringList>
#include <algorithm>

namespace {
constexpr double PI = 3.14159265358979323846;

// "Базовый" DOF из ТЗ (для дефолтных пресетов); таблица может быть любой длины
constexpr int kDof = 6;

// База из ТЗ на 6 звеньев
constexpr JointDH kTZ6[kDof] = {
  // theta_deg,   a_m,     d_m,     alpha_rad
  {  15.0,        0.0,     0.213,   +PI/2.0  }, // Joint 1
  { -50.0,       -0.8,     0.193,    0.0     }, // Joint 2
  { -60.0,      -0.590,   -0.160,    0.0     }, // Joint 3
  {  95.0,        0.0,     0.250,   +PI/2.0  }, // Joint 4
  {  50.0,        0.0,     0.280,   -PI/2.0  }, // Joint 5
  {   0.0,        0.0,     0.250,    0.0     }, // Joint 6
};

constexpr const char* kColumnHeaders[4] = {
  "theta (deg)", "a (m)", "d (m)", "alpha (rad)"
};

} // namespace

namespace Presets {

// вызываем вариант с параметром, используя внутренний kDof
Snapshot Default() {
  return Default(static_cast<size_t>(kDof));
}

Snapshot Default(size_t dof) {
  Snapshot out;
  out.reserve(dof);

  // 1) Берём сколько надо из ТЗ
  const size_t take = std::min(dof, static_cast<size_t>(kDof));
  for (size_t i = 0; i < take; ++i) {
    out.push_back(kTZ6[i]);
  }

  // 2) Если запросили больше — дополним нулями
  for (size_t i = take; i < dof; ++i) {
    out.push_back(JointDH{0.0, 0.0, 0.0, 0.0});
  }

  return out;
}

const char* const* ColumnHeaders() { return kColumnHeaders; }

void MakeJointNames(size_t n, QStringList& out) {
  out.clear();
  out.reserve(static_cast<int>(n));
  for (size_t i = 0; i < n; ++i) {
    out << QString("Joint %1").arg(static_cast<int>(i + 1));
  }
}

} // namespace Presets
