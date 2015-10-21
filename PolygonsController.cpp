#include "stdafx.h"
#include "PolygonsController.h"

#include "PolygonsDoc.h"
#include "PolygonsView.h"

#include "Exceptions.h"

#include "resource.h"

#include <algorithm>


using namespace std;



UINT const newPolygonSize = 200;
double const polygonEdgeSenseDistance = 4;
double const polygonSenseDistanceSqr = poly::sqr(polygonEdgeSenseDistance);



#define CATCH_ON_UPDATE_UI \
	catch (...) {            \
		pCmdUI->Enable(false); \
	}

#define SHOW_ERROR view->showMessage(_T("Error"));

#define CATCH_ALL_SHOW_ERROR \
	catch ( call_error const &e ) {                                         \
		view->showMessage(CString("Controller error: ") + CString(e.what())); \
	}                                                                       \
	catch ( logic_error const &e ) {                                        \
		view->showMessage(CString("Logic error: ") + CString(e.what()));      \
	}                                                                       \
	catch ( exception const &e ) {                                          \
		view->showMessage(CString("Error: ") + CString(e.what()));            \
	}                                                                       \
	catch (...) {                                                           \
		SHOW_ERROR                                                            \
	}

#define CATCH_DRAW_POLYGON \
	catch (...) {                      \
		SHOW_ERROR                       \
		                                 \
		if ( createPolygonAction ) {     \
			createPolygonAction->cancel(); \
			createPolygonAction.reset();   \
		}                                \
			                               \
		setMode(Mode_Idle);              \
	}

#define CATCH_DRAG_ACTION \
	catch ( call_error const &e ) {                                         \
		view->showMessage(CString("Controller error: ") + CString(e.what())); \
		                        \
		if ( dragAction ) {     \
			dragAction->cancel(); \
			dragAction.reset();   \
		}                       \
		                        \
		setMode(Mode_Idle);     \
	}                                                                       \
	catch ( logic_error const &e ) {                                        \
		view->showMessage(CString("Logic error: ") + CString(e.what()));      \
		                        \
		if ( dragAction ) {     \
			dragAction->cancel(); \
			dragAction.reset();   \
		}                       \
		                        \
		setMode(Mode_Idle);     \
	}                                                                       \
	catch ( exception const &e ) {                                          \
		view->showMessage(CString("Error: ") + CString(e.what()));            \
		                        \
		if ( dragAction ) {     \
			dragAction->cancel(); \
			dragAction.reset();   \
		}                       \
		                        \
		setMode(Mode_Idle);     \
	}                                                                       \
	catch (...) {             \
		SHOW_ERROR              \
		                        \
		if ( dragAction ) {     \
			dragAction->cancel(); \
			dragAction.reset();   \
		}                       \
		                        \
		setMode(Mode_Idle);     \
	}



PolygonsController::
PolygonsController(PolygonsView *view, PolygonsDoc *model, IStatusPane *statusPane)
	: view(view)
	, model(model)
	, statusPane(statusPane)
	, mode(Mode_Idle)
{
	ENSURE(view);
	ENSURE(model);
	ENSURE(statusPane);
}


PolygonsController::~PolygonsController()
{
}



void PolygonsController::setMode(Mode newMode)
{
	if ( mode == newMode )
		return;

	mode = newMode;

	switch ( mode ) {
	case Mode_DrawPolygon:
		statusPane->setStatusMessage(IDS_DrawPolygonHint);
		break;
	case Mode_AddVertex:
		statusPane->setStatusMessage(IDS_AddVertexHint);
		break;
	default:
		statusPane->resetStatusMessage();
	}
}



void PolygonsController::startAddVertexAction(poly::Polygon::ConstEdgeIterator edge,
                                              poly::Point const &point)
{
	dragAction = model->startCurPolygonAddVertexAction(edge.vertex2Iterator());
	dragAction->step(point);

	statusPane->resetStatusMessage();
}



void PolygonsController::OnUpdateEditUndo(CCmdUI *pCmdUI)
{
	try {
		pCmdUI->Enable(mode == Mode_Idle && model->canUndo());
	}
	CATCH_ON_UPDATE_UI
}

void PolygonsController::OnEditUndo()
{
	try {
		if ( mode == Mode_Idle && model->canUndo() )
			model->undo();
	}
	CATCH_ALL_SHOW_ERROR
}


void PolygonsController::OnUpdateEditRedo(CCmdUI *pCmdUI)
{
	try {
		pCmdUI->Enable(mode == Mode_Idle && model->canRedo());
	}
	CATCH_ON_UPDATE_UI
}

void PolygonsController::OnEditRedo()
{
	try {
		if ( mode == Mode_Idle && model->canRedo() )
			model->redo();
	}
	CATCH_ALL_SHOW_ERROR
}



void PolygonsController::OnUpdateEditDrawPolygon(CCmdUI *pCmdUI)
{
	try {
		if ( mode == Mode_Idle )
			pCmdUI->Enable(true);
		else if ( mode == Mode_DrawPolygon ) {
			ENSURE(createPolygonAction);
			pCmdUI->Enable( ! createPolygonAction->hasVertex() );
		}
		else
			pCmdUI->Enable(false);

		pCmdUI->SetCheck(mode == Mode_DrawPolygon);
	}
	CATCH_ON_UPDATE_UI
}


void PolygonsController::OnEditDrawPolygon()
{
	try {
		if ( mode == Mode_Idle ) {
			ENSURE( ! createPolygonAction );

			createPolygonAction = model->startCreatePolygonAction();
			drawPolygonWaitForMouseMove = false;

			setMode(Mode_DrawPolygon);
		}
		else if ( mode == Mode_DrawPolygon ) {
			ENSURE( createPolygonAction );
		
			if ( createPolygonAction->hasVertex() )
				return;

			createPolygonAction->cancel();
			createPolygonAction.reset();

			setMode(Mode_Idle);
		}
	}
	CATCH_DRAW_POLYGON
}



void PolygonsController::OnUpdateEditNewPolygon(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(mode == Mode_Idle);
}


void PolygonsController::OnEditNewPolygon()
{
	try {
		if ( mode != Mode_Idle )
			return;

	
		poly::Point const topLeft(0, 0);

		list<poly::Point> vertices;

		// CCW
		vertices.push_back   (topLeft);
		vertices.emplace_back(topLeft.x,                  topLeft.y + newPolygonSize);
		vertices.emplace_back(topLeft.x + newPolygonSize, topLeft.y + newPolygonSize);
		vertices.emplace_back(topLeft.x + newPolygonSize, topLeft.y);

		model->addPolygon(poly::Polygon(vertices));
	}
	CATCH_ALL_SHOW_ERROR
}



void PolygonsController::OnUpdateEditAddVertex(CCmdUI *pCmdUI)
{
	try {
		pCmdUI->Enable(mode == Mode_Idle && model->hasCurPolygon()  ||
									 mode == Mode_AddVertex && ! dragAction );
		pCmdUI->SetCheck(mode == Mode_AddVertex);
	}
	CATCH_ON_UPDATE_UI
}


void PolygonsController::OnEditAddVertex()
{
	try {
		if ( mode == Mode_Idle && model->hasCurPolygon() ) {
			model->resetCurVertex();
			setMode(Mode_AddVertex);
		}
		else if ( mode == Mode_AddVertex && ! dragAction )
			setMode(Mode_Idle);
	}
	catch (...) {
		SHOW_ERROR;

		ASSERT(! dragAction); // Can get here only via first if
		setMode(Mode_Idle);
	}
}



void PolygonsController::OnUpdateEditDelete(CCmdUI *pCmdUI)
{
	try {
		pCmdUI->Enable( mode == Mode_Idle &&
										(model->activeObjectIsPolygon() || model->canDeleteCurVertex()) );
	}
	CATCH_ON_UPDATE_UI
}


void PolygonsController::OnEditDelete()
{
	try {
		if ( ! mode == Mode_Idle )
			return;
	
		if ( model->activeObjectIsPolygon() )
			model->deleteCurPolygon();
		else if ( model->canDeleteCurVertex() )
			model->deleteCurVertex();
	}
	CATCH_ALL_SHOW_ERROR
}



void PolygonsController::OnUpdateEditMerge(CCmdUI *pCmdUI)
{
	try {
		pCmdUI->Enable(mode == Mode_Idle && model->hasCurPolygon());
	}
	CATCH_ON_UPDATE_UI
}

void PolygonsController::OnEditMerge()
{
	try {
		if ( mode == Mode_Idle && model->hasCurPolygon() )
			model->mergeCurPolygonWithOther();
	}
	CATCH_ALL_SHOW_ERROR
}

void PolygonsController::OnUpdateEditIntersect(CCmdUI *pCmdUI)
{
	try {	
		pCmdUI->Enable(mode == Mode_Idle && model->hasCurPolygon());
	}
	CATCH_ON_UPDATE_UI
}

void PolygonsController::OnEditIntersect()
{
	try {
		if ( mode == Mode_Idle && model->hasCurPolygon() )
			model->intersectCurPolygonWithOther();
	}
	CATCH_ALL_SHOW_ERROR
}


void PolygonsController::OnUpdateEditSubtract(CCmdUI *pCmdUI)
{
	try {
		pCmdUI->Enable(mode == Mode_Idle && model->hasCurPolygon());
	}
	CATCH_ON_UPDATE_UI
}

void PolygonsController::OnEditSubtract()
{
	try {
		if ( mode == Mode_Idle && model->hasCurPolygon() )
			model->subtractCurPolygonFromOther();
	}
	CATCH_ALL_SHOW_ERROR
}


void PolygonsController::OnUpdateEditXor(CCmdUI *pCmdUI)
{
	try {
		pCmdUI->Enable(mode == Mode_Idle && model->hasCurPolygon());
	}
	CATCH_ON_UPDATE_UI
}

void PolygonsController::OnEditXor()
{
	try {
		if ( mode == Mode_Idle && model->hasCurPolygon() )
			model->xorCurPolygonWithOther();
	}
	CATCH_ALL_SHOW_ERROR
}


void PolygonsController::OnUpdateEditPartition(CCmdUI *pCmdUI)
{
	try {
		pCmdUI->Enable(mode == Mode_Idle && model->hasCurPolygon());
	}
	CATCH_ON_UPDATE_UI
}

void PolygonsController::OnEditPartition()
{
	try {
		if ( mode == Mode_Idle && model->hasCurPolygon() )
			model->partitionPolygonByCurPolygon();
	}
	CATCH_ALL_SHOW_ERROR
}



/// Test if given point hits one of polygon edges, considering sense distance.
/*! Hit means that point's projection lies inside edge.
 */
static poly::Polygon::ConstEdgeIterator testEdgeHit(poly::Polygon const &polygon,
                                                    poly::Point const &point)
{
	double dMin = DBL_MAX;
	poly::Polygon::ConstEdgeIterator closestEdge = polygon.edgeEnd();

	for ( auto edge = polygon.edgeBegin(); edge != polygon.edgeEnd(); ++edge ) {
		// Need to hit inside edge segment, so use strict function
		double const d = distanceSqr_Strict(point, *edge);
		if ( d < dMin ) {
			dMin = d;
			closestEdge = edge;
		}
		if ( dMin == 0 )
			break;
	}

	return dMin <= polygonSenseDistanceSqr ? closestEdge : polygon.edgeEnd();
}



/// Find polygon hit by given point.
//
static list<poly::Polygon>::const_iterator findHitPolygon(list<poly::Polygon> const &polygons,
                                                          poly::Point const &point)
{
	return find_if(polygons.begin(), polygons.end(),
	               [&point](poly::Polygon const &polygon){ return poly::inside(point, polygon); });
}



poly::Polygon::const_iterator PolygonsController::findHitVertex(poly::Point const &point) const
{
	auto const &curPolygon = model->getCurPolygon();
	return find_if(curPolygon.begin(), curPolygon.end(),
	               [&](poly::Point const &vertex){ return view->isVertexHandleHit(vertex, point); });
}



void PolygonsController::OnLButtonDown(UINT nFlags, poly::Point const &point)
{
	//TRACE(__FUNCTION__"\n");
	
	switch ( mode ) {
		case Mode_DrawPolygon:
			try {
				ENSURE( createPolygonAction );

				if ( ! createPolygonAction->hasVertex() )
					createPolygonAction->addVertex(point);

				drawPolygonWaitForMouseMove = true;
			}
			CATCH_DRAW_POLYGON
		break;


		case Mode_AddVertex:
			try {
				poly::Polygon const &curPolygon = model->getCurPolygon();

				auto const edge = testEdgeHit(curPolygon, point);

				if ( edge != curPolygon.edgeEnd() )
					startAddVertexAction(edge, point);
				else {
					setMode(Mode_Idle);
					goto _Mode_Idle;
				}
			}
			CATCH_DRAG_ACTION
		break;


		case Mode_Idle:
_Mode_Idle:
			try {
				if ( model->hasCurPolygon() ) {
					poly::Polygon const &curPolygon = model->getCurPolygon();

					poly::Polygon::const_iterator const hitVertex = findHitVertex(point);
				
					if ( hitVertex != curPolygon.end() ) {
						model->setCurVertex(hitVertex);
					
						setMode(Mode_VertexDrag);
						dragAction = model->startCurVertexDragAction(point);

						return;
					}
					else
						model->resetCurVertex();

					if ( nFlags & MK_SHIFT ) {
						auto const edge = testEdgeHit(curPolygon, point);

						if ( edge != curPolygon.edgeEnd() ) {
							mode = Mode_AddVertex; // Not setMode to bypass status set
							startAddVertexAction(edge, point);
							statusPane->resetStatusMessage();

							return;
						}
					}
				}

				auto const polygon = findHitPolygon(model->getPolygons(), point);
				if ( polygon != model->getPolygons().end() ) {
					model->setCurPolygon(polygon);
					
					setMode(Mode_PolygonDrag);
					dragAction = model->startCurPolygonDragAction(point);

					return;
				}

				model->resetCurPolygon();
			}
			CATCH_DRAG_ACTION
		break;
	}
}



void PolygonsController::OnMouseMove(UINT nFlags, poly::Point const &point)
{
	//TRACE(__FUNCTION__": %f, %f\n", point.x, point.y);
	
	switch ( mode ) {
		case Mode_DrawPolygon:
			try {
				ENSURE( createPolygonAction );

				if ( ! createPolygonAction->hasVertex() )
					return;
		
				if ( drawPolygonWaitForMouseMove ) {
					createPolygonAction->addVertex(point);
					drawPolygonWaitForMouseMove = false;
				}
				else
					createPolygonAction->moveLastVertex(point);
			}
			CATCH_DRAW_POLYGON
		break;

		case Mode_PolygonDrag:
		case Mode_VertexDrag:
			try {
				ENSURE( dragAction );

				dragAction->step(point);
			}
			CATCH_DRAG_ACTION
		break;

		case Mode_AddVertex:
			try {
				if ( nFlags & MK_LBUTTON ) {
					ENSURE( dragAction );

					dragAction->step(point);
				}
			}
			CATCH_DRAG_ACTION
		break;

		case Mode_Idle:
			try {
				if ( ! (nFlags == 0 && model->hasCurPolygon()) )
					return;

				auto const &polygon = model->getCurPolygon();
				auto const edge = testEdgeHit(polygon, point);

				if ( edge != polygon.edgeEnd() )
					statusPane->setStatusMessage(IDS_EdgeHoverHint);
				else
					statusPane->resetStatusMessage();
			}
			CATCH_ALL_SHOW_ERROR
		break;
	}
}



void PolygonsController::OnLButtonUp(UINT nFlags, poly::Point const &point)
{
	//TRACE(__FUNCTION__"\n");
	
	switch ( mode ) {
		case Mode_PolygonDrag:
		case Mode_VertexDrag:
		case Mode_AddVertex:
			try {
				ENSURE( dragAction );

				dragAction->finish();
				dragAction.reset();
				setMode(Mode_Idle);
			}
			CATCH_DRAG_ACTION
		break;
	}
}



void PolygonsController::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	//TRACE(__FUNCTION__"\n");

	switch ( mode ) {
		case Mode_DrawPolygon:
			try {
				ENSURE( createPolygonAction );

				createPolygonAction->finish();
				createPolygonAction.reset();

				setMode(Mode_Idle);
			}
			CATCH_DRAW_POLYGON
		break;
	}
}



void PolygonsController::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	switch ( mode ) {
		case Mode_DrawPolygon:
			if ( nChar == VK_ESCAPE ) {
				ASSERT( createPolygonAction );

				if ( createPolygonAction ) {
					createPolygonAction->cancel();
					createPolygonAction.reset();
				}

				setMode(Mode_Idle);
			}
		break;
	}
}
