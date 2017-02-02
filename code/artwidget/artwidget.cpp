/**************************** artwidget.cpp **************************

Code to manage the widget used to display album art. 

Copyright (C) 2017
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
# include <QTimer>
# include <QFont>
# include <QFontMetrics>
# include <QPainter>
# include <QBrush>

# include "./code/artwidget/artwidget.h"	


// Constructor
ArtWidget::ArtWidget(QWidget* parent) : QLabel(parent)
{	
	// data members
	background = QPixmap();
	display = QPixmap();
	
	setAlignment(Qt::AlignCenter);
}


///////////////////// Public Functions /////////////////////////////
//
// Function to receive and process information to be displayed
void ArtWidget::setInfo(const QPixmap* src, const QString& title, const QString& artist, const int& dur)
{
	this->clear();
	if (src == NULL) return;
	
	// save data we need
	background = QPixmap(*src);
		
	// make a plain or overlaid display
	if (artist.isEmpty() && title.isEmpty() )
		makeDisplay();
	else
	  makeOverlaidDisplay(title, artist, dur);	
	
	
	return;
}

///////////////////// Public Slots /////////////////////////////
//
// Slot to make a plain display image
void ArtWidget::makeDisplay()
{
	qDebug() << "inside plain display:";
	display = background;

  // get dimensions of the label
	const int w = this->width();
	const int h = this->height();

	// set a scaled pixmap to a w x h window keeping its aspect ratio 
	this->setPixmap(display.scaled(w,h,Qt::KeepAspectRatio));	

	return;
}

//
// Slot to make an overlaid display
void ArtWidget::makeOverlaidDisplay(const QString& title, const QString& artist, const int& dur)
{
  //  constants
  const int titleheight = 36;
  const int artistheight = 20;
  const QString fontfamily = "Helvetica"; 
  const int w = this->width();
	const int h = this->height();
  
	// save and set variables
	int boxheight = -1;
	int boxwidth = -1;
	QFont curfont = QFont();
	qDebug() << "title: " << title << "artist: " << artist << "duration: " << dur;
	display = background.scaled(w,h,Qt::KeepAspectRatio);
	
	QFont titlefont = QFont(fontfamily, titleheight);
	QFont artistfont = QFont(fontfamily, artistheight);
	QFontMetrics titlemetrics = QFontMetrics(titlefont);
	QFontMetrics artistmetrics = QFontMetrics(artistfont);
	boxheight = titlemetrics.height() + artistmetrics.height();
	boxwidth = titlemetrics.width(title);
	if (artistmetrics.width(artist) > boxwidth ) boxwidth = artistmetrics.width(artist);
	
	// make sure there is enough room to display the notification
	if (boxheight > h  || boxwidth > w) {
		makeDisplay();
		return;
	}  
	
	// paint the overlay onto the background
	qDebug() << "starting to paint";
	QPainter p(&display); 
	p.setBrush(QBrush(QColor("darkgray"), Qt::SolidPattern) );
	p.drawRoundedRect(0, 0, boxwidth, boxheight, 10.0, 10.0);
	p.setPen(Qt::black);
	p.drawText(0, 20, "HI andy");
	p.end(); 


	// set a scaled pixmap to a w x h window keeping its aspect ratio 
	this->setPixmap(display);	
	
	// overlay timeout
	QTimer::singleShot(dur * 1000, this, SLOT(makeDisplay()) );
	
	return;
}
	

///////////////////// Protected Functions /////////////////////////////
//
// Have pixmap fill this label on resize
void ArtWidget::resizeEvent(QResizeEvent* e)
{
	(void) e;
	if (pixmap() == NULL) return;
	
	// get dimensions
	int w = this->width();
	int h = this->height();

	// set a scaled pixmap to a w x h window keeping its aspect ratio 
	this->setPixmap(display.scaled(w,h,Qt::KeepAspectRatio));	
}
