/***************************************************************************
* Copyright (C) 2015-2016 Alexander V. Popov.
* 
* This file is part of Tightly Associated Solvent Shell Extractor (TASSE) 
* source code.
* 
* TASSE is free software; you can redistribute it and/or modify it under 
* the terms of the GNU General Public License as published by the Free 
* Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
* 
* TASSE is distributed in the hope that it will be useful, but WITHOUT 
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
* for more details.
* 
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
***************************************************************************/
#ifndef TASSE_WINDOW_H
#define TASSE_WINDOW_H

#include "traits/datatypes.h"
#include <QtGui/QMainWindow>
#include <QtGui/QDoubleValidator>
#include <QtGui/QLabel>
#include <QtGui/QToolButton>

class QAction;
class QTextEdit;
class QGLWidget;
class QGridLayout;
class QLineEdit;
class QProgressBar;
class QCloseEvent;

class QCustomLabel : public QLabel
{
	Q_OBJECT
public:
	explicit QCustomLabel( QWidget *parent = 0, Qt::WindowFlags f = 0 ) : QLabel( parent, f ) {}
public slots:
	void setReal( real value ) { setText( QString( "%1" ).arg( value, 4, 'f', 2 ) ); }
	void setNumIn100( int num ) { setReal( num / 100.0 ); }
};

class QCustomToolButton : public QToolButton
{
	Q_OBJECT
public:
	explicit QCustomToolButton( int type, QWidget *parent = 0 ) : QToolButton( parent ), lineEdit_( nullptr ), type_( type ) {}
	void setLineEdit( QLineEdit *control ) { lineEdit_ = control; }
	QLineEdit *getLineEdit() const { return lineEdit_; }
	int getType() const { return type_; }
private:
	QLineEdit *lineEdit_;
	int type_;
};

class QCustomDoubleValidator : public QDoubleValidator
{
public:
    QCustomDoubleValidator( double bottom, double top, int decimals, QObject *parent = 0 )
		: QDoubleValidator( bottom, top, decimals, parent) {
		setNotation( QDoubleValidator::StandardNotation );
	}
    QValidator::State validate( QString &input, int &pos ) const {
        const QValidator::State origState = QDoubleValidator::validate( input, pos );
        if ( ( origState == QValidator::Intermediate ) && ( input.toDouble() > top() ) )
            return QValidator::Invalid;
        else
            return origState;
    }
};

class CMainWindow : public QMainWindow
{
	Q_OBJECT
public:
	virtual ~CMainWindow();
	static CMainWindow &Instance() {
		static std::unique_ptr<CMainWindow> _pInstance;
		if ( !_pInstance.get() )		
			_pInstance.reset( new CMainWindow() );
		return *_pInstance;
	}

	void initialize();
	void consolePuts( const char *str, int color );
	bool onUpdateProgressBar( float value );
	virtual void closeEvent( QCloseEvent *ev );

private:
	CMainWindow();
	CMainWindow( const CMainWindow &other );
	CMainWindow& operator = ( const CMainWindow& other );

	void queryControls( const struct LayoutControlDesc *controlPtr, size_t controlCount );
	void createControls( const struct LayoutControlDesc *controlPtr, size_t controlCount, QGridLayout *layout );
	void createConsolePanel();
	void createCanvas();
	void createLayout();
	void createStatusBar();
	void createMenuBar();
	void createToolBar();

	void checkNatures();
	void startThreads();
	void stopThreads();

private slots:
	void onUpdateStatusBar( const QString &msg );
	void onBrowseForFile();
	void onCmdFileExit();
	void onCmdHelpTopics();
	void onCmdHelpAbout();
	void onCmdRunStart();
	void onCmdRunStop();

private:
	QLabel				*statusText_;
	QTextEdit			*consolePanel_;
	QGLWidget			*canvas_;
	QProgressBar		*progress_;
	QVector<QAction*>	actionList_;
	QString				lastFileName_;
	bool				running_;
	bool				threaded_;
};

#endif //TASSE_WINDOW_H
