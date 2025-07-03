/*
	Copyright (c) 2015-2024 Applied Research Laboratories, The University of Texas
	at Austin (ARL:UT).
	
	SALSA is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License version 3 (GPL-3.0-only) as published
	by the Free Software Foundation.
	
	SALSA is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License along with
	SALSA.  If not, see https://www.gnu.org/licenses/.
*/
#ifndef LSAPROXYSTYLE_HPP
#define LSAPROXYSTYLE_HPP

#include <QProxyStyle>
#include <QMouseEvent>
#include <QStyleOption>

// This class exists solely to override one default application style so that
// Tree View items will expand on a mouseReleaseEvent instead of a mousePressEvent.
// This prevents a bug where items could sometimes be inadvertently disabled in the
// tree view when the user collapsed an include record.
//
// bbauer
// 07/28/2022 i1441
// The above comment is no longer true. The class also overrides the style for the
// TreeView when hovering while dragging content to drop.

class LSAProxyStyle : public QProxyStyle
{
public:
    int styleHint(StyleHint hint, const QStyleOption *option = 0,
                  const QWidget *widget = 0, QStyleHintReturn *returnData = 0) const
    {
      if (hint == QStyle::SH_ListViewExpand_SelectMouseType)
      {
          return QMouseEvent::MouseButtonRelease;
      }
      return QProxyStyle::styleHint(hint, option, widget, returnData);
    }

    // mostly thanks to https://stackoverflow.com/questions/7596584/qtreeview-draw-drop-indicator
    void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
    {
        if(element == QStyle::PE_IndicatorItemViewItemDrop && !option->rect.isNull())
        {
            QStyleOption custOpt(*option);
            // go out to left
            custOpt.rect.setLeft(0);
            if(widget)
            {
                // go out to right
                custOpt.rect.setRight(widget->width());
            }
            // make it a line
            custOpt.rect.setTop(custOpt.rect.bottom());
            QProxyStyle::drawPrimitive(element, &custOpt, painter, widget);
            return;
        }
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
};

#endif // LSAPROXYSTYLE_HPP
