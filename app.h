#pragma once
#include <QObject>
#include <QTableWidget>
#include "core.h"
#include "visual.h"

// Сервис уровня приложения: сценарии и координация слоёв.
class App : public QObject {
  Q_OBJECT
public:
  App(Core& core, Visual& visual, QObject* parent = nullptr)
    : QObject(parent), core_(core), visual_(visual) {}

  // Сценарий старта: показать дефолтную таблицу (любой длины)
  void showStartupTable(QTableWidget* table) const;            // дефолтная длина
  void showStartupTable(QTableWidget* table, size_t dof) const; // явная длина

  // Сделаем таблицу явным параметром в слоте — это ещё сильнее развяжет App от UI
public slots:
  // Нажали "Рассчитать":
  // 1) читаем таблицу -> core.setInput(...)
  // 2) считаем FK -> Results
  // 3) передаём Visual -> setComputed(...) и обновляем LCD
  void onCalculateClicked(QTableWidget* table, QLCDNumber* xLcd, QLCDNumber* yLcd, QLCDNumber* zLcd);

  // Кнопки вида камеры
  void onHomeClicked();
  void onXYClicked();
  void onYZClicked();
  void onZYClicked();

  //Кнопки таблицы
  void onRemoveRow(QTableWidget* table, int row);
  void onInsertRowBelow(QTableWidget* table, int row);

  //Очистить: одна строка, нули
  void onClearClicked(QTableWidget* table);

  //По умолчанию: как при запуске (Presets::Default())
  void onDefaultClicked(QTableWidget* table);

private:
  Core& core_;
  Visual& visual_;
};
