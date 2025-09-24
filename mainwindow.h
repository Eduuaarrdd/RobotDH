#pragma once
#include <QMainWindow>
#include <memory>
#include "core.h"
#include "visual.h"
#include "app.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private:
  std::unique_ptr<Ui::MainWindow> ui;

  // Слои
  Core core_;
  Visual visual_;
  std::unique_ptr<App> app_;
};

