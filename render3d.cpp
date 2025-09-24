#include "render3d.h"

#include <Qt3DCore/QComponent>
#include <Qt3DExtras/QPhongAlphaMaterial>
#include <Qt3DRender/QPointLight>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DRender/QMaterial>
#include <Qt3DExtras/QExtrudedTextMesh>
#include <Qt3DExtras/QConeMesh> // не используем, но пусть останется если был в проекте
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QFont>
#include <cmath>
#include <algorithm>

Render3D::Render3D(QObject* parent) : QObject(parent) {}

void Render3D::initInto(QFrame* container) {
  if (!container) return;

  // Qt3D окно
  view_ = new Qt3DExtras::Qt3DWindow();
  view_->defaultFrameGraph()->setClearColor(QColor(240,240,240)); // light gray

  // Контейнер для встраивания в обычный виджет
  viewContainer_ = QWidget::createWindowContainer(view_, container);
  viewContainer_->setFocusPolicy(Qt::StrongFocus);

  // Верстка
  auto* lay = new QVBoxLayout(container);
  lay->setContentsMargins(0,0,0,0);
  lay->addWidget(viewContainer_);

  // Корень сцены
  root_ = new Qt3DCore::QEntity();

  // Камера
  camera_ = view_->camera();
  camera_->lens()->setPerspectiveProjection(45.0f, 16.f/9.f, 0.1f, 1000.0f);
  camera_->setPosition({3.0f, 3.0f, 2.0f});
  camera_->setViewCenter({0.0f, 0.0f, 0.0f});
  camera_->setUpVector({0.0f, 0.0f, 1.0f});

  // запомним "домой"
  camHomePos_    = camera_->position();
  camHomeCenter_ = camera_->viewCenter();
  camHomeUp_     = camera_->upVector();

  // Контроллер камеры (левая — орбита; правая — панорамирование по умолчанию)
  camCtrl_ = new Qt3DExtras::QOrbitCameraController(root_);
  camCtrl_->setCamera(camera_);
  camCtrl_->setLinearSpeed(8.0f);
  camCtrl_->setLookSpeed(180.0f);

  // Свет
  auto* lightEntity = new Qt3DCore::QEntity(root_);
  lightEntity->setObjectName(QStringLiteral("keep_light"));
  auto* light = new Qt3DRender::QPointLight(lightEntity);
  light->setColor(QColor(255, 255, 255));
  light->setIntensity(1.5f);
  lightEntity->addComponent(light);
  auto* lightTr = new Qt3DCore::QTransform();
  lightTr->setTranslation(QVector3D(10.f, 12.f, 10.f));
  lightEntity->addComponent(lightTr);

  view_->setRootEntity(root_);

  frameAction_ = new Qt3DLogic::QFrameAction(root_);
  QObject::connect(frameAction_, &Qt3DLogic::QFrameAction::triggered,
                   this, [this](float dt){ onFrameUpdate(dt); });
  root_->addComponent(frameAction_);
}

void Render3D::setData(const Results& results) {
  results_ = results;

  // Авто-масштаб по данным, чтобы оси/трубки были адекватной толщины
  float maxR = 1.0f;
  for (const auto& r : results_) {
    maxR = std::max(maxR, float(std::sqrt(r.x*r.x + r.y*r.y + r.z*r.z)));
  }
  baseAxesLen_ = std::max(0.3f, maxR * 0.3f);
  axisRadius_  = std::max(0.006f, maxR * 0.01f);
  tubeRadius_  = std::max(0.009f, maxR * 0.018f)* 2.0f;

  clearScene();
  buildBaseAxes();
  buildJointAxes();
  buildTCP();
}

void Render3D::clearScene() {
  if (!root_) return;

  labels_.clear();

  const auto ents = root_->findChildren<Qt3DCore::QEntity*>(
      QString(), Qt::FindDirectChildrenOnly);

  for (Qt3DCore::QEntity* e : ents) {
    if (!e) continue;

    // НЕ удаляем свет, контроллер и (на всякий) камеру
    if (e->objectName() == QStringLiteral("keep_light")) continue;
    if (e == camera_) continue;
    if (qobject_cast<Qt3DExtras::QOrbitCameraController*>(e)) continue;

    e->deleteLater();
  }
}

/*===========================  ПОСТРОЕНИЕ БАЗЫ  ===========================*/

void Render3D::buildBaseAxes() {
    // Базовые оси (XYZ) вокруг (0,0,0). Подписи — ТОЛЬКО здесь.
    const float L = baseAxesLen_;
    makeAxisEntity({0,0,0}, {1,0,0}, L, axisXColor_, axisRadius_);
    makeAxisEntity({0,0,0}, {0,1,0}, L, axisYColor_, axisRadius_);
    makeAxisEntity({0,0,0}, {0,0,1}, L, axisZColor_, axisRadius_);

    const float txt = std::max(0.045f, L*0.12f);
    makeTextLabel({ L,0,0 },  "X", txt, axisXColor_);
    makeTextLabel({ 0, L,0 }, "Y", txt, axisYColor_);
    makeTextLabel({ 0,0, L }, "Z", txt, axisZColor_);

    // Цилиндр базы ТОЛЬКО по Z: от (0,0,0) до p0, с УЧЁТОМ знака проекции
    if (!results_.empty()) {
      const QVector3D p0 = toVec3(results_.front().x, results_.front().y, results_.front().z);
      const QVector3D zBase(0,0,1);
      const float proj = signedProjLen(p0 - QVector3D(0,0,0), zBase);
      const float Lz   = std::fabs(proj);
      const QVector3D dirZ = (proj >= 0.f) ? zBase : -zBase;
      if (Lz > 1e-6f) {
        makeCylinder({0,0,0}, dirZ, Lz, tubeRadius_, axisZColor_);
      }
    }
}

/*===========================  ПОСТРОЕНИЕ ЗВЕНЬЕВ  ===========================*/

void Render3D::buildJointAxes() {
  if (results_.empty()) return;

  const int last = int(results_.size()) - 1; // TCP = last, для него цилиндры не строим

  auto ex_i = [&](int i){ const auto& r = results_[size_t(i)];
                          return toVec3(r.xx, r.xy, r.xz).normalized(); };
  auto ey_i = [&](int i){ const auto& r = results_[size_t(i)];
                          return toVec3(r.yx, r.yy, r.yz).normalized(); };
  auto ez_i = [&](int i){ const auto& r = results_[size_t(i)];
                          return toVec3(r.zx, r.zy, r.zz).normalized(); };
  auto p_i  = [&](int i){ const auto& r = results_[size_t(i)];
                          return toVec3(r.x, r.y, r.z); };

  for (int i = 0; i <= last; ++i) {
    const QVector3D p   = p_i(i);
    const QVector3D ex  = ex_i(i);
    const QVector3D ey  = ey_i(i);
    const QVector3D ez  = ez_i(i);

    /* ---------- Z_i: цилиндр от p_i до p_{i+1} в проекции на z_i ---------- */
    float Lz_len = 0.0f;          // модуль длины цилиндра
    QVector3D dirZ = ez;          // реальное направление цилиндра (с учётом знака)
    if (i < last) {
      const QVector3D dp = p_i(i+1) - p;          // вектор до следующего начала СК
      const float proj = signedProjLen(dp, ez);   // может быть < 0
      Lz_len = std::fabs(proj);
      dirZ   = (proj >= 0.f) ? ez : -ez;
    }
    // длины осей (оси — всегда вдоль +ex/+ey/+ez, длина по модулю сегмента Z)
    const float Lz_axis = (Lz_len > 0.f) ? (Lz_len * 1.5f) : (baseAxesLen_ * 0.6f);
    const float Ly_axis = Lz_axis;

    /* ---------- X_i: цилиндр от проекции p_i на Z_{i-1} до p_i ---------- */
    float Lx_len = 0.0f;         // модуль длины цилиндра
    QVector3D X_start = p;       // заменится вычислением
    QVector3D dirX = ex;         // реальное направление цилиндра (с учётом знака)

    if (i == 0) {
      // Z_{-1} := z_base через (0,0,0)
      const QVector3D zBase(0,0,1);
      const float t = QVector3D::dotProduct(p - QVector3D(0,0,0), zBase);
      const QVector3D s = QVector3D(0,0,0) + zBase * t; // точка на Z_base
      X_start = s;
      const float proj = signedProjLen(p - s, ex);
      Lx_len = std::fabs(proj);
      dirX   = (proj >= 0.f) ? ex : -ex;
    } else {
      // проекция p_i на линию Z_{i-1} : s = p_{i-1} + ez_{i-1} * t
      const QVector3D pPrev = p_i(i-1);
      const QVector3D zPrev = ez_i(i-1);
      const float t = QVector3D::dotProduct(p - pPrev, zPrev);
      const QVector3D s = pPrev + zPrev * t;
      X_start = s;
      const float proj = signedProjLen(p - s, ex);
      Lx_len = std::fabs(proj);
      dirX   = (proj >= 0.f) ? ex : -ex;
    }
    const bool buildX = (Lx_len > 1e-5f);               // если почти ноль — X не строим
    const float Lx_axis = buildX ? (Lx_len * 1.5f) : 0;  // ось — вперёд по +X (показываем базис)

    /* ---------- Оси (тонкие) всегда из p_i ---------- */
    if (buildX) makeAxisEntity(p, ex, Lx_axis, axisXColor_, axisRadius_);
    makeAxisEntity(p, ey, Ly_axis, axisYColor_, axisRadius_);
    makeAxisEntity(p, ez, Lz_axis, axisZColor_, axisRadius_);

    /* ---------- Цилиндры (толстые) ---------- */
    if (buildX) {
      makeCylinder(X_start, dirX, Lx_len, tubeRadius_, axisXColor_);
    }
    if (Lz_len > 1e-6f && i < last) {
      makeCylinder(p, dirZ, Lz_len, tubeRadius_, axisZColor_);
    }
  }
}


/*===========================  TCP  ===========================*/

void Render3D::buildTCP() {
  if (results_.empty()) return;
  const auto& e = results_.back();
  const QVector3D p = toVec3(e.x, e.y, e.z);

  // Синяя сфера TCP
  const float r = std::max(axisRadius_*1.2f, 0.015f) * 4.0f;
  makeSphere(p, r, tcpColor_, 1.0f);

  // Подпись TCP (синяя), чуть сместим к камере и вверх
  const float s = std::max(0.045f, baseAxesLen_*0.12f);
  makeTextLabel(p + QVector3D(0, s, 0), "TCP", s, tcpColor_);
}

/*===========================  БИЛДЕРЫ ПРИМИТИВОВ  ===========================*/

Qt3DCore::QEntity* Render3D::makeAxisEntity(const QVector3D& origin,
                                            const QVector3D& dir,
                                            float length,
                                            const QColor& color,
                                            float radius) {
  QVector3D d = dir;
  if (d.lengthSquared() < 1e-12f) return nullptr;
  d.normalize();

  auto* entity = new Qt3DCore::QEntity(root_);

  auto* mesh = new Qt3DExtras::QCylinderMesh();
  mesh->setLength(length);
  mesh->setRadius(radius);
  mesh->setRings(16);
  mesh->setSlices(24);

  auto* mat = new Qt3DExtras::QPhongMaterial();
  mat->setDiffuse(color);

  auto* tr = new Qt3DCore::QTransform();
  tr->setRotation(rotationFromYTo(d));
  tr->setTranslation(origin + d * (length * 0.5f));

  entity->addComponent(mesh);
  entity->addComponent(mat);
  entity->addComponent(tr);
  return entity;
}

Qt3DCore::QEntity* Render3D::makeCylinder(const QVector3D& origin,
                                          const QVector3D& dir,
                                          float length,
                                          float radius,
                                          const QColor& color) {
  QVector3D d = dir;
  if (d.lengthSquared() < 1e-12f || length <= 0.f) return nullptr;
  d.normalize();

  auto* e = new Qt3DCore::QEntity(root_);
  auto* mesh = new Qt3DExtras::QCylinderMesh();
  mesh->setLength(length);
  mesh->setRadius(radius);
  mesh->setRings(16);
  mesh->setSlices(24);

  auto* mat = new Qt3DExtras::QPhongMaterial();
  mat->setDiffuse(color);

  auto* tr = new Qt3DCore::QTransform();
  tr->setRotation(rotationFromYTo(d));
  tr->setTranslation(origin + d * (length * 0.5f));

  e->addComponent(mesh);
  e->addComponent(mat);
  e->addComponent(tr);
  return e;
}

Qt3DCore::QEntity* Render3D::makeSphere(const QVector3D& center,
                                        float radius,
                                        const QColor& color,
                                        float alpha) {
  auto* e = new Qt3DCore::QEntity(root_);
  auto* mesh = new Qt3DExtras::QSphereMesh();
  mesh->setRadius(radius);
  mesh->setRings(24);
  mesh->setSlices(24);

  Qt3DExtras::QPhongAlphaMaterial* mat = new Qt3DExtras::QPhongAlphaMaterial();
  mat->setDiffuse(color);
  mat->setAlpha(alpha);

  auto* tr = new Qt3DCore::QTransform();
  tr->setTranslation(center);

  e->addComponent(mesh);
  e->addComponent(mat);
  e->addComponent(tr);
  return e;
}

Qt3DCore::QEntity* Render3D::makeTextLabel(const QVector3D& pos,
                                           const QString& text,
                                           float scale,
                                           const QColor& color)
{
    auto* e   = new Qt3DCore::QEntity(root_);
    auto* m   = new Qt3DExtras::QExtrudedTextMesh();
    m->setText(text);
    m->setDepth(std::max(0.003f, scale * 0.15f));
    m->setFont(QFont("DejaVu Sans", 96, QFont::DemiBold));

    auto* mat = new Qt3DExtras::QPhongMaterial();
    mat->setDiffuse(color);

    // Трансформ: базовая позиция (смещение и поворот к камере обновляем в onFrameUpdate)
    auto* tr = new Qt3DCore::QTransform();
    tr->setScale(scale);
    tr->setTranslation(pos);

    e->addComponent(m);
    e->addComponent(mat);
    e->addComponent(tr);

    // Запомним для динамического смещения/поворота к камере
    labels_.push_back({ pos, scale, tr });
    return e;
}



/*===========================  МАТЕМАТИКА  ===========================*/

QQuaternion Render3D::rotationFromYTo(const QVector3D& dir) {
  // Базовый цилиндр ориентирован вдоль +Y (0,1,0)
  static const QVector3D up(0.f, 1.f, 0.f);
  QVector3D d = dir.normalized();

  const float dot = QVector3D::dotProduct(up, d);
  if (dot > 0.9999f) return QQuaternion();
  if (dot < -0.9999f) {
    // Противоположные направления: поворот на 180° вокруг любой оси, ортогональной up
    return QQuaternion::fromAxisAndAngle(QVector3D(1,0,0), 180.0f);
  }
  const QVector3D axis = QVector3D::crossProduct(up, d).normalized();
  const float angleDeg = std::acos(std::clamp(dot, -1.0f, 1.0f)) * 180.0f / float(M_PI);
  return QQuaternion::fromAxisAndAngle(axis, angleDeg);
}

bool Render3D::closestPointsOnLines(const QVector3D& p1, const QVector3D& u,
                                    const QVector3D& p2, const QVector3D& v,
                                    QVector3D& q1, QVector3D& q2) {
  const QVector3D uu = u.normalized();
  const QVector3D vv = v.normalized();
  const QVector3D w0 = p1 - p2;

  const float a = QVector3D::dotProduct(uu, uu); // =1
  const float b = QVector3D::dotProduct(uu, vv);
  const float c = QVector3D::dotProduct(vv, vv); // =1
  const float d = QVector3D::dotProduct(uu, w0);
  const float e = QVector3D::dotProduct(vv, w0);

  const float denom = a*c - b*b;
  if (std::abs(denom) < 1e-9f) {
    // Почти параллельные: выбираем проекцию p2 на L1 и ту же точку для L2
    const float s = -d; // (при a=1)
    q1 = p1 + uu * s;
    // Для q2 берём ближайшую точку на L2 к q1
    const float t = QVector3D::dotProduct(q1 - p2, vv);
    q2 = p2 + vv * t;
    return true;
  }

  const float s = (b*e - c*d) / denom; // параметр на L1
  const float t = (a*e - b*d) / denom; // параметр на L2
  q1 = p1 + uu * s;
  q2 = p2 + vv * t;
  return true;
}

void Render3D::showIdleScene() {
  // просто базовые оси одинаковой длины + подписи + цилиндр Z = 50% длины оси
  results_.clear();
  clearScene();

  const float L = 0.6f; // произвольная "красивая" длина на старте
  makeAxisEntity({0,0,0}, {1,0,0}, L, axisXColor_, axisRadius_);
  makeAxisEntity({0,0,0}, {0,1,0}, L, axisYColor_, axisRadius_);
  makeAxisEntity({0,0,0}, {0,0,1}, L, axisZColor_, axisRadius_);

  const float txt = std::max(0.045f, L*0.12f);
  makeTextLabel({ L,0,0 },  "X", txt, axisXColor_);
  makeTextLabel({ 0, L,0 }, "Y", txt, axisYColor_);
  makeTextLabel({ 0,0, L }, "Z", txt, axisZColor_);

  // цилиндр только по Z, короче на 50%
  makeCylinder({0,0,0}, {0,0,1}, L * 0.5f, tubeRadius_ * 2.0f, axisZColor_);
}

void Render3D::onFrameUpdate(float /*dt*/)
{
    if (!camera_) return;

    for (auto& L : labels_)
    {
        if (!L.xform) continue;

        QVector3D toCam = (camera_->position() - L.basePos);
        const float len2 = toCam.lengthSquared();
        if (len2 < 1e-9f) {
            toCam = QVector3D(0,0,1);
        } else {
            toCam /= std::sqrt(len2);
        }

        // отступ от объектов в сторону камеры
        const float nudge = std::max(axisRadius_ * 6.0f, L.scale * 0.15f);
        L.xform->setTranslation(L.basePos + toCam * nudge);

        // развернуть текст
        L.xform->setRotation(billboardUpright(toCam, camera_->upVector()));

    }
}


QQuaternion Render3D::billboardUpright(const QVector3D& toCam, const QVector3D& camUp)
{
    QVector3D f = toCam.normalized();          // forward: на камеру
    QVector3D u = camUp.normalized();          // вверх камеры
    QVector3D r = QVector3D::crossProduct(u, f);
    if (r.lengthSquared() < 1e-8f) {
        // если почти коллинеарны, возьмём запасной up
        QVector3D fallback(0,1,0);
        if (std::fabs(QVector3D::dotProduct(fallback.normalized(), f)) > 0.95f)
            fallback = QVector3D(1,0,0);
        r = QVector3D::crossProduct(fallback, f);
    }
    r.normalize();
    QVector3D u2 = QVector3D::crossProduct(f, r).normalized();

    QMatrix3x3 R;
    R(0,0)=r.x();  R(1,0)=r.y();  R(2,0)=r.z();
    R(0,1)=u2.x(); R(1,1)=u2.y(); R(2,1)=u2.z();
    R(0,2)=f.x();  R(1,2)=f.y();  R(2,2)=f.z();

    return QQuaternion::fromRotationMatrix(R);
}

void Render3D::home() {
  if (!camera_) return;
  camera_->setUpVector(camHomeUp_);
  camera_->setPosition(camHomePos_);
  camera_->setViewCenter(camHomeCenter_);
}

// вспомогательный: выставить камеру по направлению dir (на центр) и up
static void setCamDirUp(Qt3DRender::QCamera* cam, const QVector3D& dir, const QVector3D& up)
{
    if (!cam) return;
    const QVector3D center = cam->viewCenter();
    const float dist = (cam->position() - center).length();
    QVector3D f = dir.normalized();
    if (f.lengthSquared() < 1e-9f) f = QVector3D(0,0,-1);
    cam->setUpVector(up.normalized());
    cam->setViewCenter(center);
    cam->setPosition(center - f * (dist > 1e-4f ? dist : 3.0f));
}

void Render3D::viewXY()
{
    // Сверху вниз: камера над центром по +Z, смотрит в -Z; up — мировой +Y
    setCamDirUp(camera_, QVector3D(0,0,-1), QVector3D(0,1,0));
}

void Render3D::viewYZ()
{
    // Вид на плоскость YZ: камера по +X, смотрит в -X; up — мировой +Z
    setCamDirUp(camera_, QVector3D(-1,0,0), QVector3D(0,0,1));
}

void Render3D::viewZY()
{
    // Вид на плоскость ZY (эквивалент XZ, “слева/справа”): камера по +Y, смотрит в -Y; up — мировой +Z
    setCamDirUp(camera_, QVector3D(0,-1,0), QVector3D(0,0,1));
}
