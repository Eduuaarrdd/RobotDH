#pragma once
#include "initaldate.h"
#include <array>

class Core {
public:
  Core() = default;

  // Входные данные (снимок DH)
  void setInput(const Snapshot& s) { input_ = s; }
  const Snapshot& input() const { return input_; }

  // Главный фасад: прям. кинематика по текущему input()
  // Возвращает интерпретированные данные для каждого звена (Joint0..JointN-1)
  Results computeForwardKinematics() const;

private:
  // ---- Вспомогательная математика (классический DH) ----
  // Единичная 4x4
  static void identity(double T[4][4]);

  // Умножение 4x4: Out = L * R
  static void mul(const double L[4][4], const double R[4][4], double Out[4][4]);

  // Локальная DH-матрица A_i(theta,a,d,alpha): Rz(theta)*Tz(d)*Tx(a)*Rx(alpha)
  static void makeA(double theta_rad, double a_m, double d_m, double alpha_rad, double A[4][4]);

  // Нормализация единиц: theta_deg->rad; alpha уже в rad; a,d — метры (без изменений)
  static Snapshot normalizeUnits(const Snapshot& s);

  // Накопить все T0->i
  static void composeAll(const Snapshot& s, std::vector<std::array<double,16>>& transforms);

  // Интерпретировать одну T0->i
  static Interp interpretOne(const std::array<double,16>& Tflat);

  // Интерпретировать все
  static Results interpretAll(const std::vector<std::array<double,16>>& transforms);

private:
  Snapshot input_{};
};

