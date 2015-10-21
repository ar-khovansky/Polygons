#pragma once

#include "IStatusPane.h"



class MainFrame : public CFrameWnd
                , public IStatusPane
{
	
protected: // create from serialization only
	MainFrame();
	DECLARE_DYNCREATE(MainFrame)

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
private:
	// IStatusPane overrides
	void setStatusMessage(UINT id) override;
	void resetStatusMessage() override;

// Implementation
public:
	virtual ~MainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CToolBar          m_wndToolBar;
	CStatusBar        m_wndStatusBar;

	// Desired status message. We have to store it, because we are not the only user of status bar.
	UINT statusMsgId;

// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnSetMessageString(WPARAM wParam, LPARAM lParam);
};


