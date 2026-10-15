/* Copyright (c) 2026 Otto Link. Distributed under the terms of the GNU General
 * Public License. The full license is in the file LICENSE, distributed with
 * this software. */
#include <cmath>

#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QPainter>

#include "gnodegui/icon_button.hpp"

namespace gngui
{

IconButton::IconButton(AbstractIcon *icon, QWidget *parent) : QWidget(parent), icon(icon)
{
  const int size = (int)std::ceil(this->icon->get_width() +
                                  2.f * this->icon->pen().widthF());
  this->setFixedSize(size, size);
  this->setToolTip(this->icon->get_tooltip());
  this->setCursor(Qt::PointingHandCursor);

  // same shadow as the one AbstractIcon installs on its in-scene items
  auto effect = new QGraphicsDropShadowEffect(this);
  effect->setOffset(4, 4);
  effect->setBlurRadius(20);
  effect->setColor(Qt::black);
  this->setGraphicsEffect(effect);
}

void IconButton::enterEvent(QEnterEvent *event)
{
  this->opacity_factor = 0.5;
  this->update();
  QWidget::enterEvent(event);
}

void IconButton::leaveEvent(QEvent *event)
{
  this->opacity_factor = 1.;
  this->extra_pen_width = 0.f;
  this->update();
  QWidget::leaveEvent(event);
}

void IconButton::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton)
  {
    this->extra_pen_width = 1.f;
    this->update();

    Q_EMIT this->clicked();
  }
  QWidget::mousePressEvent(event);
}

void IconButton::mouseReleaseEvent(QMouseEvent *event)
{
  this->extra_pen_width = 0.f;
  this->update();
  QWidget::mouseReleaseEvent(event);
}

void IconButton::paintEvent(QPaintEvent *event)
{
  Q_UNUSED(event);

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setOpacity(this->opacity_factor);

  QPen pen = this->icon->pen();
  pen.setWidthF(pen.widthF() + this->extra_pen_width);
  painter.setPen(pen);
  painter.setBrush(this->icon->brush());
  painter.drawPath(this->icon->path());
}

} // namespace gngui
