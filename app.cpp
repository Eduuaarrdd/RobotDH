#include "app.h"
#include "presets.h"
#include <QMessageBox>

void App::showStartupTable(QTableWidget* table) const {
  // дефолтная длина (внутренний kDof из пресета)
  visual_.drawTable(table, Presets::Default());
}

void App::showStartupTable(QTableWidget* table, size_t dof) const {
  // явная длина
  visual_.drawTable(table, Presets::Default(dof));
}

void App::onCalculateClicked(QTableWidget* table, QLCDNumber* xLcd, QLCDNumber* yLcd, QLCDNumber* zLcd) {
  if (!table) return;

  // 1) Снять ввод из таблицы
  const Snapshot snap = visual_.readTable(table);

  // 2) Сохранить в ядро и посчитать FK
  core_.setInput(snap);
  Results results = core_.computeForwardKinematics();

  // 3) Передать результаты в визуал и обновить числовой вывод
  visual_.setComputed(results);          // это же автоматически отдаст данные в 3D
  visual_.updateLCDs(xLcd, yLcd, zLcd);  // последняя точка на LCD
}

void App::onHomeClicked() { visual_.home3D(); }
void App::onXYClicked()   { visual_.viewXY3D(); }
void App::onYZClicked()   { visual_.viewYZ3D(); }
void App::onZYClicked()   { visual_.viewZY3D(); }

void App::onRemoveRow(QTableWidget* table, int row) {
  if (!table) return;

  Snapshot s = visual_.readTable(table);
  const int rows = static_cast<int>(s.size());
  if (rows <= 1) {
    // Минимум 1 строка — предупреждаем
    QMessageBox::warning(table, QStringLiteral("Нельзя удалить"),
                         QStringLiteral("Это последняя строка. Минимум должна быть 1."));
    return;
  }
  if (row < 0 || row >= rows) return;

  // Удаляем выбранную строку
  s.erase(s.begin() + row);

  // Перерисовываем таблицу (имена Joint пересчитаются в setupHeaders)
  visual_.drawTable(table, s);
}

void App::onInsertRowBelow(QTableWidget* table, int row) {
  if (!table) return;

  Snapshot s = visual_.readTable(table);
  const int rows = static_cast<int>(s.size());
  if (row < 0 || row >= rows) return;

  // Вставляем "пустую" строку (нули) под row
  s.insert(s.begin() + (row + 1), JointDH{0.0, 0.0, 0.0, 0.0});

  // Перерисовываем таблицу (имена Joint пересчитаются в setupHeaders)
  visual_.drawTable(table, s);
}

void App::onClearClicked(QTableWidget* table) {
  if (!table) return;

  // Одна строка нулей
  Snapshot s;
  s.push_back(JointDH{0.0, 0.0, 0.0, 0.0});

  // Перерисовать таблицу
  visual_.drawTable(table, s);

  //Очистить вычисленные данные/3D — если нужно, раскомментируй:
  visual_.clearComputed();
  visual_.resetSceneToIdle();
}

void App::onDefaultClicked(QTableWidget* table) {
  if (!table) return;

  // Таблица как при старте (внутренний kDof из пресетов)
  visual_.drawTable(table, Presets::Default());
}
