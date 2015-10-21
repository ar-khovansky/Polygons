#include "stdafx.h"
#include "PolygonsView.h"

#include "PolygonsDoc.h"
#include "PolygonsController.h"
#include "MainFrm.h"

#include "resource.h"

#include "Poly/Poly.h"

#include "Lib/Math.h"

#include <vector>


using namespace std;

// GDI+ is used for drawing.
// Microsoft recommends to use Direct2D in new code, but it is boilerplate-heavy, so not
// suitable to demo purposes.
using namespace Gdiplus;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif



UINT const vertexHandleSizeSeed = 4; // rectSize = vertexHandleSizeSeed * 2 + 1;

UINT msgBoxFlags = MB_OK | MB_ICONEXCLAMATION;



// PolygonsView

IMPLEMENT_DYNCREATE(PolygonsView, CView)

BEGIN_MESSAGE_MAP(PolygonsView, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)

	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, &PolygonsView::OnUpdateEditUndo)
	ON_COMMAND(ID_EDIT_UNDO, &PolygonsView::OnEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, &PolygonsView::OnUpdateEditRedo)
	ON_COMMAND(ID_EDIT_REDO, &PolygonsView::OnEditRedo)

	ON_UPDATE_COMMAND_UI(ID_EDIT_DRAWPOLYGON, &PolygonsView::OnUpdateEditDrawPolygon)
	ON_COMMAND(ID_EDIT_DRAWPOLYGON, &PolygonsView::OnEditDrawPolygon)
	ON_UPDATE_COMMAND_UI(ID_EDIT_NEWPOLYGON, &PolygonsView::OnUpdateEditNewPolygon)
	ON_COMMAND(ID_EDIT_NEWPOLYGON, &PolygonsView::OnEditNewPolygon)
	ON_UPDATE_COMMAND_UI(ID_EDIT_ADDVERTEX, &PolygonsView::OnUpdateEditAddVertex)
	ON_COMMAND(ID_EDIT_ADDVERTEX, &PolygonsView::OnEditAddVertex)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, &PolygonsView::OnUpdateEditDelete)
	ON_COMMAND(ID_EDIT_DELETE, &PolygonsView::OnEditDelete)

	ON_UPDATE_COMMAND_UI(ID_EDIT_MERGE, &PolygonsView::OnUpdateEditMerge)
	ON_COMMAND(ID_EDIT_MERGE, &PolygonsView::OnEditMerge)
	ON_UPDATE_COMMAND_UI(ID_EDIT_INTERSECT, &PolygonsView::OnUpdateEditIntersect)
	ON_COMMAND(ID_EDIT_INTERSECT, &PolygonsView::OnEditIntersect)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SUBTRACT, &PolygonsView::OnUpdateEditSubtract)
	ON_COMMAND(ID_EDIT_SUBTRACT, &PolygonsView::OnEditSubtract)
	ON_UPDATE_COMMAND_UI(ID_EDIT_XOR, &PolygonsView::OnUpdateEditXor)
	ON_COMMAND(ID_EDIT_XOR, &PolygonsView::OnEditXor)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PARTITION, &PolygonsView::OnUpdateEditPartition)
	ON_COMMAND(ID_EDIT_PARTITION, &PolygonsView::OnEditPartition)

	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()


// PolygonsView construction/destruction

PolygonsView::PolygonsView()
{
}

PolygonsView::~PolygonsView()
{
}



BOOL PolygonsView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}


void PolygonsView::OnInitialUpdate()
{
	//TRACE(__FUNCTION__"\n");

	CView::OnInitialUpdate();

	controller.reset(new PolygonsController(
		this,
	  const_cast<PolygonsDoc*>(GetDocument()),
		static_cast<IStatusPane*>(static_cast<MainFrame*>(GetParentFrame()))));
}


// PolygonsView drawing

// Drawing has no optimization. In particular:
// - Update hints from document are not used. All viewport is invalidated and redrawn
//   each time.
// - Is polygon simple or not, is calculated during drawing, while the model already
//   has calculated this, but does not expose the info.


static void drawPolygon(Graphics &graphics, poly::Polygon const &polygon)
{
	// Experimental style. Means that "vertices" is initialized by following block.
	vector<Gdiplus::PointF> vertices; {
		vertices.reserve(polygon.numVertices());
		for ( auto const &pt : polygon )
			vertices.emplace_back((float)pt.x, (float)pt.y);
	}

	bool const simple = polygon.isSimple();
	
	SolidBrush const brush(simple ? Color(0xA0, 0xFF, 0xA0) : Color(0xFF, 0xA0, 0xA0));
	graphics.FillPolygon(&brush, vertices.data(), polygon.numVertices());

	Pen const pen(simple ? Color(0, 0, 0) : Color(0xFF, 0, 0));
	graphics.DrawPolygon(&pen, vertices.data(), polygon.numVertices());
}



static void drawCurPolygon(Graphics &graphics,
                           poly::Polygon const &polygon,
													 poly::Polygon::const_iterator curVertexIt)
{
	UINT const numVertices = polygon.numVertices();
	
	// Experimental style. Means that the two variables are initialized by following block.
	vector<Gdiplus::PointF> vertices;
	UINT curVertexIdx = UINT_MAX;
	{
		vertices.reserve(numVertices);
		UINT i = 0;
		for ( auto it = polygon.begin(); it != polygon.end(); ++it, ++i ) {
			vertices.emplace_back((float)it->x, (float)it->y);
		
			if ( it == curVertexIt )
				curVertexIdx = i;
		}
	}
	
	bool const simple = polygon.isSimple();
	
	SolidBrush const brush(simple ? Color(0xA0, 0xA0, 0xA0, 0xFF) : Color(0xA0, 0xFF, 0xA0, 0xFF));
	graphics.FillPolygon(&brush, vertices.data(), numVertices);
	
	Pen const pen(simple ? Color(0, 0, 0xFF) : Color(0xFF, 0, 0xFF));
	
	graphics.DrawPolygon(&pen, vertices.data(), vertices.size());

	
	UINT const handleRectSize = vertexHandleSizeSeed * 2 + 1;
	
	// Vertex handles, except current
	
	// More experimental style
	vector<Gdiplus::RectF> vertexRects; {
		vertexRects.reserve(curVertexIt == polygon.end() ? numVertices : numVertices - 1);
		for ( UINT i = 0; i < numVertices; ++i ) {
			if ( i != curVertexIdx )
				vertexRects.emplace_back(vertices.at(i).X - vertexHandleSizeSeed,
																 vertices.at(i).Y - vertexHandleSizeSeed,
																 (float)handleRectSize, (float)handleRectSize);
		}
	}
	graphics.DrawRectangles(&pen, vertexRects.data(), vertexRects.size());

	
	// Current vertex handle
	
	if ( curVertexIt != polygon.end() ) {
		Pen const pen(Color(0, 0xA0, 0));
		Gdiplus::RectF const rect(vertices.at(curVertexIdx).X - vertexHandleSizeSeed,
		                          vertices.at(curVertexIdx).Y - vertexHandleSizeSeed,
		                          (float)handleRectSize, (float)handleRectSize);
		graphics.DrawRectangle(&pen, rect);
	}
}



void PolygonsView::OnDraw(CDC* pDC)
{
	PolygonsDoc const * const pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	Gdiplus::Graphics graphics(pDC->GetSafeHdc());
	
	// Draw current polygon last to be always visible

	auto const curPolygonIt = pDoc->getCurPolygonIt();
	
	for ( auto it = pDoc->getPolygons().begin(); it != pDoc->getPolygons().end(); ++it ) {
		if ( it != curPolygonIt )
			drawPolygon(graphics, *it);
	}

	if ( curPolygonIt != pDoc->getPolygons().end() )
		drawCurPolygon(graphics, *curPolygonIt, pDoc->getCurVertexIt());
}



void PolygonsView::showMessage(CString const &msg)
{
	AfxMessageBox(msg, msgBoxFlags);
}



bool PolygonsView::isVertexHandleHit(poly::Point const &vertex, poly::Point const &point) const
{
	return ( abs(iRound(vertex.x) - iRound(point.x)) <= vertexHandleSizeSeed &&
	         abs(iRound(vertex.y) - iRound(point.y)) <= vertexHandleSizeSeed );
}



// PolygonsView printing

BOOL PolygonsView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void PolygonsView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void PolygonsView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}


// PolygonsView diagnostics

#ifdef _DEBUG
void PolygonsView::AssertValid() const
{
	CView::AssertValid();
}

void PolygonsView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

PolygonsDoc const * PolygonsView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(PolygonsDoc)));
	return (PolygonsDoc*)m_pDocument;
}
#endif //_DEBUG


// PolygonsView message handlers

// Mostly directly pass to controller.
// Convert points to world (model) coordinates.


void PolygonsView::OnUpdateEditDrawPolygon(CCmdUI *pCmdUI)
{ controller->OnUpdateEditDrawPolygon(pCmdUI); }
void PolygonsView::OnEditDrawPolygon()
{ controller->OnEditDrawPolygon(); }

void PolygonsView::OnUpdateEditNewPolygon(CCmdUI *pCmdUI)
{ controller->OnUpdateEditNewPolygon(pCmdUI); }
void PolygonsView::OnEditNewPolygon()
{ controller->OnEditNewPolygon(); }

void PolygonsView::OnUpdateEditAddVertex(CCmdUI *pCmdUI)
{ controller->OnUpdateEditAddVertex(pCmdUI); }
void PolygonsView::OnEditAddVertex()
{ controller->OnEditAddVertex(); }

void PolygonsView::OnUpdateEditDelete(CCmdUI *pCmdUI)
{ controller->OnUpdateEditDelete(pCmdUI); }
void PolygonsView::OnEditDelete()
{ controller->OnEditDelete(); }


void PolygonsView::OnUpdateEditMerge(CCmdUI *pCmdUI)
{ controller->OnUpdateEditMerge(pCmdUI); }
void PolygonsView::OnEditMerge()
{ controller->OnEditMerge(); }

void PolygonsView::OnUpdateEditIntersect(CCmdUI *pCmdUI)
{ controller->OnUpdateEditIntersect(pCmdUI); }
void PolygonsView::OnEditIntersect()
{ controller->OnEditIntersect(); }

void PolygonsView::OnUpdateEditSubtract(CCmdUI *pCmdUI)
{ controller->OnUpdateEditSubtract(pCmdUI); }
void PolygonsView::OnEditSubtract()
{ controller->OnEditSubtract(); }

void PolygonsView::OnUpdateEditXor(CCmdUI *pCmdUI)
{ controller->OnUpdateEditXor(pCmdUI); }
void PolygonsView::OnEditXor()
{ controller->OnEditXor(); }

void PolygonsView::OnUpdateEditPartition(CCmdUI *pCmdUI)
{ controller->OnUpdateEditPartition(pCmdUI); }
void PolygonsView::OnEditPartition()
{ controller->OnEditPartition(); }


void PolygonsView::OnUpdateEditUndo(CCmdUI *pCmdUI)
{ controller->OnUpdateEditUndo(pCmdUI); }
void PolygonsView::OnEditUndo()
{ controller->OnEditUndo(); }

void PolygonsView::OnUpdateEditRedo(CCmdUI *pCmdUI)
{ controller->OnUpdateEditRedo(pCmdUI); }
void PolygonsView::OnEditRedo()
{ controller->OnEditRedo(); }


static poly::Point toWorldCoordinates(CPoint pt)
{
	return poly::Point(pt.x, pt.y);
}


void PolygonsView::OnLButtonDown(UINT nFlags, CPoint point)
{ controller->OnLButtonDown(nFlags, toWorldCoordinates(point)); }

void PolygonsView::OnLButtonUp(UINT nFlags, CPoint point)
{ controller->OnLButtonUp(nFlags, toWorldCoordinates(point)); }

void PolygonsView::OnMouseMove(UINT nFlags, CPoint point)
{ controller->OnMouseMove(nFlags, toWorldCoordinates(point)); }

void PolygonsView::OnLButtonDblClk(UINT nFlags, CPoint point)
{ controller->OnLButtonDblClk(nFlags, point); }

void PolygonsView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	controller->OnKeyDown(nChar, nRepCnt, nFlags);

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}
