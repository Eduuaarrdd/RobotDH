#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent),
    ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  // --- Оформление QLCDNumber: плоские сегменты, без рамок, свои цвета ---
  ui->xLcd->setSegmentStyle(QLCDNumber::Flat);
  ui->yLcd->setSegmentStyle(QLCDNumber::Flat);
  ui->zLcd->setSegmentStyle(QLCDNumber::Flat);

  ui->xLcd->setFrameShape(QFrame::NoFrame);
  ui->yLcd->setFrameShape(QFrame::NoFrame);
  ui->zLcd->setFrameShape(QFrame::NoFrame);

  // Запас по разрядам (чтобы не обрезало)
  ui->xLcd->setDigitCount(6);
  ui->yLcd->setDigitCount(6);
  ui->zLcd->setDigitCount(6);

  // Мелкая десятичная точка
  ui->xLcd->setSmallDecimalPoint(true);
  ui->yLcd->setSmallDecimalPoint(true);
  ui->zLcd->setSmallDecimalPoint(true);

  // Цвета сегментов + лёгкий фон под каждый
  ui->xLcd->setStyleSheet(
      "QLCDNumber { color: #16a34a; background:#f0fff4; border:1px solid #bbf7d0; }"); // зелёный
  ui->yLcd->setStyleSheet(
      "QLCDNumber { color: #b45309; background:#fffbeb; border:1px solid #fde68a; }"); // жёлто-оранжевый
  ui->zLcd->setStyleSheet(
      "QLCDNumber { color: #dc2626; background:#fef2f2; border:1px solid #fecaca; }"); // красный


  // Встроим 3D окно в QFrame (см. дерево .ui — это верхний frame под вертикальным лэйаутом)
  visual_.init3D(ui->renderFrame);

  // Создаём прослойку App (QObject как родитель — необязательно, но удобно)
  app_ = std::make_unique<App>(core_, visual_, this);

  // Визуализируем таблицу дефолтными значениями при старте (длина из Presets)
  app_->showStartupTable(ui->inputTable);

  // Коннектим кнопку к слоту App, передаём таблицу как аргумент.
  connect(ui->calculateBtn, &QPushButton::clicked, this, [this]{
    app_->onCalculateClicked(ui->inputTable, ui->xLcd, ui->yLcd, ui->zLcd);
  });

  // Кнопки вида камеры через App
  connect(ui->homeBtn, &QPushButton::clicked, app_.get(), &App::onHomeClicked);
  connect(ui->xyBtn,   &QPushButton::clicked, app_.get(), &App::onXYClicked);
  connect(ui->yzBtn,   &QPushButton::clicked, app_.get(), &App::onYZClicked);
  connect(ui->zyBtn,   &QPushButton::clicked, app_.get(), &App::onZYClicked);

  // --- Кнопки-ячейки в таблице: сигналы от Visual -> слоты App (передаём ссылку на таблицу)
  connect(&visual_, &Visual::requestRemoveRow, this, [this](int row){
    app_->onRemoveRow(ui->inputTable, row);
  });
  connect(&visual_, &Visual::requestInsertRowBelow, this, [this](int row){
    app_->onInsertRowBelow(ui->inputTable, row);
  });

  // Очистить и По умолчанию
  connect(ui->clearBtn,   &QPushButton::clicked, this, [this]{
    app_->onClearClicked(ui->inputTable);
  });
  connect(ui->defaultBtn, &QPushButton::clicked, this, [this]{
    app_->onDefaultClicked(ui->inputTable);
  });


}

MainWindow::~MainWindow() = default;
