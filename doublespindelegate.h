#pragma once
#include <QStyledItemDelegate>
#include <QDoubleSpinBox>

class DoubleSpinDelegate : public QStyledItemDelegate {
  Q_OBJECT
public:
  explicit DoubleSpinDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex&) const override {
    auto* spin = new QDoubleSpinBox(parent);
    spin->setDecimals(3);
    spin->setSingleStep(0.001);
    spin->setRange(-1e9, 1e9);        // широкий диапазон
    spin->setKeyboardTracking(false); // применяем по фокусу/enter
    return spin;
  }

  void setEditorData(QWidget* editor, const QModelIndex& index) const override {
    auto* spin = qobject_cast<QDoubleSpinBox*>(editor);
    spin->setValue(index.data(Qt::EditRole).toDouble());
  }

  void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override {
    auto* spin = qobject_cast<QDoubleSpinBox*>(editor);
    model->setData(index, spin->value(), Qt::EditRole);
    model->setData(index, QString::number(spin->value(), 'f', 3), Qt::DisplayRole); // вывод с 3 знаками
  }

  void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex&) const override {
    editor->setGeometry(option.rect);
  }
};
