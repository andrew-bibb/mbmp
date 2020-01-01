/**************************** artwidget.cpp **************************

Code to manage the widget used to display album art. This includes 
creating and displaying an on screen display popup with song title
and aritst at the beginning of each track.

Copyright (C) 2017-2020
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
	
	// setup the popup timer
	timer = new QTimer(this);
	connect (timer, SIGNAL(timeout()), this, SLOT(turnOffPopup()));
		
	// probably not necessary	
	setAlignment(Qt::AlignCenter);	
}


///////////////////// Public Functions /////////////////////////////
//
// Function to receive and process information to be pxm_displayed
void ArtWidget::setInfo(const QPixmap* src, const QString& t, const QString& a, const int& dur)
{
	// save data we need
	if (src == NULL) 
		pxm_albumart = QPixmap();
	else	
		pxm_albumart = QPixmap(*src);
	title = t;
	artist = a;
	
	// start the timer to control turning off the popup
	timer->stop();
	b_showpopup = true;
	timer->start(dur * 1000);
		
	// make pxm_display based on new information	
	makeDisplay();	
	
	return;
}

///////////////////// Public Slots /////////////////////////////
//
// Slot to turn off the popup.  Called from timer configured in setInfo()
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
  const float osd_pct_width = 0.75;	// maximum width (%) of the osd popup
	const float osd_pct_height = 0.25;	// maximum height (%) of the osd popup
	// following two constants need to sum to something less than 1.0 (to leave room for padding)
	// this is not checked by the program.
	const float title_pct_hgt = 0.40;	// how much of the osd box height belongs to the title
	const float artist_pct_hgt = 0.30; // same for the artist string 
  
	// define variables
	int osdheight = -1;
	int osdwidth = -1;
	
	// start building the display
	this->clear();
	QPixmap pxm_display = QPixmap(this->width(), this->height() );
	pxm_display.fill(col_background);
	
	// setup the painter
	QPainter p(&pxm_display); 
	p.setPen(Qt::black);
	p.setBrush(QBrush(col_osd, Qt::SolidPattern) );
		
	// draw artwork
	if (pxm_albumart.isNull() ) 
		p.drawTiledPixmap(0, 0, this->width(), this->height(), QPixmap(":/images/images/128x128/mbmp.png"));
	else {
		QPixmap artwork = pxm_albumart.scaled(this->width(), this->height(), Qt::KeepAspectRatio);
		p.drawPixmap( ((this->width() - artwork.width())/ 2), ((this->height() - artwork.height()) / 2), artwork);	
	}

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

	// make osd overlay assuming everything is good to go
	if (b_showpopup && osdheight <= this->height() * osd_pct_height  && osdwidth <= this->width() * osd_pct_width && (! artist.isEmpty() || ! title.isEmpty()) ) {	
		osdheight = this->height() * osd_pct_height;
		displayfont.setPixelSize(osdheight * title_pct_hgt);
		fontmetrics = QFontMetrics(displayfont);
		osdwidth = fontmetrics.width(title);
		displayfont.setPixelSize(osdheight * artist_pct_hgt);
		fontmetrics = QFontMetrics(displayfont);
		if (fontmetrics.width(artist) > osdwidth ) osdwidth = fontmetrics.width(artist);
		// now make sure the text did not drive the osd width too wide
		if (osdwidth > this->width() * osd_pct_width) {
			osdheight *=  (this->width() * osd_pct_width) / osdwidth;
			osdwidth = (this->width() * osd_pct_width);
		}
		
		// paint the overlay onto the pxm_display	
		float pad = osdheight * (1.0 - title_pct_hgt - artist_pct_hgt) / 3.0;
		osdwidth += 2.0 * pad;
		p.drawRoundedRect((this->width() - osdwidth) / 2.0, (this->height() - osdheight) / 2.0, osdwidth, osdheight, osdheight * 0.1, osdheight * 0.1);		
		displayfont.setPixelSize(osdheight * title_pct_hgt);
		p.setFont(displayfont);
		p.drawText( ((this->width() - osdwidth) / 2), ((this->height() - osdheight) / 2), osdwidth, (osdheight * title_pct_hgt) + 2.0 * pad , Qt::AlignCenter, title);
		displayfont.setPixelSize(osdheight * artist_pct_hgt);
		p.setFont(displayfont);
		p.drawText( ((this->width() - osdwidth) / 2), ((this->height() - osdheight) / 2 + pad + (osdheight * title_pct_hgt)), osdwidth, ((osdheight * artist_pct_hgt) + 2.0 * pad), Qt::AlignCenter, artist); 	
	}	// if there is enough room and b_showpopup is true
	
	p.end(); 	


	// set pixmap on this 
	this->setPixmap(pxm_display);	
	
	return;
}
	
