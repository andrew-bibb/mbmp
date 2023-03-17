 /**************************** videowidget.cpp **************************

Code to manage the videowidget. Gstreamer renders a video stream onto
this widget

Copyright (C) 2014-2023
by: Andrew J. Bibb
License: MIT

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
***********************************************************************/

# include <QDebug>
# include <QPalette>

# include "./code/videowidget/videowidget.h"


// Constructor
VideoWidget::VideoWidget(QWidget* parent) : QWidget(parent)
{
	// for mouse events
	setEnabled(true);

	// set background to black
	QPalette pal = this->palette();
	pal.setColor(QPalette::Window, Qt::black);
	this->setPalette(pal);
	this->setAutoFillBackground(true);

	setFocusPolicy(Qt::StrongFocus);

}

///////////////////// Protected Functions /////////////////////////////
//
// Gstnavigation does not use double clicks so eat them
void VideoWidget::mouseDoubleClickEvent(QMouseEvent* e)
{
	e->accept();
}

//
// GstNavigation does not seem to need mouse moves, but send it on anyway
void VideoWidget::mouseMoveEvent(QMouseEvent* e)
{
        # if QT_VERSION < 0x060000
	emit navsignal ("mouse-move", 0, e->x(), e->y() );
        # else
	emit navsignal ("mouse-move", 0, e->position().x(), e->position().y() );
        #endif
	e->accept();
}

//
// Not actually sure it even needs mouse press events, but send them anyway
// Only send presses from the left button, allow right button to propagate up
void VideoWidget::mousePressEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton) {
           # if QT_VERSION < 0x060000
           emit navsignal ("mouse-button-press", 1, e->x(), e->y() );
           # else
           emit navsignal ("mouse-button-press", 1, e->position().x(), e->position().y() );
           # endif
           e->accept();
	}
	else e->ignore();
}

//
// GstNavigation does need mouse release events, a DVD on screen option is
// selected only on a button release, not a press.  As above, only send
// on left releases, allow right to propagate up.
void VideoWidget::mouseReleaseEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton) {
         # if QT_VERSION < 0x060000
            emit navsignal ("mouse-button-release", 1, e->x(), e->y());
         # else
            emit navsignal ("mouse-button-release", 1, e->position().x(), e->position().y());
         # endif
            e->accept();
	}
	else 	e->ignore();
}

//
// Eat keypresses, let the playerctl shortcuts do it
void VideoWidget::keyPressEvent(QKeyEvent* e)
{
	e->accept();
}

void VideoWidget::keyReleaseEvent(QKeyEvent* e)
{
	e->accept();
}
