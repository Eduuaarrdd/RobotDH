#pragma once
#include <vector>   // вместо <array>, т.к. теперь динамика

struct JointDH {
  double theta_deg;  // угол сочленения, градусы
  double a_m;        // линейный параметр a, метры
  double d_m;        // линейный параметр d, метры
  double alpha_rad;  // угол alpha, радианы
};

// Динамический снимок: количество строк таблицы может меняться
using Snapshot = std::vector<JointDH>;

// Интерпретированные данные для каждого звена: позиция + полный ортонормированный базис (X,Y,Z) в мировой СК
struct Interp {
  // Позиция начала кадра звена i в базовой СК
  double x, y, z;

  // Оси кадра звена i в базовой СК (единичные векторы, правый базис)
  double xx, xy, xz;  // ось X_i
  double yx, yy, yz;  // ось Y_i
  double zx, zy, zz;  // ось Z_i
};

using Results = std::vector<Interp>;

