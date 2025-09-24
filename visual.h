#pragma once
#include <QTableWidget>
#include <QLCDNumber>
#include <QFrame>
#include <QObject>
#include <memory>

#include "initaldate.h"
#include "render3d.h"

// Визуальный слой: таблица + мост к 3D.
// Логика правок данных НЕ здесь — мы только рисуем и сообщаем о действиях пользователя.
class Visual : public QObject {
  Q_OBJECT
public:
  explicit Visual(QObject* parent = nullptr) : QObject(parent) {}

  QWidget* Container3D() const {
    return renderer_ ? renderer_->containerWidget() : nullptr;
  }

  // Нарисовать слепок в таблице. Таблица остаётся редактируемой.
  void drawTable(QTableWidget* table, const Snapshot& snap) const;

  // Подготовка заголовков/ширин/подсказок под указанное число строк.
  void setupHeaders(QTableWidget* table, int rows) const;

  // Считать текущее состояние таблицы в Snapshot.
  Snapshot readTable(const QTableWidget* table) const;

  // Принять рассчитанные результаты и передать в рендер
  void setComputed(const Results& r) { results_ = r; if (renderer_) renderer_->setData(results_); }

  // Очистить результаты (по желанию)
  void clearComputed() { results_.clear(); }

  // Показать конец цепи (последний элемент results_) на LCD
  void updateLCDs(QLCDNumber* xLcd, QLCDNumber* yLcd, QLCDNumber* zLcd) const;

  // --- 3D ---
  // Встроить Qt3D окно в указанный QFrame (контейнер)
  void init3D(QFrame* frame) {
    if (!renderer_) renderer_ = std::make_unique<Render3D>();
    renderer_->initInto(frame);
    renderer_->showIdleScene();
  }

  // Возврат камеры к "домашнему" виду
  void home3D() { if (renderer_) renderer_->home(); }

  // Быстрые виды камеры
  void viewXY3D() { if (renderer_) renderer_->viewXY(); }
  void viewYZ3D() { if (renderer_) renderer_->viewYZ(); }
  void viewZY3D() { if (renderer_) renderer_->viewZY(); }

  // Полный сброс 3D как при старте: базовые оси + подписи, камера "домой"
  void resetSceneToIdle();

signals:
  // Действия пользователя по кнопкам в ячейках
  void requestRemoveRow(int row) const;        // удалить КОНКРЕТНУЮ строку
  void requestInsertRowBelow(int row) const;   // добавить строку ПОД указанной

private:
  static QString fmt(double v) { return QString::number(v, 'f', 3); } // 3 знака

  // --- приватные хелперы для "кнопочных" столбцов ---
  void placeActionButtonsForRow(QTableWidget* table, int row) const;
  QWidget* makeActionButton(QWidget* parent,
                            const QString& text,
                            const QString& tooltip,
                            const char* which,
                            int row) const;

private:
  Results results_;                       // последний расчёт
  std::unique_ptr<Render3D> renderer_;    // инкапсулированный рендерер Qt3D
};

