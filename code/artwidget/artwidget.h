/**************************** artwidget.h ****************************



Copyright (C) 2017-2019
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

# ifndef ARTWIDGET_H
# define ARTWIDGET_H

# include <QLabel>
# include <QColor>
# include <QTimer>

class ArtWidget : public QLabel 
{	
  Q_OBJECT

  public:
		ArtWidget (QWidget*);
		
		void setInfo(const QPixmap*, const QString& = QString(), const QString& = QString(), const int& = 5);
	
	public Q_SLOTS:
		void turnOffPopup();
			
	protected:
		void resizeEvent(QResizeEvent*);
		
	private:
		// members
		QPixmap pxm_albumart;
		QString artist;
		QString title;
		bool b_showpopup;
		QTimer* timer;
		// values below may become user modifiable eventually (along with duration in setInfo)
		const QColor col_background = QColor("ivory");
		const QColor col_osd = QColor("wheat");
		const QString fontfamily = "Helvetica";
	
	// functions
	void makeDisplay();
};

#endif
