#pragma once

#include "Poly/Poly.h"   // Geometry library

#include <list>
#include <memory>



/// Polygons document.
/*!
 * Architecturally, the document implements
 * - application layer logic,
 * - presentation model for the presentation layer (MVC),
 * - data access layer (for databases via ODBC),
 * - part of storage logic (for files).
 * For domain logic (geometry), document uses library named Poly.
 *
 * The document is operated by the MVC controller and notifies its view about changes.
 * The controller is not part of MFC. View notifications work via standard MFC mechanism.
 *
 * Presentation model part is not arranged separately. It consists of current (selected)
 * polygon / vertex and related logic.
 *
 * Document supports undo/redo of domain (geometric) operations.
 *
 * Document can store domain (geometry) state in ODBC database and file of custom format
 * (*.poly).
 *
 * Most of document's API is essentially API of presentation model because most calls
 * use concept of current (selected) object, which is presentation layer concept. In such
 * cases control path is controller -> presentation model -> application model.
 *
 * API is mostly procedural style. Only composite actions (see below) have
 * object-oriented interface.
 */
class PolygonsDoc : public CDocument
{
protected: // create from serialization only
	PolygonsDoc();
	DECLARE_DYNCREATE(PolygonsDoc)

public:

	/*!
	 * Composite user actions
	 * -----------------------
	 *
	 * Composite action is assembled from several steps - mouse clicks and movements,
	 * but represents atomic action from user's point of view. For example, drawing
	 * polygon from first click to finishing double click, or dragging object from
	 * button down to button up. Such action is represented in history as a single item.
	 *
	 * The following interfaces are used by the MVC controller, who obtains them
	 * via factory methods like "startSomeAction". When composite action is in progress,
	 * no other action can be performed, which is enforced by the document by throwing
	 * exceptions.
	 */

	/// Base interface for composite actions.
	/*! It's not given out to the client though.
	 */
	class CompositeUserAction {
	public:
		virtual ~CompositeUserAction() {}

		/// Finish action.
		/*! 
		 * If result is accepted by the application logic, the action is committed
		 * to the history. If application logic rejects the result, the action is
		 * rolled back, which is equivalent to cancel(). Resulting polygon is rejected
		 * if it is not simple (is self-intersecting or self-touching).
		 *
		 * After this call, regardless of outcome, the object is in finished state.
		 * \see finished()
		 *
		 * \return True if action is accepted, false otherwise.
		 *
		 * \throw state_error If action is already finished.
		 */
		virtual bool finish() = 0;
		
		/// Cancel action.
		/*!
		 * The action is rolled back and discarded. It is not get into history.
		 *
		 * After this call the object is in finished state.
		 * \see finished()
		 *
		 * Cancel is called automatically on destruction, though it is recommended to
		 * call it explicitly.
		 *
		 * Action is effectively cancelled on any exception, other than thrown by
		 * the action itself to indicate erroneous call (e.g. invalid argument).
		 *
		 * Note that cancel() does not throw when called on finished object. This is
		 * to allow uniform cleanup after exce
		 */
		virtual void cancel() = 0;
		
		/// Tell if action is finished.
		/*!
		 * An action becomes finished
		 * 1) after a call to finish() or cancel();
		 * 2) after an exception is thrown, other than thrown by the action itself to
		 * indicate erroneous call (e.g. invalid argument).
		 * Finished action cannot be reused and should be deleted.
		 *
		 * After current action is finished, other actions can be performed.
		 *
		 * Any non-const call on finished object, except cancel(), results in exception.
		 */
		virtual bool finished() const = 0;
	};
	

	/// Base interface for dragging actions.
	//!
	class DragAction : virtual public CompositeUserAction {
	public:
		/// Perform a step in dragging action.
		/*! In other words, drag object to specified point.
		 *
		 * \param dstPos  New position of anchor point, specified on action start.
		 *
		 * \throw state_error If called on finished object.
		 */
		virtual void step(poly::Point const &dstPos) = 0;
	};


	/// Action of incremental assembling polygon from vertices (e.g. drawing).
	/// 
	class CreatePolygonAction : virtual public CompositeUserAction
	{
	public:
		/// Add vertex at specified position.
		/*!
		 * \param pos  Vertex position.
		 *
		 * \throw state_error If called on finished object.
		 */
		virtual void addVertex(poly::Point const &pos) = 0;

		/// Move last added vertex to specified position.
		/*!
		 * \param pos  New vertex position.
		 *
		 * \throw state_error If there are no vertices yet, or
		 *                    if called on finished object.
		 */
		virtual void moveLastVertex(poly::Point const &pos) = 0;

		/// Tell if at least one vertex has been added.
		///
		virtual bool hasVertex() const = 0;
	};


// Attributes
public:
	std::list<poly::Polygon> const & getPolygons() const;
	bool hasCurPolygon() const;
	std::list<poly::Polygon>::const_iterator getCurPolygonIt() const;
	poly::Polygon const & getCurPolygon() const;
	bool hasCurVertex() const;
	poly::Polygon::const_iterator getCurVertexIt() const;
	bool activeObjectIsPolygon() const;
	bool canDeleteCurVertex() const;

// Operations
public:
	void setCurPolygon(std::list<poly::Polygon>::const_iterator it);
	void resetCurPolygon();

	void setCurVertex(poly::Polygon::const_iterator it);
	void resetCurVertex();


	void addPolygon(poly::Polygon polygon);

	void deleteCurPolygon();
	void deleteCurVertex();

	std::unique_ptr<CreatePolygonAction> startCreatePolygonAction();

	std::unique_ptr<DragAction> startCurPolygonDragAction(poly::Point const &anchorSrcPos);
	
	std::unique_ptr<DragAction> startCurVertexDragAction(poly::Point const &anchorSrcPos);

	std::unique_ptr<DragAction> startCurPolygonAddVertexAction(
		poly::Polygon::const_iterator beforeVertex);

	void mergeCurPolygonWithOther();
	void intersectCurPolygonWithOther();
	void subtractCurPolygonFromOther();
	void xorCurPolygonWithOther();
	void partitionPolygonByCurPolygon();

	bool canUndo() const;
	void undo();
	bool canRedo() const;
	void redo();


// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual void DeleteContents();
	virtual void Serialize(CArchive& ar);

// Implementation
public:
	virtual ~PolygonsDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnFileSaveInDB();


private:
	class CompositeUserActionImpl;
	
	class CreatePolygonActionImpl;
	class CurPolygonDragAction;
	class CurVertexDragAction;
	class CurPolygonAddVertexAction;

	typedef std::list<poly::Polygon>::iterator       PolygonsIterator;
	typedef std::list<poly::Polygon>::const_iterator PolygonsCIterator;
	typedef poly::Polygon::iterator       VerticesIterator;
	typedef poly::Polygon::const_iterator VerticesCIterator;

//
	// Pimple / Compiler firewall idiom.
	// In our case the idiom has one extra advantage: easy and robust document
	// reinitialization when it is reused by MFC.
	
	class Private;
	Private *d;
};
