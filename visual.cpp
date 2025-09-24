#include "visual.h"
#include "presets.h"
#include "doublespindelegate.h"
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QAbstractButton>
#include <QToolButton>
#include <QHBoxLayout>
#include <QStyle>

//Установка заголовков колонок и строк
void Visual::setupHeaders(QTableWidget* table, int rows) const {
  table->clear();
  table->setRowCount(rows);

  // 4 данных + 2 действий (Delete / Add)
  constexpr int kDataCols = 4;
  constexpr int kActionCols = 2;
  table->setColumnCount(kDataCols + kActionCols);

  // Заголовки колонок (с единицами)
  QStringList cols;
  for (int c = 0; c < kDataCols; ++c) cols << Presets::ColumnHeaders()[c];
  cols << "" << ""; // столбцы кнопок — без текста в заголовке
  table->setHorizontalHeaderLabels(cols);

  // Заголовки строк (Joint 1..N)
  QStringList rowNames;
  Presets::MakeJointNames(static_cast<size_t>(rows), rowNames);
  table->setVerticalHeaderLabels(rowNames);

  // Немного оформления
  auto* hh = table->horizontalHeader();
  auto* vh = table->verticalHeader();

  // Данные тянутся, кнопочные — по содержимому
  for (int c = 0; c < kDataCols; ++c)
    hh->setSectionResizeMode(c, QHeaderView::Stretch);
  for (int c = kDataCols; c < kDataCols + kActionCols; ++c)
    hh->setSectionResizeMode(c, QHeaderView::ResizeToContents);

  vh->setSectionResizeMode(QHeaderView::Stretch);   // строки делят высоту
  vh->setMinimumSectionSize(24);

  table->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  table->setAlternatingRowColors(true);
  table->setEditTriggers(QAbstractItemView::AllEditTriggers);

  // Серые заголовки и угловая ячейка
  table->setStyleSheet(
      "QHeaderView::section { background-color: #e0e0e0; font-weight: 600; }"
      "QTableCornerButton::section { background-color: #e0e0e0; border: 1px solid lightgray; }"
  );

  // Делегат со спинбоксами для ПЕРВЫХ 4 столбцов (кнопки — это виджеты, делегат их не трогает)
  table->setItemDelegateForColumn(0, new DoubleSpinDelegate(table));
  table->setItemDelegateForColumn(1, new DoubleSpinDelegate(table));
  table->setItemDelegateForColumn(2, new DoubleSpinDelegate(table));
  table->setItemDelegateForColumn(3, new DoubleSpinDelegate(table));
}

//Нарисовать таблицу
void Visual::drawTable(QTableWidget* table, const Snapshot& snap) const {
  const int rows = static_cast<int>(snap.size());
  setupHeaders(table, rows);

  for (int r = 0; r < rows; ++r) {
    const auto& j = snap[static_cast<size_t>(r)];

    const double vals[4] = { j.theta_deg, j.a_m, j.d_m, j.alpha_rad };
    const char* tips [4] = { "θ, градусы", "a, метры", "d, метры", "α, радианы" };

    for (int c = 0; c < 4; ++c) {
      auto* item = new QTableWidgetItem(fmt(vals[c]));
      item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      item->setToolTip(QString::fromUtf8(tips[c]));
      table->setItem(r, c, item);
    }

    // Кнопки действий в последних двух столбцах
    placeActionButtonsForRow(table, r);
  }
}

//Прочитать таблицу
Snapshot Visual::readTable(const QTableWidget* table) const {
  const int rows = table->rowCount();
  Snapshot s;
  s.resize(static_cast<size_t>(rows));

  auto readCell = [&](int r, int c)->double {
    if (auto* it = table->item(r, c)) return it->text().toDouble();
    return 0.0; // пока без валидации по ТЗ
  };

  for (int r = 0; r < rows; ++r) {
    s[static_cast<size_t>(r)].theta_deg = readCell(r, 0);
    s[static_cast<size_t>(r)].a_m       = readCell(r, 1);
    s[static_cast<size_t>(r)].d_m       = readCell(r, 2);
    s[static_cast<size_t>(r)].alpha_rad = readCell(r, 3);
  }
  return s;
}

void Visual::updateLCDs(QLCDNumber* xLcd, QLCDNumber* yLcd, QLCDNumber* zLcd) const {
  if (results_.empty() || !xLcd || !yLcd || !zLcd) return;

  const Interp& ee = results_.back(); // конец цепи
  xLcd->display(ee.x);
  yLcd->display(ee.y);
  zLcd->display(ee.z);
}

// Создать и поставить две кнопки в конце строки r: [ X ] [ + ]
void Visual::placeActionButtonsForRow(QTableWidget* table, int row) const
{
  if (!table) return;

  // Индексы столбцов для кнопок (после 4-х столбцов с данными)
  constexpr int kColDelete = 4;
  constexpr int kColAdd    = 5;

  // Кнопка УДАЛИТЬ
  auto* btnDel = makeActionButton(
      table,
      QStringLiteral("×"),                         // крупный крест (можно заменить на "x")
      QStringLiteral("Удалить эту строку"),        // подсказка
      "del",                                       // метка сценария
      row);                                        // номер строки (зашиваем в property)

  // Разрешаем кнопке растягиваться по всей ячейке
  btnDel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  // Ставим кнопку в ячейку [row, kColDelete]
  table->setCellWidget(row, kColDelete, btnDel);

  //Кнопка ДОБАВИТЬ
  auto* btnAdd = makeActionButton(
      table,
      QStringLiteral("+"),                         // крупный плюс
      QStringLiteral("Добавить строку ниже"),
      "add",
      row);

  btnAdd->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  table->setCellWidget(row, kColAdd, btnAdd);
}

// Универсальный фабричный метод для кнопки
QWidget* Visual::makeActionButton(QWidget* parent,
                                  const QString& text,
                                  const QString& tooltip,
                                  const char* which,
                                  int row) const
{
  auto* b = new QToolButton(parent);

  // Плоский вид без рамок
  b->setAutoRaise(true);

  // Текстовый символ и тултип
  b->setText(text);
  b->setToolTip(tooltip);

  // Сохраняем служебные данные, чтобы понимать, по какой строке кликнули
  b->setProperty("row", row);
  b->setProperty("which", QString::fromLatin1(which));

  // Крупный шрифт и жирное начертание
  QFont f = b->font();
  f.setPointSize(16);
  f.setBold(true);
  b->setFont(f);

  // Центровка и цвет символа.
  // Цвет задаём стилем; фон оставляем прозрачным, чтобы гармонировало с таблицей.
  const bool isDel = (QString::fromLatin1(which) == "del");
  b->setStyleSheet(QStringLiteral(
      "QToolButton {"
      "  color: %1;"          // красный для del, зелёный для add
      "  font-weight: bold;"
      "}"
      "QToolButton::menu-indicator { image: none; }"
    ).arg(isDel ? "red" : "green"));

  // Растягивание по ячейке — на случай если ячейки высокие/широкие
  b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  // По клику эмитим соответствующий сигнал Visual с актуальным номером строки
  connect(b, &QToolButton::clicked, this, [this, b] {
    const int r = b->property("row").toInt();
    const QString w = b->property("which").toString();
    if (w == "del") {
      emit requestRemoveRow(r);
    } else {
      emit requestInsertRowBelow(r);
    }
  });

  return b;
}

void Visual::resetSceneToIdle() {
  results_.clear();                      // забываем вычисленные точки
  if (renderer_) {
    renderer_->showIdleScene();          // перерисовать "пустую" базовую сцену
    renderer_->home();                   // вернуть камеру в Home
  }
}

