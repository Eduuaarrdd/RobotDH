#pragma once
#include <QObject>
#include <QWidget>
#include <QFrame>
#include <QVector3D>
#include <vector>

// Qt3D
#include <Qt3DExtras/qt3dwindow.h>
#include <Qt3DCore/QEntity>
#include <Qt3DRender/QCamera>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DCore/QTransform>
#include <Qt3DLogic/QFrameAction>

#include "initaldate.h" // Results

struct TextBillboard
{
    QVector3D             basePos;
    float                 scale = 1.0f;
    Qt3DCore::QTransform* xform = nullptr;
};

// Рендерер 3D-сцены по результатам вычислений.
// Правила:
// - Цвета: X — зелёный, Y — жёлтый, Z — красный.
// - Подписи только у БАЗОВОЙ оси (X,Y,Z).
// - Цилиндры-сегменты окрашены в цвет соответствующей оси.
// - X_i-цилиндр: между пересечениями с Z_{i-1} и Z_i. Если длина <= 1e-5 — X (и ось X) не строим.
// - Z_i-цилиндр: от p_i до пересечения с X_{i+1}. Для TCP цилиндр не строим.
// - Все три оси кадра i стартуют из одной точки p_i. Длина оси = 1.5 * длины цилиндра,
//   для Y_i длину берём как у оси Z_i.
// - База: рисуем оси + подписи; цилиндр базы только по Z_base до пересечения с X0;
//   по X_base цилиндр НЕ строим.
// - TCP: красная сфера + подпись "TCP".

class Render3D : public QObject {
  Q_OBJECT
public:
  QWidget* containerWidget() const { return viewContainer_; }

  explicit Render3D(QObject* parent = nullptr);

  // Инициализация рендера внутри виджета-контейнера (QFrame из UI)
  void initInto(QFrame* container);

  // Обновить данные сцены
  void setData(const Results& results);

  // Сброс камеры в исходное положение (кнопка Home)
  void home();

  // Быстрые виды камеры
  void viewXY(); // сверху на плоскость XY  (Z вверх)
  void viewYZ(); // вид на плоскость YZ    (X вправо, Z вверх)
  void viewZY(); // вид на плоскость ZY    (Y вправо, Z вверх)

  // Показ "статичных" базовых осей до первого расчёта
  void showIdleScene();

private:
  // --- построение сцены ---
  void clearScene();
  void buildBaseAxes();   // базовые оси + подписи + цилиндр по Z_base
  void buildJointAxes();  // оси+цилиндры для звеньев (по Results)
  void buildTCP();        // сфера и подпись TCP

  // --- билдеры примитивов ---
  Qt3DCore::QEntity* makeAxisEntity(const QVector3D& origin,
                                    const QVector3D& dir,
                                    float length,
                                    const QColor& color,
                                    float radius = 0.01f);
  Qt3DCore::QEntity* makeCylinder(const QVector3D& origin,
                                  const QVector3D& dir,
                                  float length,
                                  float radius,
                                  const QColor& color);
  Qt3DCore::QEntity* makeSphere(const QVector3D& center,
                                float radius,
                                const QColor& color,
                                float alpha = 1.0f);
  Qt3DCore::QEntity* makeTextLabel(const QVector3D& pos,
                                   const QString& text,
                                   float scale,
                                   const QColor& color);

  // --- математика/утилиты ---
  static QQuaternion rotationFromYTo(const QVector3D& dir);
  static QQuaternion billboardUpright(const QVector3D& toCam, const QVector3D& camUp);
  static QVector3D toVec3(double x, double y, double z) { return QVector3D(float(x), float(y), float(z)); }
  static bool closestPointsOnLines(const QVector3D& p1, const QVector3D& u,
                                   const QVector3D& p2, const QVector3D& v,
                                   QVector3D& q1, QVector3D& q2);
  static float signedProjLen(const QVector3D& a, const QVector3D& dir) {
    const float n = dir.length();
    if (n < 1e-12f) return 0.f;
    return QVector3D::dotProduct(a, dir / n);
  }

  // Qt3D
  Qt3DExtras::Qt3DWindow*            view_       = nullptr;
  QWidget*                           viewContainer_ = nullptr;
  Qt3DCore::QEntity*                 root_       = nullptr;
  Qt3DRender::QCamera*               camera_     = nullptr;
  Qt3DExtras::QOrbitCameraController*camCtrl_    = nullptr;

  // Данные
  Results  results_;

  // Стиль/масштаб
  float axisRadius_  = 0.01f;
  float baseAxesLen_ = 0.5f;
  float tubeRadius_  = 0.02f;
  QColor axisXColor_ = QColor( 22, 163,  74);     // X — зелёный
  QColor axisYColor_ = QColor(245, 158,  11);     // Y — жёлтый
  QColor axisZColor_ = QColor(220,  38,  38);     // Z — красный

  // Положение камеры "Home"
  QVector3D camHomePos_{3.0f, 3.0f, 2.0f};
  QVector3D camHomeCenter_{0.0f, 0.0f, 0.0f};
  QVector3D camHomeUp_{0.0f, 0.0f, 1.0f};

  // Цвет TCP (синий)
  QColor tcpColor_ = QColor( 37,  99, 235);

  Qt3DLogic::QFrameAction* frameAction_ = nullptr;
  std::vector<TextBillboard> labels_;      // все текстовые ярлыки
  void onFrameUpdate(float dt);            // обновление отступа к камере
};
