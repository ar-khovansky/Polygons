#include "stdafx.h"
#include "MainFrm.h"

#include "resource.h"

#include <afxpriv.h>    // defines WM_SETMESSAGESTRING



#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// MainFrame

IMPLEMENT_DYNCREATE(MainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(MainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_MESSAGE(WM_SETMESSAGESTRING, OnSetMessageString)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};


// MainFrame construction/destruction

MainFrame::MainFrame()
	: statusMsgId(AFX_IDS_IDLEMESSAGE)
{
}

MainFrame::~MainFrame()
{
}



int MainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));

	// TODO: Delete these three lines if you don't want the toolbar to be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);


	return 0;
}


BOOL MainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

// MainFrame diagnostics

#ifdef _DEBUG
void MainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void MainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}
#endif //_DEBUG



/*!
 * Dynamic status message implementation
 * -------------------------------------
 *
 * We implement interface IStatusPane, through which a client communicates desired message.
 * We send WM_SETMESSAGESTRING to actually set it. Also we override the WM_SETMESSAGESTRING
 * handler, intercept all changes of the message, and substitute idle message with desired.
 */


void MainFrame::setStatusMessage(UINT id)
{
	if ( statusMsgId == id )
		return;

	statusMsgId = id;
	SendMessage(WM_SETMESSAGESTRING, id);
}

void MainFrame::resetStatusMessage()
{
	setStatusMessage(AFX_IDS_IDLEMESSAGE);
}


// MainFrame message handlers

// Called when status message is set
LRESULT MainFrame::OnSetMessageString(WPARAM wParam, LPARAM lParam)
{
	// Substitute idle message with our own
	if ( wParam == AFX_IDS_IDLEMESSAGE && lParam == 0 )
		wParam = statusMsgId;

	return CFrameWnd::OnSetMessageString(wParam, lParam);
}
