#pragma once

#include <memory>



class PolygonsDoc;
class PolygonsController;

namespace poly { class Point; }



/// The MVC view for PolygonsDoc.
/*!
 * In MFC's Document - View architecture, the view incorporates controller and so directly
 * modifies document. Here is an MVC view. It can only receive notifications from document,
 * and read its data.
 *
 * The view creates and owns the controller. This is not an elaborate decision, it's just
 * most simple what can be done within MFC.
 */
class PolygonsView : public CView
{
protected: // create from serialization only
	PolygonsView();
	DECLARE_DYNCREATE(PolygonsView)

// Attributes
public:
	// Note that PolygonsDoc is const. This is not the case in MFC's Document - View.
	PolygonsDoc const * GetDocument() const;

// Operations
public:

// Controller interface
public:
	/// Test if given point hits handle shape of given vertex.
	/*!
	 * Controller calls this because handles are purely presentation objects. Only view knows
	 * their shape and size. While we want to ensure that controller's "sense area" around vertex
	 * is exactly as drawn handle shape.
	 */
	bool isVertexHandleHit(poly::Point const &vertex, poly::Point const &point) const;

	// Controller calls this directly
	//
	void showMessage(CString const &msg);

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~PolygonsView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnUpdateEditUndo(CCmdUI *pCmdUI);
	afx_msg void OnEditUndo();
	afx_msg void OnUpdateEditRedo(CCmdUI *pCmdUI);
	afx_msg void OnEditRedo();

	afx_msg void OnUpdateEditDrawPolygon(CCmdUI *pCmdUI);
	afx_msg void OnEditDrawPolygon();
	afx_msg void OnUpdateEditNewPolygon(CCmdUI *pCmdUI);
	afx_msg void OnEditNewPolygon();
	afx_msg void OnUpdateEditAddVertex(CCmdUI *pCmdUI);
	afx_msg void OnEditAddVertex();
	afx_msg void OnUpdateEditDelete(CCmdUI *pCmdUI);
	afx_msg void OnEditDelete();

	afx_msg void OnUpdateEditMerge(CCmdUI *pCmdUI);
	afx_msg void OnEditMerge();
	afx_msg void OnUpdateEditIntersect(CCmdUI *pCmdUI);
	afx_msg void OnEditIntersect();
	afx_msg void OnUpdateEditSubtract(CCmdUI *pCmdUI);
	afx_msg void OnEditSubtract();
	afx_msg void OnUpdateEditXor(CCmdUI *pCmdUI);
	afx_msg void OnEditXor();
	afx_msg void OnUpdateEditPartition(CCmdUI *pCmdUI);
	afx_msg void OnEditPartition();

	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

private:
	std::unique_ptr<PolygonsController> controller;
};



#ifndef _DEBUG  // debug version in PolygonsView.cpp
inline PolygonsDoc const * PolygonsView::GetDocument() const
   { return reinterpret_cast<PolygonsDoc*>(m_pDocument); }
#endif
