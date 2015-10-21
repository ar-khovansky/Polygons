
// Polygons.h : main header file for the Polygons application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


// PolygonsApp:
// See Polygons.cpp for the implementation of this class
//

class PolygonsApp : public CWinApp
{
public:
	PolygonsApp();

	/// Returns open database when "Open from DB" is in progress, null otherwise.
	/*! See OnFileOpenFromDB() for details.
	 */
	CDatabase * getDatabase() const { return db; }

// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnFileOpenFromDB();

private:
	ULONG_PTR gdipToken; ///< GDI+ token

	// This points to open database when opening document from DB. In rest of time it is null.
	// See OnFileOpenFromDB() for details.
	CDatabase *db;
};



extern PolygonsApp theApp;
