/*  YUView - YUV player with advanced analytics toolset
*   Copyright (C) 2015  Institut für Nachrichtentechnik
*                       RWTH Aachen University, GERMANY
*
*   YUView is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   YUView is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with YUView.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "displaywidget.h"
#include "frameobject.h"

#include <QPainter>
#include <QMessageBox>
#include <QSettings>
#include <QUrl>
#include <QTextDocument>
#define _USE_MATH_DEFINES
#include <math.h>

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

DisplayWidget::DisplayWidget(QWidget *parent) : QWidget(parent)
{
    p_drawGrid =  false;
    p_gridSize = 64;

    p_displayObject = NULL;
    p_overlayStatisticsObject = NULL;

    p_displayRect = QRect();

    p_selectionRect = QRect();
    p_zoomBoxPoint = QPoint();

    QPalette Pal(palette());
    // load color from preferences
    QSettings settings;
    QColor bgColor = settings.value("Background/Color").value<QColor>();
    Pal.setColor(QPalette::Background, bgColor);
    setAutoFillBackground(true);
    setPalette(Pal);
}

DisplayWidget::~DisplayWidget() {
}

void DisplayWidget::drawFrame(unsigned int frameIdx)
{
    // make sure that frame objects contain requested frame
    if( p_displayObject != NULL )
        p_displayObject->loadImage(frameIdx);
    if( p_overlayStatisticsObject )
        p_overlayStatisticsObject->loadImage(frameIdx);

    // redraw -- CHECK: repaint() might be an alternative here?!
    repaint();
}
void DisplayWidget::centerView() {

    int offsetX = floor((width() - p_displayRect.width())/2.0);
    int offsetY = floor((height() - p_displayRect.height())/2.0);
    QPoint topLeft(offsetX, offsetY);
    QPoint bottomRight(p_displayRect.width() + offsetX-1, p_displayRect.height()-1 + offsetY);
    p_displayRect = QRect(topLeft, bottomRight);
    update();

}

void DisplayWidget::centerView(bool isRight) {

    // TODO: this is not quite the comparison view yet, but close :-)
    QPoint topLeft,bottomRight;
    if (isRight)
    {
        if (width()<=p_displayRect.width())
        {
            bottomRight.setX(width());
        }
        else
        {
        bottomRight.setX((width()+p_displayRect.width())/2);
        }
        bottomRight.setY((height()+p_displayRect.height())/2);
        topLeft.setX(bottomRight.x()-p_displayRect.width()+1);
        topLeft.setY(bottomRight.y()-p_displayRect.height()+1);

    }
    else
    {
        if (width()<=p_displayRect.width())
        {
            topLeft.setX(0);
        }
        else
        {
        topLeft.setX((width()-p_displayRect.width())/2);
        }
        topLeft.setY((height()-p_displayRect.height())/2);
        bottomRight.setX(topLeft.x()+p_displayRect.width()-1);
        bottomRight.setY(topLeft.y()+p_displayRect.height()-1);
    }
    p_displayRect = QRect(topLeft, bottomRight);
    update();
}

void DisplayWidget::resetView()
{
    p_displayRect.setWidth(displayObject()->displayImage().width());
    p_displayRect.setHeight(displayObject()->displayImage().height());
    centerView();
}

void DisplayWidget::drawFrame()
{
    QPixmap image = p_displayObject->displayImage();

    if(p_displayRect.isEmpty())
    {
        int offsetX = floor((width() - image.width())/2);
        int offsetY = floor((height() - image.height())/2);
        QPoint topLeft(offsetX, offsetY);
        QPoint bottomRight(image.width()-1 + offsetX, image.height()-1 + offsetY);
        p_displayRect = QRect(topLeft, bottomRight);
    }

    //draw Frame
    QPainter painter(this);
    painter.drawPixmap(p_displayRect, image, image.rect());
}

void DisplayWidget::drawRegularGrid()
{
    if(p_displayRect.isEmpty())
    {
        QPixmap image = p_displayObject->displayImage();

        int offsetX = (width() - image.width())/2;
        int offsetY = (height() - image.height())/2;
        QPoint topLeft(offsetX, offsetY);
        QPoint bottomRight(image.width() + offsetX, image.height() + offsetY);
        p_displayRect = QRect(topLeft, bottomRight);
    }

    // draw regular grid
    QPainter painter(this);
    const int stepSize = p_gridSize*zoomFactor();
    for (int i=0; i<p_displayRect.width(); i+=stepSize)
    {
        QPoint start = p_displayRect.topLeft() + QPoint(i,0);
        QPoint end = p_displayRect.bottomLeft() + QPoint(i,0);
        painter.drawLine(start, end);
    }
    for (int i=0; i<p_displayRect.height(); i+=stepSize)
    {
        QPoint start = p_displayRect.bottomLeft() - QPoint(0,i);
        QPoint end = p_displayRect.bottomRight() - QPoint(0,i);
        painter.drawLine(start, end);
    }
}

void DisplayWidget::drawStatisticsOverlay()
{
    QPixmap overlayImage = p_overlayStatisticsObject->displayImage();

    if(p_displayRect.isEmpty())
    {
        int offsetX = (width() - overlayImage.width())/2;
        int offsetY = (height() - overlayImage.height())/2;
        QPoint topLeft(offsetX, offsetY);
        QPoint bottomRight(overlayImage.width() + offsetX, overlayImage.height() + offsetY);
        p_displayRect = QRect(topLeft, bottomRight);
    }

    //draw Frame
    QPainter painter(this);
    painter.drawPixmap(p_displayRect, overlayImage, overlayImage.rect());
}

void DisplayWidget::clear()
{
    QPalette Pal(palette());
    // load color from preferences
    QSettings settings;
    QColor bgColor = settings.value("Background/Color").value<QColor>();
    Pal.setColor(QPalette::Background, bgColor);
    setAutoFillBackground(true);
    setPalette(Pal);

    // TODO: clear?!
}

void DisplayWidget::paintEvent(QPaintEvent * event)
{
    // check if we have at least one object to draw
    if( p_displayObject != NULL )
    {
        drawFrame();

        // draw overlay, if requested
        if(p_overlayStatisticsObject)
        {
            drawStatisticsOverlay();
        }

        if(p_drawGrid)
        {
            drawRegularGrid();
        }

        // draw rectangular selection area.
        if (!p_selectionRect.isEmpty())
        {
            drawSelectionRectangle();
        }

        // draw Zoombox
        if (!p_zoomBoxPoint.isNull())
        {
            drawZoomBox();
        }
    }
}

void DisplayWidget::drawSelectionRectangle()
{
    QPainter painter(this);
    painter.drawRect(p_selectionRect);
}

void DisplayWidget::drawZoomBox()
{
    // check if we have at least one object to draw
    if( p_displayObject == NULL )
        return;

    QPoint srcPoint = (p_zoomBoxPoint - p_displayRect.topLeft())/zoomFactor();

    // zoom in
    const int zoomBoxFactor = 16;
    const int srcSize = 5;
    const int targetSize = srcSize*zoomBoxFactor;
    const int margin = 11;
    const int padding = 6;    

    QPainter painter(this);
    // translate to lower right corner
    painter.translate(width()-targetSize-margin, height()-targetSize-margin);

    // fill zoomed image into rect
    QPixmap image = p_displayObject->displayImage();
    QRect srcRect = QRect(srcPoint.x()-(srcSize>>1), srcPoint.y()-(srcSize>>1), srcSize, srcSize);
    QRect targetRect = QRect(0, 0, targetSize, targetSize);
    painter.drawPixmap(targetRect, image, srcRect);

    // draw border
    painter.drawRect(targetRect);

    // mark pixel under cursor
    const int srcPixelSize = 1;
    const int targetPixelSize = srcPixelSize*zoomBoxFactor;
    QRect pixelRect = QRect((targetSize-targetPixelSize)/2, (targetSize-targetPixelSize)/2, targetPixelSize, targetPixelSize);
    painter.drawRect(pixelRect);

    // draw pixel info
    QColor pixelValue = p_displayObject->getPixelValue( srcPoint.x(), srcPoint.y() );

    QTextDocument textDocument;
    textDocument.setDefaultStyleSheet("* { color: #FFFFFF }");
    textDocument.setHtml(QString("<h4>Coordinates:</h4>"
                                 "<table width=\"100%\">"
                                 "<tr><td>X:</td><td align=\"right\">%1</td></tr>"
                                 "<tr><td>Y:</td><td align=\"right\">%2</td></tr>"
                                 "</table><br />"
                                 "<h4>Values:</h4>"
                                 "<table width=\"100%\">"
                                 "<tr><td>Y:</td><td align=\"right\">%3</td></tr>"
                                 "<tr><td>U:</td><td align=\"right\">%4</td></tr>"
                                 "<tr><td>V:</td><td align=\"right\">%5</td></tr>"
                                 "</table>"
                                 ).arg(srcPoint.x()).arg(srcPoint.y()).arg(pixelValue.red()).arg(pixelValue.green()).arg(pixelValue.blue())
            );
    textDocument.setTextWidth(textDocument.size().width());

    QRect rect(QPoint(0, 0), textDocument.size().toSize() + QSize(2 * padding, 2 * padding));
    painter.translate(-rect.width(), targetSize - rect.height());
    painter.setBrush(QColor(0, 0, 0, 70));
    painter.drawRect(rect);
    painter.translate(padding, padding);
    textDocument.drawContents(&painter);
    painter.end();
}

void DisplayWidget::setRegularGridParameters(bool show, int size, QColor gridColor) {
    p_drawGrid = show;
    p_gridSize = size;
    p_gridColor = gridColor;

    update();
}
