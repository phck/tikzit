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

#ifndef NODESTYLE_H
#define NODESTYLE_H

#include "style.h"

#include <QColor>
#include <QPen>
#include <QBrush>
#include <QPainterPath>
#include <QIcon>

class NodeStyle : public Style
{
public:
    NodeStyle();
    NodeStyle(QString name, GraphElementData *data);

//    QColor fillColor(bool tikzitOverride=true) const;
//    QBrush brush() const;
//    QPainterPath path() const override;
//    QString shape(bool tikzitOverride=true) const;

//    QPainterPath palettePath() const override;
//    QIcon icon() const override;

};

extern NodeStyle *noneStyle;
extern NodeStyle *unknownStyle;

#endif // NODESTYLE_H
