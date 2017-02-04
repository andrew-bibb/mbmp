/**************************** artwidget.cpp **************************

Code to manage the widget used to pxm_display album art. 

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
# include <QPen>

# include "./code/artwidget/artwidget.h"	


// Constructor
ArtWidget::ArtWidget(QWidget* parent) : QLabel(parent)
{	
	// data members
	pxm_albumart = QPixmap();
	title = QString();
	artist = QString();
	b_showpopup = false;
		
	setAlignment(Qt::AlignCenter);
		
}


///////////////////// Public Functions /////////////////////////////
//
// Function to receive and process information to be pxm_displayed
void ArtWidget::setInfo(const QPixmap* src, const QString& t, const QString& a, const int& dur)
{
	// initialize values
	pxm_albumart = QPixmap();
	title = QString();
	artist = QString();
	b_showpopup = true;

	// save data we need
	if (src != NULL) {
		pxm_albumart = QPixmap(*src);
		title = t;
		artist = a;
	}
	
	// set a timer to turn off the popup
	QTimer::singleShot(dur * 1000, this, SLOT(turnOffPopup()) );
		
	// make pxm_display based on new information	
	makeDisplay();	
	
	return;
}

///////////////////// Public Slots /////////////////////////////
//
// Slot to turn off the popup.  Called from singleShot timer in setInfo()
void ArtWidget::turnOffPopup()
{
	b_showpopup = false;
	makeDisplay();
	
	return;
}

///////////////////// Protected Functions /////////////////////////////
//
// Have pixmap fill this label on resize
void ArtWidget::resizeEvent(QResizeEvent* e)
{
	(void) e;

	makeDisplay();
	
	return;
}


///////////////////// Private Functions /////////////////////////////
//
// Slot to make an overlaid pxm_display
void ArtWidget::makeDisplay()
{
  //  constants
  const int titlepoint = 12; // minimum height we want to display
  const int artistpoint = 10; // same
  const QString fontfamily = "Helvetica"; 
  const float percentwidth = 0.75;	// maximum width of the osd popup
	const float percentheight = 0.25;	// maximum height of the osd popup
  
	// save and set variables
	int osdheight = -1;
	int osdwidth = -1;
	
	// start building the display
	this->clear();
	QPixmap pxm_display = QPixmap(this->width(), this->height() );
	pxm_display.fill(col_background);
	
	if (! pxm_albumart.isNull() ) {
		QPainter p(&pxm_display); 
		p.setPen(Qt::black);
		p.setBrush(QBrush(col_osd, Qt::SolidPattern) );
		
		// draw artwork
		QPixmap artwork = pxm_albumart.scaled(this->width(), this->height(), Qt::KeepAspectRatio);
		p.drawPixmap( ((this->width() - artwork.width())/ 2), ((this->height() - artwork.height()) / 2), artwork);	
	
		//determine the minimum size of the osd box we want 
		QFont displayfont = QFont(fontfamily);
		displayfont.setPointSize(titlepoint);
		QFontMetrics fontmetrics = QFontMetrics(displayfont);
		osdheight = fontmetrics.height();
		osdwidth = fontmetrics.width(title);
		displayfont.setPointSize(artistpoint);
		fontmetrics = QFontMetrics(displayfont);
		osdheight += fontmetrics.height();
		if (fontmetrics.width(artist) > osdwidth ) osdwidth = fontmetrics.width(artist);
	
		// if b_showpopup and if there is enough room to display our minimum size osd box
		if (b_showpopup && osdheight <= this->height() * percentheight  && osdwidth <= this->width() * percentwidth) {	
			osdheight = this->height() * percentheight;
			displayfont.setPixelSize(osdheight * 0.45);
			fontmetrics = QFontMetrics(displayfont);
			osdwidth = fontmetrics.width(title);
			displayfont.setPixelSize(osdheight * 0.40);
			fontmetrics = QFontMetrics(displayfont);
			if (fontmetrics.width(artist) > osdwidth ) osdwidth = fontmetrics.width(artist);
			
			// paint the overlay onto the pxm_display	
			p.drawRoundedRect((this->width() - osdwidth) / 2.0, (this->height() - osdheight) / 2.0, osdwidth, osdheight, 10.0, 10.0);		
			displayfont.setPixelSize(osdheight * 0.45);
			p.drawText( ((this->width() - osdwidth) / 2), ((this->height() - osdheight) / 2), osdwidth, 0.45 * osdheight, Qt::AlignCenter, title);
			displayfont.setPixelSize(osdheight * 0.40);
			p.drawText( ((this->width() - osdwidth) / 2), ((this->height() - osdheight) / 2 + osdheight * 0.55), osdwidth, 0.40 * osdheight, Qt::AlignCenter, artist); 	
		}	// if there is enough room and b_showpopup is true
		
		p.end(); 	
	}	// if pxm_albumart not null

	// set pixmap on this 
	this->setPixmap(pxm_display);	
	
	return;
}
	
