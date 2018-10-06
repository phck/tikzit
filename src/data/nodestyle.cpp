/*
    TikZiT - a GUI diagram editor for TikZ
    Copyright (C) 2018 Aleks Kissinger

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "nodestyle.h"
#include "tikzit.h"

#include <QPainter>
#include <QImage>

NodeStyle *noneStyle = new NodeStyle();
NodeStyle *unknownStyle = new NodeStyle("unknown", new GraphElementData({GraphElementProperty("tikzit fill", "blue")}));

NodeStyle::NodeStyle() : Style()
{
}


NodeStyle::NodeStyle(QString name, GraphElementData *data): Style(name, data)
{
}

//QColor NodeStyle::fillColor(bool tikzitOverride) const
//{
//    if (_data == 0) return Qt::white;

//    QString col = propertyWithDefault("fill", "white", tikzitOverride);
//    return tikzit->colorByName(col);
//}

//QBrush NodeStyle::brush() const
//{
//    return QBrush(fillColor());
//}

//QString NodeStyle::shape(bool tikzitOverride) const
//{
//    return propertyWithDefault("shape", "circle", tikzitOverride);
//}

//QPainterPath NodeStyle::path() const
//{
//    QPainterPath pth;
//    QString sh = shape();

//    if (sh == "rectangle") {
//        pth.addRect(-30.0f, -30.0f, 60.0f, 60.0f);
//    } else { // default is 'circle'
//        pth.addEllipse(QPointF(0.0f,0.0f), 30.0f, 30.0f);
//    }
//    return pth;
//}

//QPainterPath NodeStyle::palettePath() const
//{
//    return path();
//}

//QIcon NodeStyle::icon() const
//{
//    // draw an icon matching the style
//    QImage px(100,100,QImage::Format_ARGB32_Premultiplied);
//    px.fill(Qt::transparent);


//    QPainter painter(&px);
//    painter.setRenderHint(QPainter::Antialiasing);
//    QPainterPath pth = path();
//    pth.translate(50.0f, 50.0f);

//    if (_data == 0) {
//        QColor c(180,180,200);
//        painter.setPen(QPen(c));
//        painter.setBrush(QBrush(c));
//        painter.drawEllipse(QPointF(50.0f,50.0f), 3,3);

//        QPen pen(QColor(180,180,220));
//        pen.setWidth(3);
//        QVector<qreal> p;
//        p << 2.0 << 2.0;
//        pen.setDashPattern(p);
//        painter.setPen(pen);
//        painter.setBrush(Qt::NoBrush);
//        painter.drawPath(pth);
//    } else {
//        painter.setPen(pen());
//        painter.setBrush(brush());
//        painter.drawPath(pth);
//    }

//    return QIcon(QPixmap::fromImage(px));
//}

//QString NodeStyle::category()
//{
//    return propertyWithDefault("tikzit category", "", false);
//}

