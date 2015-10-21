
// Polygons.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "Polygons.h"
#include "MainFrm.h"

#include "PolygonsDoc.h"
#include "PolygonsView.h"

#include <gdiplus.h>


using namespace Gdiplus;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif



// The one and only PolygonsApp object
PolygonsApp theApp;



// PolygonsApp

BEGIN_MESSAGE_MAP(PolygonsApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, &PolygonsApp::OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, &CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinApp::OnFilePrintSetup)
	ON_COMMAND(ID_FILE_OPENFROMDB, &PolygonsApp::OnFileOpenFromDB)
END_MESSAGE_MAP()


// PolygonsApp construction

PolygonsApp::PolygonsApp()
	: gdipToken(NULL)
	, db(nullptr)
{
	// TODO: replace application ID string below with unique ID string; recommended
	// format for string is CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("Polygons.1_0"));

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// PolygonsApp initialization

BOOL PolygonsApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	EnableTaskbarInteraction(FALSE);

	// AfxInitRichEdit2() is required to use RichEdit control	
	// AfxInitRichEdit2();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)


	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(PolygonsDoc),
		RUNTIME_CLASS(MainFrame),       // main SDI frame window
		RUNTIME_CLASS(PolygonsView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);


	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Enable DDE Execute open
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);


	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	// call DragAcceptFiles only if there's a suffix
	//  In an SDI app, this should occur after ProcessShellCommand
	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();
	
	GdiplusStartupInput GSI;
	if ( GdiplusStartup(&gdipToken, &GSI, NULL) != Gdiplus::Ok ) {
		AfxMessageBox(_T("GDI+ initialization failed"));
		return FALSE;
	}
	
	return TRUE;
}



int PolygonsApp::ExitInstance()
{
	if ( gdipToken )
		GdiplusShutdown(gdipToken);

	return CWinApp::ExitInstance();
}


// PolygonsApp message handlers


// Open from DB works as following.
// - Application handles the command.
// - It asks user for data source and opens the DB.
// - It sets its member accessed by getDatabase() to point to DB.
// - It creates new document as if File->New was hit.
// - Document in OnNewDocument() calls app->getDatabase() and gets pointer to DB.
// - From the fact that pointer is not null, Document realizes that it is not normal new
//   document initialization, but it is asked to load from DB.
// - Document loads from DB.
// - Application sets member accessed by getDatabase() to null.
//
// During actual File->New processing, getDatabase() returns null, so document knows it's
// normal New.


void PolygonsApp::OnFileOpenFromDB()
{
	CDatabase db;
	
	// Shows data source selection dialog
	if ( ! db.OpenEx(NULL) )
		return;                // Cancelled
	
	this->db = &db;

	try {
		OnFileNew();
	}
	catch (...) {
		this->db = nullptr;
		
		throw;
	}

	db.Close();
}


////////////////////////////////////////////////////////////////////////////////////////////////////

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// App command to run the dialog
void PolygonsApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}
