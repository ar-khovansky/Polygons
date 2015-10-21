#pragma once

#include "PolygonsDoc.h"
#include "IStatusPane.h"

#include "Poly/Poly.h"

#include <memory>



class PolygonsView;



/// MVC controller
/*!
 * MFC uses Document - View architecture, where View plays roles of both View and Controller
 * from MVC. So this controller is not integrated with MFC message routing. Instead it
 * receives all input from the view via methods copying message handlers in the view. Most view's
 * message handlers simply pass their calls to the controller. This solution may be hackish,
 * but it is sufficient for the case. The goal was to demonstrate separate view and controller.
 *
 * Also controller sets status message via interface IStatusPane.
 */
class PolygonsController
{
public:
	PolygonsController(PolygonsView *view, PolygonsDoc *model, IStatusPane *statusPane);
	~PolygonsController();

	void OnUpdateEditUndo(CCmdUI *pCmdUI);
	void OnEditUndo();
	void OnUpdateEditRedo(CCmdUI *pCmdUI);
	void OnEditRedo();

	void OnUpdateEditDrawPolygon(CCmdUI *pCmdUI);
	void OnEditDrawPolygon();
	void OnUpdateEditNewPolygon(CCmdUI *pCmdUI);
	void OnEditNewPolygon();
	void OnUpdateEditAddVertex(CCmdUI *pCmdUI);
	void OnEditAddVertex();
	void OnUpdateEditDelete(CCmdUI *pCmdUI);
	void OnEditDelete();

	void OnUpdateEditMerge(CCmdUI *pCmdUI);
	void OnEditMerge();
	void OnUpdateEditIntersect(CCmdUI *pCmdUI);
	void OnEditIntersect();
	void OnUpdateEditSubtract(CCmdUI *pCmdUI);
	void OnEditSubtract();
	void OnUpdateEditXor(CCmdUI *pCmdUI);
	void OnEditXor();
	void OnUpdateEditPartition(CCmdUI *pCmdUI);
	void OnEditPartition();

	void OnLButtonDown(UINT nFlags, poly::Point const &point);
	void OnLButtonUp(UINT nFlags, poly::Point const &point);
	void OnLButtonDblClk(UINT nFlags, CPoint point);
	void OnMouseMove(UINT nFlags, poly::Point const &point);

	void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);


private:
	enum Mode { Mode_Idle, Mode_DrawPolygon, Mode_PolygonDrag, Mode_VertexDrag, Mode_AddVertex };

//
	poly::Polygon::const_iterator findHitVertex(poly::Point const &point) const;

	void setMode(Mode mode);

	void startAddVertexAction(poly::Polygon::ConstEdgeIterator edge, poly::Point const &point);


//Fields

	// Controller can directly call some view functions, for example show message box,
	// so ptr is not to const.
	PolygonsView *const view;

	PolygonsDoc *const model;

	IStatusPane *const statusPane; ///< Abstract pane displaying status messages.

	Mode mode;

	std::unique_ptr<PolygonsDoc::CreatePolygonAction> createPolygonAction;
	//TODO: incapsulate into class with createPolygonAction
	bool drawPolygonWaitForMouseMove;
	
	std::unique_ptr<PolygonsDoc::DragAction> dragAction;
};

