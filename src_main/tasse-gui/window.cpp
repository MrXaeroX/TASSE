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
#include <tasse.h>
#include <nature.h>
#include <hbonds.h>
#include <topology.h>
#include "value_for_control.h"
#include "window.h"

#include <QtCore/QUrl>
#include <QtGui/QApplication>
#include <QtGui/QCheckBox>
#include <QtGui/QCloseEvent>
#include <QtGui/QDesktopServices>
#include <QtGui/QDesktopWidget>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QFileDialog>
#include <QtGui/QLineEdit>
#include <QtGui/QMenuBar>
#include <QtGui/QMessageBox>
#include <QtGui/QProgressBar>
#include <QtGui/QSlider>
#include <QtGui/QSplitter>
#include <QtGui/QStatusBar>
#include <QtGui/QTabWidget>
#include <QtGui/QToolBar>
#include <QtGui/QTextEdit>
#include <QtOpenGL/QGLWidget>

#define ITEMDESC_MENU				BIT( 0u )
#define ITEMDESC_TOOLBAR			BIT( 1u )
#define ITEMDESC_TOGGLE				BIT( 2u )
#define ITEMDESC_GROUP				BIT( 3u )
#define ITEMDESC_DEFAULT			BIT( 4u )

struct MenuActionDesc {
	const char	*menuName;
	const char	*itemName;
	const char	*itemTitle;
	const char	*itemImage;
	const char	*shortcut;
	const char	*hint;
	const char	*slotName;
	const QKeySequence::StandardKey sequenceKey;
	uint32		flags;
	uint32		toolbarOrder;
};

static struct MenuActionDesc s_Actions[] = 
{
	{ "&File",	"FileExit",	"E&xit",		nullptr, nullptr, "Quit the application", SLOT( onCmdFileExit() ), QKeySequence::Quit, ITEMDESC_MENU, 0 },
	{ "&Run",	"RunStart",	"Start computation", nullptr, "F9", "Run computations", SLOT(onCmdRunStart()), QKeySequence::UnknownKey, ITEMDESC_MENU, 0 },
	{ "&Run",	"RunStop",	"Stop computation", nullptr, "Pause", "Stop running computations", SLOT(onCmdRunStop()), QKeySequence::UnknownKey, ITEMDESC_MENU, 0 },
	{ "&Help",  "HelpTopics", "&Reference Manual", nullptr, "F1", "Display " PROGRAM_LARGE_NAME " reference manual", SLOT(onCmdHelpTopics()), QKeySequence::UnknownKey, ITEMDESC_MENU, 0 },
	{ "&Help",	"HelpAbout", "&About...", nullptr,	nullptr, "Display information about " PROGRAM_LARGE_NAME, SLOT( onCmdHelpAbout() ), QKeySequence::UnknownKey, ITEMDESC_MENU, 0 },
};

static struct LayoutControlDesc s_BasicControls[] = {
	DEFINE_CONTROL( "Input Topology", CTRL_FILE_TRJ, CVAR_CHARS, 0, 0, gpGlobals->input_topology ),
	DEFINE_CONTROL( "Input Coordinates", CTRL_FILE_TRJ, CVAR_CHARS, 0, 0, gpGlobals->input_coordinate ),
	DEFINE_CONTROL( "Input Trajectory", CTRL_FILE_TRJ, CVAR_CHARS, 0, 0, gpGlobals->input_trajectory ),
	DEFINE_CONTROL( "Output PDB File", CTRL_FILE_PDB, CVAR_CHARS, 0, 0, gpGlobals->output_pdbname ),
	DEFINE_CONTROL( "Output Information File", CTRL_FILE_ANY, CVAR_CHARS, 0, 0, gpGlobals->output_tuples ),
	DEFINE_CONTROL( "Solvent Residue Name", CTRL_INPUT, CVAR_CHARS, 0, 0, &gpGlobals->solvent_title ),
	DEFINE_CONTROL( "Initial Snapshot", CTRL_INPUT, CVAR_SIZET, 0, 0, &gpGlobals->first_snap ),
	DEFINE_CONTROL( "Minimum Occurence Cutoff", CTRL_SLIDER, CVAR_REAL, 0, 1, &gpGlobals->occurence_cutoff ),
	DEFINE_CONTROL( "Electrostatics Weight Factor", CTRL_SLIDER, CVAR_REAL, 0, 1, &gpGlobals->electrostatic_coeff ),
	DEFINE_CONTROL("Hydrogen Bonds Weight Factor", CTRL_SLIDER, CVAR_REAL, 0, 1, &gpGlobals->hbond_126_coeff )
};

static LayoutControlDesc s_AdvancedControls[] = 
{
	DEFINE_CONTROL( "Relative Permittivity (Dielectric Constant)", CTRL_INPUT, CVAR_REAL, 0, 0, &gpGlobals->dielectric_const ),
	DEFINE_CONTROL( "Electrostatics Cutoff Radius (A)", CTRL_INPUT, CVAR_REAL, 2, 10000, &gpGlobals->electrostatic_radius ),
	DEFINE_CONTROL( "Minimum Hydrogen Bond Energy (kcal/mol)", CTRL_SLIDER, CVAR_REAL, 0, 5, &gpGlobals->hbond_cutoff_energy ),
	DEFINE_CONTROL( "Maximum Hydrogen Bond Length (A)", CTRL_SLIDER, CVAR_REAL, 2, 10, &gpGlobals->hbond_max_length ),
	DEFINE_CONTROL( "VdW Overlapping Tolerance", CTRL_SLIDER, CVAR_REAL, 0, 1, &gpGlobals->vdw_tolerance ),
	DEFINE_CONTROL( "Group Hydrogen Bonds Formed by the same Donor/Acceptor", CTRL_CHECKBOX, CVAR_BOOL, 0, 0, &gpGlobals->group_bonds ),
	DEFINE_CONTROL( "Read Charges from the Input (not applicable to PDB)", CTRL_CHECKBOX, CVAR_BOOL, 0, 0, &gpGlobals->read_charges ),
#if MAX_THREADS > 1
	DEFINE_CONTROL( "Number of Threads", CTRL_SLIDER, CVAR_INT, 1, MAX_THREADS, &gpGlobals->thread_count ),
#endif
	DEFINE_CONTROL( "Yield Resources to Other Applications", CTRL_CHECKBOX, CVAR_BOOL, 0, 0, &gpGlobals->low_prio ),
	DEFINE_CONTROL( "Verbose Mode", CTRL_CHECKBOX, CVAR_BOOL, 0, 0, &gpGlobals->verbose )
};

//////////////////////////////////////////////////////////////////////////

void ThreadUpdateGUI( float progress )
{
	if ( CMainWindow::Instance().onUpdateProgressBar( progress ) || QApplication::hasPendingEvents() ) {
		ThreadLock();
		QApplication::processEvents();
		ThreadUnlock();
	}
}

#define DEFAULT_STATUS_TIP	QObject::tr( "For Help, press F1" )

CMainWindow :: CMainWindow() : running_( false ), threaded_( false )
{
	setObjectName( PROGRAM_SHORT_NAME "_MainWindow" );
	setWindowTitle( tr( PROGRAM_LARGE_NAME ) );
	setWindowIcon( QIcon( ":/qtasse48.png" ) );

	createLayout();
	createStatusBar();
	createMenuBar();
	createToolBar();

	QDesktopWidget dw;
	setGeometry( dw.x() + ( dw.width() - 560 ) / 2,
				 dw.y() + ( dw.height() - 560 ) / 2,
				 560, 560 );

	// initialize timing
	utils->FloatMilliseconds();
}

CMainWindow :: ~CMainWindow()
{
	// clear H-bond information
	hbonds->Clear();

	// clear topology
	topology->Clear();

	// unload config files
	configs->Unload();

	// cleanup threads
	ThreadCleanup();
}

void CMainWindow :: closeEvent( QCloseEvent *ev )
{
	if ( running_ && !threaded_ ) {
		ev->ignore();
		return;
	}

	// make sure threads are stopped
	stopThreads();
	ev->accept();
}

void CMainWindow :: initialize()
{
	console->Print( ( tr( "<b>%1 %2 initialized</b> (%3 %4 build %5 %6)\n" ).arg( PROGRAM_LARGE_NAME, PROGRAM_VERSION, PROGRAM_EXE_TYPE, PROGRAM_ARCH, __DATE__, __TIME__ ) ).toAscii().data() );

	// scan and load config files
	configs->Load();

	// initialize topology libraries
	topology->Initialize();
	console->Print( "Topology library initialized.\n" );

	// show the main window
	show();
	raise();
	update();
}

void CMainWindow :: consolePuts( const char *str, int color )
{
	assert( consolePanel_ != nullptr );
	if ( !consolePanel_ )
		return;
	QString outputStr( str );
	outputStr.replace( " ", "&nbsp;" );
	outputStr.replace( "\n", "<br>" );
	if ( color != 0 )
		outputStr = QString( "<font color=\"#%1\">%2</font>" ).arg( color, 6, 16, QChar( '0' ) ).arg( outputStr );
	QTextCursor cur = consolePanel_->textCursor();
	cur.movePosition( QTextCursor::End );
	cur.insertHtml( outputStr );
	consolePanel_->setTextCursor( cur );
}

void CMainWindow :: createConsolePanel()
{
	QPalette palette;
	palette.setBrush(QPalette::Active, QPalette::Base, this->palette().brush(QPalette::Active, QPalette::Window));
	palette.setBrush(QPalette::Inactive, QPalette::Base, this->palette().brush(QPalette::Inactive, QPalette::Window));
	palette.setBrush(QPalette::Disabled, QPalette::Base, this->palette().brush(QPalette::Disabled, QPalette::Window));

	consolePanel_ = new QTextEdit( this );
	consolePanel_->setObjectName( "ConsoleText" );
	consolePanel_->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	consolePanel_->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	consolePanel_->setUndoRedoEnabled( false );
	consolePanel_->setReadOnly( true );
	consolePanel_->setTextInteractionFlags( Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse );
	consolePanel_->setText( "" );
	consolePanel_->setPalette( palette );
	consolePanel_->setAutoFillBackground( true );
	consolePanel_->setMinimumHeight( 80 );
}

void CMainWindow :: queryControls( const struct LayoutControlDesc *controlPtr, size_t controlCount )
{
	// iterate through all the controls
	for ( size_t i = 0; i < controlCount; ++i, ++controlPtr ) {
		switch ( controlPtr->ctrlType ) {
		case CTRL_CHECKBOX:
			{
				QCheckBox *ctrl = qobject_cast<QCheckBox*>( controlPtr->w_ptr );
				ValueForControl<bool>( controlPtr ).assign( ctrl->isChecked() );

			}
			break;
		case CTRL_INPUT:
		case CTRL_FILE_PDB:
		case CTRL_FILE_TRJ:
		case CTRL_FILE_ANY:
			{
				QLineEdit *ctrl = qobject_cast<QLineEdit*>( controlPtr->w_ptr );
				ValueForControl<QString>( controlPtr ).assign( ctrl->text() );
			}
			break;
		case CTRL_SLIDER:
			{
				QSlider *ctrl = qobject_cast<QSlider*>( controlPtr->w_ptr );
				if ( controlPtr->cvarType == CVAR_REAL )
					ValueForControl<real>( controlPtr ).assign( real( 0.01 ) * ctrl->value() );
				else
					ValueForControl<int>( controlPtr ).assign( ctrl->value() );
			}
			break;
		default:
			break;
		}
	}
}

void CMainWindow :: createControls( const struct LayoutControlDesc *controlPtr, size_t controlCount, QGridLayout *layout )
{
	int currentRow = 0;

	// iterate through all the controls
	for ( size_t i = 0; i < controlCount; ++i, ++controlPtr ) {
		switch ( controlPtr->ctrlType ) {
		case CTRL_CHECKBOX:
			{
				QCheckBox *ctrl = new QCheckBox( this );
				if ( controlPtr->title ) ctrl->setText( controlPtr->title );
				ctrl->setChecked( ValueForControl<bool>( controlPtr ) );
				controlPtr->w_ptr = ctrl;
				layout->addWidget( ctrl, currentRow, 0, 1, 3 );
			}
			break;
		case CTRL_INPUT:
			{
				QLabel *lbl = new QLabel( this );
				if ( controlPtr->title ) lbl->setText( QString( "%1:" ).arg( controlPtr->title ) );
				layout->addWidget( lbl, currentRow, 0 );
				QLineEdit *ctrl = new QLineEdit( this );
				ctrl->setFixedHeight( 20 );
				ctrl->setText( ValueForControl<QString>( controlPtr ) );
				if ( controlPtr->minValue || controlPtr->maxValue )
					ctrl->setValidator( new QCustomDoubleValidator( controlPtr->minValue, controlPtr->maxValue, 100, ctrl ) );
				controlPtr->w_ptr = ctrl;
				layout->addWidget( ctrl, currentRow, 1 );
			}
			break;
		case CTRL_SLIDER:
			{
				QLabel *lbl = new QLabel( this );
				if ( controlPtr->title ) lbl->setText( QString( "%1:" ).arg( controlPtr->title ) );
				layout->addWidget( lbl, currentRow, 0 );
				QSlider *ctrl = new QSlider( this );
				ctrl->setOrientation( Qt::Horizontal );
				ctrl->setInvertedAppearance( false );
				ctrl->setInvertedControls( false );
				ctrl->setTickPosition( QSlider::NoTicks );
				ctrl->setSingleStep( 1 );
				ctrl->setPageStep( 10 );
				if ( controlPtr->cvarType == CVAR_REAL ) {
					ctrl->setMinimum( controlPtr->minValue * 100 );
					ctrl->setMaximum( controlPtr->maxValue * 100 );
				} else {
					ctrl->setMinimum( controlPtr->minValue );
					ctrl->setMaximum( controlPtr->maxValue );
				}
				controlPtr->w_ptr = ctrl;
				layout->addWidget( ctrl, currentRow, 1 );
				QCustomLabel *clbl = new QCustomLabel( this );
				if ( controlPtr->cvarType == CVAR_REAL ) {
					clbl->setReal( controlPtr->minValue );
					clbl->setMinimumWidth( QFontMetrics( clbl->font() ).width( "99.00" ) );
					connect( ctrl, SIGNAL( valueChanged(int) ), clbl, SLOT( setNumIn100(int) ) );
					ctrl->setValue( ValueForControl<real>( controlPtr ) * 100 );
				} else {
					clbl->setNum( controlPtr->minValue );
					clbl->setMinimumWidth( QFontMetrics( clbl->font() ).width( "100" ) );
					connect( ctrl, SIGNAL( valueChanged(int) ), clbl, SLOT( setNum(int) ) );
					ctrl->setValue( ValueForControl<int>( controlPtr ) );
				}
				layout->addWidget( clbl, currentRow, 2 );
			}
			break;
		case CTRL_FILE_PDB:
		case CTRL_FILE_TRJ:
		case CTRL_FILE_ANY:
			{
				QLabel *lbl = new QLabel( this );
				if ( controlPtr->title ) lbl->setText( QString( "%1:" ).arg( controlPtr->title ) );
				layout->addWidget( lbl, currentRow, 0 );
				QLineEdit *ctrl = new QLineEdit( this );
				ctrl->setFixedHeight( 20 );
				ctrl->setText( ValueForControl<QString>( controlPtr ) );
				controlPtr->w_ptr = ctrl;
				layout->addWidget( ctrl, currentRow, 1 );
				QCustomToolButton *bttn = new QCustomToolButton( controlPtr->ctrlType, this );
				bttn->setText( "..." );
				bttn->setLineEdit( ctrl );
				bttn->setFixedSize( 20, 20 );
				connect( bttn, SIGNAL(clicked(bool)), this, SLOT( onBrowseForFile() ) );
				layout->addWidget( bttn, currentRow, 2 );
			}
			break;
		default:
			break;
		}
		++currentRow;
	}

	QSpacerItem *verticalSpacer = new QSpacerItem( 0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding );
	layout->addItem( verticalSpacer, currentRow, 0, 1, 3 );
}

void CMainWindow :: createCanvas()
{
//	QGLFormat f;
//	f.setDoubleBuffer(false);
//	QGLFormat::setDefaultFormat(f);

	canvas_ = new QGLWidget( this );
	//!TODO
}

void CMainWindow :: createLayout()
{
	createConsolePanel();
	createCanvas();

	QWidget *central = new QWidget( this );
	central->setStatusTip( DEFAULT_STATUS_TIP );

	QGridLayout *gridLayout1 = new QGridLayout();
	gridLayout1->setContentsMargins( 10, 10, 10, 10 );
	createControls( s_BasicControls, sizeof(s_BasicControls)/sizeof(s_BasicControls[0]), gridLayout1 );
	QGridLayout *gridLayout2 = new QGridLayout();
	gridLayout2->setContentsMargins( 10, 10, 10, 10 );
	createControls( s_AdvancedControls, sizeof(s_AdvancedControls)/sizeof(s_AdvancedControls[0]), gridLayout2 );

	progress_ = new QProgressBar( this );
	progress_->setMinimum( 0 );
	progress_->setMaximum( 500 );
	progress_->setTextVisible( false );
	progress_->setFixedHeight( 10 );
	progress_->setEnabled( false );
	progress_->setVisible( false );

	QTabWidget *tabs = new QTabWidget( this );
	tabs->setObjectName( "Tabs" );
	QWidget *tab0 = new QWidget();
	tab0->setObjectName( "tab0" );
	tab0->setMinimumSize( 350, 200 );
	tab0->setLayout( gridLayout1 );
	QWidget *tab1 = new QWidget();
	tab1->setObjectName( "tab1" );
	tab1->setMinimumSize( 350, 200 );
	tab1->setLayout( gridLayout2 );
	QWidget *tab2 = new QWidget();
	tab2->setObjectName( "tab2" );
	tab2->setMinimumSize( 350, 200 );
	QVBoxLayout *tab2layout = new QVBoxLayout();
	tab2layout->setContentsMargins( 4, 4, 4, 4 );
	tab2layout->addWidget( canvas_, 0 );
	tab2->setLayout( tab2layout );
	tabs->addTab( tab0, "Basic Settings" );
	tabs->addTab( tab1, "Advanced Settings" );
	tabs->addTab( tab2, "Renderer" );

	QSplitter *splitter = new QSplitter( Qt::Vertical, this );
	splitter->setObjectName( "Splitter" );
	splitter->setChildrenCollapsible( false );
	splitter->addWidget( tabs );
	splitter->addWidget( progress_ );
	splitter->addWidget( consolePanel_ );
	
	QVBoxLayout *layout = new QVBoxLayout();
	layout->setContentsMargins( 1, 1, 1, 1 );
	layout->addWidget( splitter, 0 );
	central->setLayout( layout );
	setCentralWidget( central );
}

void CMainWindow :: createStatusBar()
{
	statusText_ = new QLabel( this );
	statusText_->setAlignment( Qt::AlignLeft );
	statusBar()->addPermanentWidget( statusText_, true );
	connect( statusBar(), SIGNAL(messageChanged(const QString&)), this, SLOT(onUpdateStatusBar(const QString&)));	
}

void CMainWindow :: createMenuBar()
{
	const size_t actionCount = sizeof(s_Actions) / sizeof(s_Actions[0]);
	const struct MenuActionDesc *actionDesc = &s_Actions[0];
	QMenu *pCurrentMenu = nullptr;
	QActionGroup *pCurrentGroup = nullptr;

	// iterate through all the actions
	for ( size_t i = 0; i < actionCount; ++i, ++actionDesc ) {
		// check if action is allowed in the menu
		if ( !( actionDesc->flags & ITEMDESC_MENU ) )
			continue;

		// action must contain a valid menu name
		assert( actionDesc->menuName != nullptr );

		// restart the menu, if needed
		if ( !pCurrentMenu || pCurrentMenu->title().compare( actionDesc->menuName ) ) {
			pCurrentMenu = menuBar()->addMenu( actionDesc->menuName );
			pCurrentMenu->setObjectName( actionDesc->menuName );
			pCurrentGroup = nullptr;
		}

		if ( actionDesc->itemTitle ) {
			// normal item
			QAction *pAction = new QAction( actionDesc->itemTitle, pCurrentMenu );
			if ( actionDesc->itemName ) pAction->setObjectName( actionDesc->itemName );
			if ( actionDesc->itemImage ) pAction->setIcon( QIcon( QString(":/").append( actionDesc->itemImage ).append( ".png" ) ) );
			if ( actionDesc->shortcut ) pAction->setShortcut( tr( actionDesc->shortcut ) );
			else if ( actionDesc->sequenceKey != QKeySequence::UnknownKey ) pAction->setShortcuts( actionDesc->sequenceKey );
			if ( actionDesc->hint ) pAction->setStatusTip( actionDesc->hint );
			if ( actionDesc->slotName )	connect( pAction, SIGNAL(triggered()), this, actionDesc->slotName );
			if ( actionDesc->flags & ITEMDESC_GROUP ) {
				pAction->setCheckable( true );
				if ( !pCurrentGroup ) {
					pCurrentGroup = new QActionGroup( pCurrentMenu );
					pCurrentGroup->setExclusive( true );
					pAction->setChecked( true );
				}
				pAction->setActionGroup( pCurrentGroup );
			} else {
				pCurrentGroup = nullptr;
			}
			if ( actionDesc->flags & ITEMDESC_TOGGLE ) pAction->setCheckable( true );
			if ( actionDesc->flags & ITEMDESC_DEFAULT ) pAction->setChecked( true );
			actionList_.push_back( pAction );
			pCurrentMenu->addAction( pAction );
		} else {
			// separator
			pCurrentMenu->addSeparator();
		}
	}
}

void CMainWindow :: createToolBar()
{
	const size_t actionCount = sizeof(s_Actions) / sizeof(s_Actions[0]);
	const struct MenuActionDesc *actionDesc = &s_Actions[0];
	QVector<const struct MenuActionDesc*> localActionDesc;

	// iterate through all the actions
	for ( size_t i = 0; i < actionCount; ++i, ++actionDesc ) {
		// check if action is allowed in the menu
		if ( !( actionDesc->flags & ITEMDESC_TOOLBAR ) )
			continue;
		localActionDesc << actionDesc;
	}

	const int localActionCount = localActionDesc.size();
	if ( !localActionCount )
		return;

	qSort( localActionDesc.begin(), localActionDesc.end(), 
		[]( const struct MenuActionDesc *a1, const struct MenuActionDesc *a2 ) { return ( a1->toolbarOrder < a2->toolbarOrder ); } );

	// have a valid action, create the toolbar
	QToolBar *pToolbar = addToolBar( tr( "Toolbar" ) );
	pToolbar->setObjectName( "Toolbar" );
	pToolbar->setIconSize( QSize( 16, 16 ) );

	for ( int i = 0; i < localActionCount; ++i ) {
		actionDesc = localActionDesc.at( i );
		if ( actionDesc->itemTitle ) {
			// normal item
			QAction *pRef = nullptr;
			auto it = std::find_if( actionList_.begin(), actionList_.end(), 
				[ actionDesc ]( const QAction *a ) { return !a->objectName().compare( actionDesc->itemName ); } );
			if ( it != actionList_.end() )
				pRef = *it;
			QAction *pAction = new QAction( actionDesc->itemTitle, pToolbar );
			if ( actionDesc->itemName ) pAction->setObjectName( actionDesc->itemName );
			if ( actionDesc->itemImage ) pAction->setIcon( QIcon( QString(":/").append( actionDesc->itemImage ).append( ".png" ) ) );
			if ( !pRef ) {
				if ( actionDesc->shortcut ) pAction->setShortcut( tr( actionDesc->shortcut ) );
				else if ( actionDesc->sequenceKey != QKeySequence::UnknownKey ) pAction->setShortcuts( actionDesc->sequenceKey );
			}
			if ( actionDesc->hint ) pAction->setStatusTip( actionDesc->hint );
			if ( actionDesc->slotName ) connect( pAction, SIGNAL(triggered()), this, actionDesc->slotName );
			if ( actionDesc->flags & ITEMDESC_TOGGLE ) pAction->setCheckable( true );
			if ( actionDesc->flags & ITEMDESC_DEFAULT ) pAction->setChecked( true );
			if ( pRef ) {
				connect( pAction, SIGNAL(toggled(bool)), pRef, SLOT(setChecked(bool)) );
				connect( pRef, SIGNAL(toggled(bool)), pAction, SLOT(setChecked(bool)) );
			}
			pToolbar->addAction( pAction );
		} else {
			// separator
			pToolbar->addSeparator();
		}
	}
}

//////////////////////////////////////////////////////////////////////////

bool CMainWindow :: onUpdateProgressBar( float value )
{
	if ( !running_ )
		return false;

	uint32 oldValue = progress_->value();
	uint32 newValue = progress_->maximum() * value;

	if ( oldValue != newValue ) {
		progress_->setValue( newValue );
		return true;
	} else {
		return false;
	}
}

void CMainWindow :: onUpdateStatusBar( const QString &msg )
{
	if ( statusText_ ) {
		statusText_->setText( msg );
		statusText_->repaint();
	}
}

void CMainWindow :: onBrowseForFile()
{
	QCustomToolButton *bttn = qobject_cast<QCustomToolButton*>( QObject::sender() );
	if ( bttn ) {
		QString filter;
		if ( bttn->getType() == CTRL_FILE_PDB )
			filter = QString( "PDB Files (*.pdb)" );
		else if ( bttn->getType() == CTRL_FILE_TRJ )
			filter = QString( "Trajectory Files (*.pdb *.lst *.prmtop *.inpcrd *.mdcrd *.rst);;All Files (*.*)" );
		else
			filter = QString( "All Files (*.*)" );
		QString initial;
		if ( bttn->getLineEdit() ) 
			initial = bttn->getLineEdit()->text();
		if ( initial.isEmpty() || !QFileInfo( initial ).exists() )
			initial = lastFileName_;
		QString fileName = QFileDialog::getOpenFileName( this, QString(), initial, filter, nullptr, QFileDialog::DontResolveSymlinks );
		if ( !fileName.isEmpty() ) {
			if ( bttn->getLineEdit() )
				bttn->getLineEdit()->setText( fileName );
			lastFileName_ = fileName;
		}
	}
}

void CMainWindow :: onCmdFileExit()
{
	close();
}

void CMainWindow :: onCmdRunStart()
{
	queryControls( s_BasicControls, sizeof(s_BasicControls)/sizeof(s_BasicControls[0]) );
	queryControls( s_AdvancedControls, sizeof(s_AdvancedControls)/sizeof(s_AdvancedControls[0]) );

	gpGlobals->input_topology_nature = TYP_AUTO;
	gpGlobals->input_coordinate_nature = TYP_AUTO;
	gpGlobals->input_trajectory_nature = TYP_AUTO;

	try {
		checkNatures();
		startThreads();
	} catch ( const std::runtime_error &e ) {
		QApplication::restoreOverrideCursor();
		stopThreads();
		running_ = false;
		QMessageBox::warning( this, PROGRAM_LARGE_NAME,QString( "Error: %1\nSee the log file for the details." ).arg( e.what() ) );
	}
}

void CMainWindow :: onCmdRunStop()
{
	stopThreads();
}

void CMainWindow :: onCmdHelpTopics()
{
	QString helpFileName( QApplication::applicationDirPath().append("/").append(PROGRAM_HELP_FILENAME) );
	if ( QFileInfo( helpFileName ).exists() )
		QDesktopServices::openUrl( QUrl( helpFileName, QUrl::TolerantMode ) );
	else
		utils->Warning( "missing \"%s\"!\n", helpFileName.toLocal8Bit().data() );
}

void CMainWindow :: onCmdHelpAbout()
{
	QString progCopyright(  tr( "This program is free software; you can redistribute it and/or "
								"modify it under the terms of the GNU General Public License "
								"as published by the Free Software Foundation; either version 2 "
								"of the License, or (at your option) any later version.<br><br>"
								"This program is distributed in the hope that it will be useful, "
								"but WITHOUT ANY WARRANTY; without even the implied warranty of "
								"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
								"GNU General Public License for more details." ) );

	QString aboutMsg( tr("<h2>" PROGRAM_LARGE_NAME "</h2><h3>Version: " PROGRAM_VERSION " (" PROGRAM_ARCH ")</h3><br>Copyright &copy; 2015-2016. All rights reserved.<br><br>" ).append( progCopyright ) );
	QMessageBox::about( this, tr("About %1").arg( PROGRAM_SHORT_NAME ), aboutMsg );
}

//////////////////////////////////////////////////////////////////////////

static void process_trajectory_snapshot( uint32 threadNum, uint32 snapshotNum, const coord3_t *coords )
{
	// this function can be called concurrently and therefore must be reenterant!
	hbonds->CalcMicrosets( threadNum, snapshotNum, coords );
}

void CMainWindow :: checkNatures()
{
	if ( gpGlobals->input_topology_nature == TYP_AUTO ) {
		gpGlobals->input_topology_nature = NatureHelper()[gpGlobals->input_topology]();
		if ( gpGlobals->input_topology_nature == TYP_AUTO ) {
			utils->Warning( "failed to autodetect input topology nature, using default \"%s\" (%i)\n", NatureHelper( DEFAULT_TOPOLOGY_NATURE ).toString(), DEFAULT_TOPOLOGY_NATURE );
			gpGlobals->input_topology_nature = DEFAULT_TOPOLOGY_NATURE;
		} else {
			console->Print( "Auto-detected input topology nature: %s\n", NatureHelper( gpGlobals->input_topology_nature ).toString() );
		}
	}
	if ( gpGlobals->input_coordinate_nature == TYP_AUTO ) {
		gpGlobals->input_coordinate_nature = NatureHelper()[gpGlobals->input_coordinate]();
		if ( gpGlobals->input_coordinate_nature == TYP_AUTO ) {
			utils->Warning( "failed to autodetect input coordinate nature, using default \"%s\" (%i)\n", NatureHelper( DEFAULT_COORDINATE_NATURE ).toString(), DEFAULT_COORDINATE_NATURE );
			gpGlobals->input_coordinate_nature = DEFAULT_COORDINATE_NATURE;
		} else {
			console->Print( "Auto-detected input coordinate nature: %s\n", NatureHelper( gpGlobals->input_coordinate_nature ).toString() );
		}
	}
	if ( gpGlobals->input_trajectory_nature == TYP_AUTO ) {
		gpGlobals->input_trajectory_nature = NatureHelper()[gpGlobals->input_trajectory]();
		if ( gpGlobals->input_trajectory_nature == TYP_AUTO ) {
			utils->Warning( "failed to autodetect input trajectory nature, using default \"%s\" (%i)\n", NatureHelper( DEFAULT_TRAJECTORY_NATURE ).toString(), DEFAULT_TRAJECTORY_NATURE );
			gpGlobals->input_trajectory_nature = DEFAULT_TRAJECTORY_NATURE;
		} else {
			console->Print( "Auto-detected input trajectory nature: %s\n", NatureHelper( gpGlobals->input_trajectory_nature ).toString() );
		}
	}
}

void CMainWindow :: startThreads()
{
	if ( running_ ) {
		utils->Warning( "computation process is already running!\n" );
		return;
	}

	running_ = true;

	QApplication::setOverrideCursor( Qt::WaitCursor );

	// init threads
	ThreadSetDefault( gpGlobals->thread_count, gpGlobals->low_prio );

	// initialize H-bond information
	hbonds->Initialize();

	// load topology
	if ( !topology->Load( gpGlobals->input_topology, gpGlobals->input_topology_nature,
						  gpGlobals->input_coordinate, gpGlobals->input_coordinate_nature ) ) {
		QApplication::restoreOverrideCursor();
		running_ = false;
		return;
	}

	// setup H-bond detection
	hbonds->Setup( ThreadCount() );

	// topology and coordinates were successfully loaded
	// start processing the trajectory
	if ( gpGlobals->convert_only ) {
		// just save the PDB
		topology->Save( gpGlobals->output_pdbname );
	} else {
		// build lists of donors and acceptors
		hbonds->BuildAtomLists();
		// enable progress bar
		progress_->setVisible( true );
		progress_->setEnabled( true );
		progress_->setValue( 0 );
		// process the trajectory
		QApplication::restoreOverrideCursor();
		threaded_ = true;
		uint32 total = topology->ProcessTrajectory( gpGlobals->input_trajectory, gpGlobals->input_trajectory_nature, 
													gpGlobals->first_snap, process_trajectory_snapshot, gpGlobals->pacifier );
		// disable progress bar
		progress_->setValue( 0 );
		progress_->setEnabled( false );
		progress_->setVisible( false );
		threaded_ = false;
		if ( ThreadInterrupted() ) {
			utils->Warning( "computation process interrupted by user!\n\n" );
			QApplication::restoreOverrideCursor();
		} else {
			QApplication::setOverrideCursor( Qt::WaitCursor );
			if ( total ) {
				// build final solvent info
				hbonds->BuildFinalSolvent( total );
				// save final PDB
				topology->Save( gpGlobals->output_pdbname );
				// write output tables
				if ( *gpGlobals->output_tuples )
					hbonds->PrintFinalTuples( total, gpGlobals->output_tuples );
				// write performance counters
				hbonds->PrintPerformanceCounters();
				// final notice
				console->Print( "Task complete!\n" );
			}
		}
	}

	QApplication::restoreOverrideCursor();
	running_ = false;
}

void CMainWindow :: stopThreads()
{
	if ( threaded_ ) {
		// show user that we acked the command
		QApplication::setOverrideCursor( Qt::WaitCursor );
		console->Print( "Interrupring, please wait...\n" );

		// interrupt threads
		ThreadInterrupt();
	}

	// setup progress bar
	progress_->setValue( 0 );
	progress_->setEnabled( false );
	progress_->setVisible( false );
}
