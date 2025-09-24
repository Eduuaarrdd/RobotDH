#pragma once
#include "initaldate.h"
#include <QStringList>  // нужно для QStringList в сигнатуре

namespace Presets {

// Вариант 1: дефолтная длина из пресета (внутренний kDof живёт в .cpp)
Snapshot Default();

// Вариант 2: явно заданная длина (rows)
Snapshot Default(size_t dof);

// Подписи колонок (с единицами) — фиксированные 4 шт.
const char* const* ColumnHeaders();

// Сгенерировать имена Joint 1..N для вертикальных заголовков.
void MakeJointNames(size_t n, QStringList& out);

} // namespace Presets

