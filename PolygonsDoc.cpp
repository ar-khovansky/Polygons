#include "stdafx.h"
#include "PolygonsDoc.h"

#include "PolygonsDoc_Private.h"

#include "Resource.h"
#include "Polygons.h"
#include "PointsRecordset.h"
#include "Actions.h"
#include "Events.h"
#include "IteratorByIdx.h"
#include "Exceptions.h"

#include "Lib/Iterators.h"


using namespace std;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
// Main actions


class Act_AddPolygon : public Action
{
public:
	/// \param polygon  Polygon to add
	Act_AddPolygon(poly::Polygon const &polygon) : _polygon(polygon) { ENSURE(! polygon.empty()); }
	Act_AddPolygon(poly::Polygon      &&polygon) : _polygon(polygon) { ENSURE(! polygon.empty()); }

	EventList apply(list<poly::Polygon> &polygons) override
	{
		vector<Event> events(1, Event(Event::Polygon, Event::Added, polygons.size()));
		
		polygons.push_back(move(_polygon));

		return EventList(move(events));
	}
	
	EventList undo(list<poly::Polygon> &polygons) override {
		ENSURE(! polygons.empty());

		vector<Event> events(1, Event(Event::Polygon, Event::Deleted, polygons.size() - 1));
		
		_polygon = move(polygons.back());
		polygons.pop_back();

		return EventList(move(events));
	}

	/// Get polygon to add
	/*! \throw state_error If called when Done.
	 */
	poly::Polygon & polygon() {
		// When Done, polygon is moved out
		if ( done() )
			throw state_error("Called when done");

		return _polygon;
	}

	using Action::apply;
	using Action::undo;

private:
	// Composite actions want to update the polygon, but due to moving implementation of this
	// class, polygon is moved out of here to state when action is Done. So the polygon is
	// protected with accessor.
	poly::Polygon _polygon;
};



class Act_DeletePolygon : public Action
{
public:
	UINT const polygonIdx;

//
	/// \param polygonIdx  Index of polygon to delete.
	//
	Act_DeletePolygon(UINT polygonIdx) : polygonIdx(polygonIdx) {}

	EventList apply(list<poly::Polygon> &polygons) override {
		vector<Event> events(1, Event(Event::Polygon, Event::Deleted, polygonIdx));
		
		auto const it = polygonIteratorByIdx(polygons, polygonIdx);
		polygon = move(*it);
		polygons.erase(it);

		return EventList(move(events));
	}

	EventList undo(list<poly::Polygon> &polygons) override {
		vector<Event> events(1, Event(Event::Polygon, Event::Added, polygonIdx));

		polygons.insert(polygonIteratorByIdx(polygons, polygonIdx), move(polygon));
	
		return EventList(move(events));
	}

private:
	poly::Polygon polygon;
};



class Act_MovePolygon : public Action
{
public:
	UINT const polygonIdx;
	poly::Vector vector;

//
	Act_MovePolygon(UINT polygonIdx, poly::Vector const &vector)
		: polygonIdx(polygonIdx), vector(vector) {}

	EventList apply(list<poly::Polygon> &polygons) override {
		polygonByIdx(polygons, polygonIdx).translate(vector);
		return EventList();
	}

	EventList undo(list<poly::Polygon> &polygons) override {
		polygonByIdx(polygons, polygonIdx).translate(-vector);
		return EventList();
	}

	using Action::apply;
	using Action::undo;
};



class Act_AddVertex : public Action
{
public:
	UINT const polygonIdx;
	UINT const vertexIdx;
	poly::Point vertex;

//
	Act_AddVertex(UINT polygonIdx, UINT vertexIdx, poly::Point const &vertex)
		: polygonIdx(polygonIdx), vertexIdx(vertexIdx), vertex(vertex) {}

	EventList apply(list<poly::Polygon> &polygons) override {
		vector<Event> events(1, Event(Event::Vertex, Event::Added, polygonIdx, vertexIdx));

		auto &polygon = polygonByIdx(polygons, polygonIdx);
		polygon.insertVertex(vertexIteratorByIdx(polygon, vertexIdx), vertex);

		return EventList(move(events));
	}

	EventList undo(list<poly::Polygon> &polygons) override {
		vector<Event> events(1, Event(Event::Vertex, Event::Deleted, polygonIdx, vertexIdx));

		auto &polygon = polygonByIdx(polygons, polygonIdx);
		polygon.removeVertex(vertexIteratorByIdx(polygon, vertexIdx));

		return EventList(move(events));
	}

	using Action::apply;
	using Action::undo;
};



class Act_DeleteVertex : public Action
{
public:
	UINT const polygonIdx;
	UINT const vertexIdx;
	
//
	Act_DeleteVertex(UINT polygonIdx, UINT vertexIdx)
		: polygonIdx(polygonIdx), vertexIdx(vertexIdx) {}

	EventList apply(list<poly::Polygon> &polygons) override {
		vector<Event> events(1, Event(Event::Vertex, Event::Deleted, polygonIdx, vertexIdx));

		auto &polygon = polygonByIdx(polygons, polygonIdx);
		auto const it = vertexIteratorByIdx(polygon, vertexIdx);
		vertex = *it;
		// Throws if last vertex
		polygon.removeVertex(it);

		return EventList(move(events));
	}

	EventList undo(list<poly::Polygon> &polygons) override {
		vector<Event> events(1, Event(Event::Vertex, Event::Added, polygonIdx, vertexIdx));

		auto &polygon = polygonByIdx(polygons, polygonIdx);
		polygon.insertVertex(vertexIteratorByIdx(polygon, vertexIdx), vertex);

		return EventList(move(events));
	}

private:
	poly::Point vertex;
};



class Act_MoveVertex : public Action
{
public:
	UINT const polygonIdx;
	UINT const vertexIdx;
	poly::Vector vector;

//
	Act_MoveVertex(UINT polygonIdx, UINT vertexIdx, poly::Vector const &vector)
		: polygonIdx(polygonIdx), vertexIdx(vertexIdx), vector(vector) {}

	EventList apply(list<poly::Polygon> &polygons) override {
		vertex(polygons) += vector;
		return EventList();
	}
	EventList undo(list<poly::Polygon> &polygons) override {
		vertex(polygons) -= vector;
		return EventList();
	}

	using Action::apply;
	using Action::undo;

private:
	poly::Point & vertex(list<poly::Polygon> &polygons)
		{ return * vertexIteratorByIdx(polygons, polygonIdx, vertexIdx); }
};

////////////////////////////////////////////////////////////////////////////////////////////////////


/// Base for actions representing boolean operations on polygons.
/*!
 * A boolean operation takes two polygons and produces with several result polygons. The First
 * polygon is consumed by operation (deleted). Whether the second is consumed is optional.
 *
 * Zero result polygons is ok on this logic level.
 * 
 * Children of this class need to implement only doOperation() method.
 */
class BooleanOperation : public Action
{
public:
	UINT const p1Idx, p2Idx;
	bool const preservePolygon2;   ///< Do not consume second polygon

//
	BooleanOperation(UINT p1Idx, UINT p2Idx, bool preservePolygon2 = false)
		: p1Idx(p1Idx), p2Idx(p2Idx), preservePolygon2(preservePolygon2)
	{
		ENSURE(p2Idx != p1Idx);
	}

	EventList apply(list<poly::Polygon> &polygons) override final;
	EventList undo (list<poly::Polygon> &polygons) override final;

protected:
	virtual vector<poly::Polygon> doOperation(poly::Polygon const &p1, poly::Polygon const &p2) = 0;

//
	poly::Polygon p1, p2;
	UINT numResultPolygons;
};



EventList BooleanOperation::apply(list<poly::Polygon> &polygons)
{
	UINT const numDeletedPolygons = preservePolygon2 ? 1 : 2;

	auto const p1It = polygonIteratorByIdx(polygons, p1Idx);
	auto const p2It = polygonIteratorByIdx(polygons, p2Idx);
	
	vector<poly::Polygon> const result = doOperation(*p1It, *p2It);
	
	
	vector<Event> events;
	events.reserve(numDeletedPolygons + result.size());
	events.emplace_back(Event::Polygon, Event::Deleted, p1Idx);
	if ( ! preservePolygon2 )
		events.emplace_back(Event::Polygon, Event::Deleted, p2Idx);
	for ( UINT i = polygons.size() - numDeletedPolygons;
		         i < polygons.size() - numDeletedPolygons + result.size();
						 ++i )
		events.emplace_back(Event::Polygon, Event::Added, i);


	p1 = move(*p1It); // no throw
	if ( ! preservePolygon2 )
		p2 = move(*p2It); // no throw
	numResultPolygons = result.size();

	UINT const formerPolygonsSize = polygons.size();
	try {
		// Basic exception guarantee: on exception target container is valid,
		// but new elements are still there
		move(result.begin(), result.end(), back_inserter(polygons));
	}
	catch (...) {
		truncate(polygons, formerPolygonsSize); // no throw

		*p1It = move(p1); // no throw
		if ( ! preservePolygon2 )
			*p2It = move(p2); // no throw

		throw;
	}

	polygons.erase(p1It); // no throw
	if ( ! preservePolygon2 )
		polygons.erase(p2It); // no throw

	return EventList(move(events)); // no throw
}



EventList BooleanOperation::undo(list<poly::Polygon> &polygons)
{
	vector<Event> events;
	events.reserve(numResultPolygons + (preservePolygon2 ? 1 : 2));
	for ( UINT i = polygons.size() - numResultPolygons; i < polygons.size(); ++i )
		events.emplace_back(Event::Polygon, Event::Deleted, i);
	events.emplace_back(Event::Polygon, Event::Added, p1Idx);
	if ( ! preservePolygon2 )
		events.emplace_back(Event::Polygon, Event::Added, p2Idx);


	// First insert saved polygons
	// Insert polygon with lesser index first
	
	struct Insert {
		UINT pIdx;
		poly::Polygon &p;
	} const insert[2] = {{p1Idx, p1}, {p2Idx, p2}};
	Insert const *insert1 = insert, *insert2 = insert + 1;
	if ( ! preservePolygon2 && p2Idx < p1Idx )
		swap(insert1, insert2);
	
	// Strong exception guarantee
	auto const it1 = polygons.insert(polygonIteratorByIdx(polygons, insert1->pIdx),
	                                 move(insert1->p));
		
	if ( ! preservePolygon2 ) {
		try {
			// Strong exception guarantee
			polygons.insert(polygonIteratorByIdx(polygons, insert2->pIdx), move(insert2->p));
		}
		catch (...) {
			polygons.erase(it1); // no throw
			throw;
		}
	}

	// Now erase result
	
	erase_n_back(polygons, numResultPolygons); // no throw

	return EventList(move(events)); // no throw
}

////////////////////////////////////////////////////////////////////////////////////////////////////


class Act_MergePolygons : public BooleanOperation
{
public:
	Act_MergePolygons(UINT p1Idx, UINT p2Idx) : BooleanOperation(p1Idx, p2Idx) {}

private:
	vector<poly::Polygon> doOperation(poly::Polygon const &p1, poly::Polygon const &p2) override
		{ return poly::add(p1, p2); }
};



class Act_IntersectPolygons : public BooleanOperation
{
public:
	Act_IntersectPolygons(UINT p1Idx, UINT p2Idx) : BooleanOperation(p1Idx, p2Idx) {}

private:
	vector<poly::Polygon> doOperation(poly::Polygon const &p1, poly::Polygon const &p2) override
		{ return poly::intersect(p1, p2); }
};



class Act_SubtractPolygons : public BooleanOperation
{
public:
	Act_SubtractPolygons(UINT p1Idx, UINT p2Idx) : BooleanOperation(p1Idx, p2Idx) {}

private:
	vector<poly::Polygon> doOperation(poly::Polygon const &p1, poly::Polygon const &p2) override
		{ return poly::subtract(p1, p2); }
};



class Act_XorPolygons : public BooleanOperation
{
public:
	Act_XorPolygons(UINT p1Idx, UINT p2Idx) : BooleanOperation(p1Idx, p2Idx) {}

private:
	vector<poly::Polygon> doOperation(poly::Polygon const &p1, poly::Polygon const &p2) override
		{ return poly::xor(p1, p2); }
};



/// Partition polygon action
/*!
 * Source polygon is partitioned by overlay polygon. The overlay remain unchanged.
 */
class Act_PartitionPolygon : public BooleanOperation
{
public:
	Act_PartitionPolygon(UINT p1Idx, UINT p2Idx) : BooleanOperation(p1Idx, p2Idx, true) {}

private:
	vector<poly::Polygon> doOperation(poly::Polygon const &p1, poly::Polygon const &p2) override
		{ return poly::partition(p1, p2); }
};



////////////////////////////////////////////////////////////////////////////////////////////////////
// PolygonsDoc

IMPLEMENT_DYNCREATE(PolygonsDoc, CDocument)

BEGIN_MESSAGE_MAP(PolygonsDoc, CDocument)
	ON_COMMAND(ID_FILE_SAVEINDB, &PolygonsDoc::OnFileSaveInDB)
END_MESSAGE_MAP()


// PolygonsDoc construction/destruction

PolygonsDoc::PolygonsDoc()
	: d(nullptr)
{
}

PolygonsDoc::~PolygonsDoc()
{
	delete d;
}



BOOL PolygonsDoc::OnNewDocument()
{
	//TRACE(__FUNCTION__"\n");

	if (!CDocument::OnNewDocument())
		return FALSE;

	// See PolygonsApp::OnFileOpenFromDB() for details.
	CDatabase *const db = theApp.getDatabase();
	if ( db )
		return d->loadFromDB(db);

	return TRUE;
}



void PolygonsDoc::DeleteContents()
{
	//TRACE(__FUNCTION__"\n");

	// Easy and robust (cannot forget to clear some member) with Pimple
	delete d;
	d = new Private(this);

	CDocument::DeleteContents();
}


// PolygonsDoc serialization

void PolygonsDoc::Serialize(CArchive& ar)
{
	//TODO: error handling
		
	if (ar.IsStoring())
	{
		ar << (UINT) d->polygons.size();

		for ( auto const &polygon : d->polygons ) {
			ar << (UINT) polygon.numVertices();

			for ( auto const &vertex : polygon )
				ar << vertex.x << vertex.y;
		}
	}
	else
	{
		UINT numPolygons;
		ar >> numPolygons;

		for ( UINT i = 0; i < numPolygons; ++i ) {
			UINT numVertices;
			ar >> numVertices;

			list<poly::Point> vertices;
			
			for ( UINT j = 0; j < numVertices; ++j ) {
				poly::Point vertex;
				ar >> vertex.x >> vertex.y;

				vertices.push_back(vertex);
			}

			poly::Polygon polygon(vertices);

			//if ( ! polygon.isSimple() )

			d->polygons.push_back(move(polygon));
		}
	}
}


// PolygonsDoc diagnostics

#ifdef _DEBUG
void PolygonsDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void PolygonsDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG



/// Get all polygons.
///
list<poly::Polygon> const & PolygonsDoc::getPolygons() const
{	return d->polygons; }


/// Tell if there is current polygon.
///
bool PolygonsDoc::hasCurPolygon() const
{ return d->hasCurPolygon(); }


/// Get current polygon iterator.
/*!
	* The condition
	*     getCurPolygonIt() == getPolygons().end()
	* means no current polygon.
	*/
list<poly::Polygon>::const_iterator PolygonsDoc::getCurPolygonIt() const
{	return d->curPolygonIt; }


/// Get current polygon.
/*!
	* \throw state_error If there is no current polygon.
	*/
poly::Polygon const & PolygonsDoc::getCurPolygon() const
{
	if ( ! hasCurPolygon() )
		throw state_error("No current polygon");
	
	return *d->curPolygonIt;
}


/// Tell if there is current vertex.
/*! This implies hasCurPolygon() == true.
	*/
bool PolygonsDoc::hasCurVertex() const
{ return d->hasCurVertex(); }


/// Get current vertex iterator in current polygon.
/*!
	* The condition
	*     getCurVertexIt() == getCurPolygon().end()
	* means no current vertex.
	*
	* \throw state_error If there is no current polygon.
	*/
poly::Polygon::const_iterator PolygonsDoc::getCurVertexIt() const
{
	if ( ! hasCurPolygon() )
		throw state_error("No current polygon");
	
	return d->curVertexIt;
}

	
/// Tells if current active object is polygon.
/*! I.e. there is current polygon, but no current vertex.
	*/
bool PolygonsDoc::activeObjectIsPolygon() const
{ return hasCurPolygon() && ! hasCurVertex(); }

	
/// Tells if it is possible to delete current vertex.
/*! It is not possible if polygon has three vertices.
	*/
bool PolygonsDoc::canDeleteCurVertex() const
{
	return hasCurVertex() && d->curPolygonIt->numVertices() > 3;
}



// PolygonsDoc commands


////////////////////////////////////////////////////////////////////////////////////////////////////
// Currect polygon / vertex


/// Set current polygon to specified.
/*!
 * \throw state_error If other action is in progress.
 */
void PolygonsDoc::setCurPolygon(PolygonsCIterator it)
{
	d->checkNoActionInProgress();
	
	d->setCurPolygon(it);
}


/// Reset current polygon.
/*!
 * \throw state_error If other action is in progress.
 */
void PolygonsDoc::resetCurPolygon()
{
	d->checkNoActionInProgress();
	
	d->resetCurPolygon();
}


/// Set current vertex to specified.
/*!
 * \throw state_error If other action is in progress.
 * \throw state_error If no current polygon.
 */
void PolygonsDoc::setCurVertex(VerticesCIterator it)
{
	d->checkNoActionInProgress();

	d->setCurVertex(it);
}


/// Set current vertex to specified.
/*!
 * \throw state_error If other action is in progress.
 */
void PolygonsDoc::resetCurVertex()
{
	d->checkNoActionInProgress();

	d->resetCurVertex();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Main actions


/// Add specified polygon to document.
/*!
 * \return Iterator to added polygon.
 *
 * \throw invalid_argument If polygon is empty.
 * \throw state_error      If other action is in progress.
 */
void PolygonsDoc::addPolygon(poly::Polygon polygon)
{
	if ( polygon.empty() )
		throw invalid_argument("Polygon is empty");
	
	d->doAction(unique_ptr<Action>(new
		Act_AddPolygon(move(polygon))));
}


/// Delete current polygon.
/*!
 * \throw state_error If other action is in progress.
 * \throw state_error If there is no current polygon.
 */
void PolygonsDoc::deleteCurPolygon()
{
	d->startAction();
	
	d->doAction(unique_ptr<Action>(new
		Act_DeletePolygon(d->curPolygonIdx())));
}


/// Delete current vertex.
/*!
 * \throw state_error If other action is in progress.
 * \throw state_error If there is no current vertex.
 */
void PolygonsDoc::deleteCurVertex()
{
	d->startAction();

	if ( ! canDeleteCurVertex() )
		throw state_error("Cannot delete current vertex");

	d->doAction(unique_ptr<Action>(new
		Act_DeleteVertex(d->curPolygonIdx(), d->curVertexIdx())));
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Boolean operations


/// Merge (unite) current polygon with other intersecting polygon.
/*!
 * \throw state_error If other action is in progress.
 * \throw state_error If there is no current polygon.
 * \throw state_error If there is no intersecting polygon.
 */
void PolygonsDoc::mergeCurPolygonWithOther()
{
	d->startAction();

	d->doAction(unique_ptr<Action>(new
		Act_MergePolygons(d->getIntersectingPolygonIdx(), d->curPolygonIdx())));
}



/// Intersect current polygon with other intersecting polygon.
/*!
 * \throw state_error If other action is in progress.
 * \throw state_error If there is no current polygon.
 * \throw state_error If there is no intersecting polygon.
 */
void PolygonsDoc::intersectCurPolygonWithOther()
{
	d->startAction();

	d->doAction(unique_ptr<Action>(new
		Act_IntersectPolygons(d->getIntersectingPolygonIdx(), d->curPolygonIdx())));
}


/// Subtract current polygon from other intersecting polygon.
/*!
 * \throw state_error If other action is in progress.
 * \throw state_error If there is no current polygon.
 * \throw state_error If there is no intersecting polygon.
 */
void PolygonsDoc::subtractCurPolygonFromOther()
{
	d->startAction();
	
	d->doAction(unique_ptr<Action>(new
		Act_SubtractPolygons(d->getIntersectingPolygonIdx(), d->curPolygonIdx())));
}


/// XOR current polygon with other intersecting polygon.
/*!
 * \throw state_error If other action is in progress.
 * \throw state_error If there is no current polygon.
 * \throw state_error If there is no intersecting polygon.
 */
void PolygonsDoc::xorCurPolygonWithOther()
{
	d->startAction();
	
	d->doAction(unique_ptr<Action>(new
		Act_XorPolygons(d->getIntersectingPolygonIdx(), d->curPolygonIdx())));
}


/// Partition other intersecting polygon by current polygon.
/*!
 * \throw state_error If other action is in progress.
 * \throw state_error If there is no current polygon.
 * \throw state_error If there is no intersecting polygon.
 */
void PolygonsDoc::partitionPolygonByCurPolygon()
{
	d->startAction();
	
	d->doAction(unique_ptr<Action>(new
		Act_PartitionPolygon(d->getIntersectingPolygonIdx(), d->curPolygonIdx())));
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Undo / redo


/// Tell if undo can be performed.
///
bool PolygonsDoc::canUndo() const
{
	return ! d->compositeActionLock &&
	       ! d->actionLog.empty();
}


/// Undo last action.
/*!
 * \throw state_error If other action is in progress.
 * \throw state_error If there is nothing to undo.
 */
void PolygonsDoc::undo()
{
	d->checkNoActionInProgress();
	if ( d->actionLog.empty() )
		throw state_error("Nothing to undo");
	
	d->checkActionLog();


	d->undoneActions.push_back(unique_ptr<Action>()); // SEG

	Action *const action = d->actionLog.back().get();
	
	action->uncommit(); // Cannot throw after checkActionLog()
	try {
		action->undo(d->polygons, d);
	}
	catch (...) {
		action->commit();
		d->undoneActions.pop_back();
		throw;
	}
	
	d->undoneActions.back() = move(d->actionLog.back()); // no throw
	
	d->actionLog.pop_back(); // no throw


	//TODO
	//d->restoreSelectionBefore(d->undoneActions.back().get());
}



/// Tell if redo can be performed.
///
bool PolygonsDoc::canRedo() const
{
	return ! d->compositeActionLock &&
	       ! d->undoneActions.empty();
}



/// Redo last undone action.
/*!
 * \throw state_error If other action is in progress.
 * \throw state_error If there is nothing to redo.
 */
void PolygonsDoc::redo()
{
	d->checkNoActionInProgress();
	if ( d->undoneActions.empty() )
		throw state_error("Nothing to redo");
	
	d->checkActionLog();


	d->actionLog.push_back(unique_ptr<Action>()); // SEG

	Action *const action = d->undoneActions.back().get();

	try {
		action->apply(d->polygons, d);
	}
	catch (...) {
		d->actionLog.pop_back();
		throw;
	}

	// Can throw only if apply() forgot to set Done flag
	action->commit();
	
	d->actionLog.back() = move(d->undoneActions.back()); // no throw

	d->undoneActions.pop_back(); // no throw


	//TODO
	//d->restoreSelectionAfter(d->actionLog.back().get());
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Composite actions


/// Place this at the beginning of methods in derived classes.
///
#define CUA_BEGIN_METHOD \
	checkNotFinished(); \
	try {


/// Place this at the end of methods in derived classes.
///
#define CUA_END_METHOD \
	}                                   \
	catch ( call_error const &/*e*/ ) { \
		throw;                            \
	}                                   \
	catch ( CMemoryException */*e*/ ) { \
		onMemoryEx();                     \
		throw;                            \
	}                                   \
	catch (...) {                       \
		onAnyEx();                        \
		throw;                            \
	}



/// CompositeUserAction implementation
///
class PolygonsDoc::CompositeUserActionImpl : virtual public PolygonsDoc::CompositeUserAction
{
protected:
	CompositeUserActionImpl(PolygonsDoc::Private *d, bool *modelLock);

	~CompositeUserActionImpl() override;


	/// Tell if to accept the action and commit it
	///
	virtual bool canCommit() const { return true; }

	// Overrides
	bool finish() override;
	void cancel() override;
	bool finished() const override { return isFinished; }


	void checkNotFinished() const;
	
	bool hasActionInLog() const;

	void pushActionToLog(unique_ptr<Action> action);
	void popActionFromLog();

	template <typename Act>
	Act * getLastAction() const;

	void doCancel();
	
	void onMemoryEx();
	void onAnyEx();

	void finalize();

//
protected:
	PolygonsDoc::Private *const d;
	bool *const modelLock;
	
	bool isFinished;

	bool _hasActionInLog;
};



PolygonsDoc::CompositeUserActionImpl::
CompositeUserActionImpl(PolygonsDoc::Private *d, bool *modelLock)
	: d(d)
	, modelLock(modelLock)
	, isFinished(false)
	, _hasActionInLog(false)
{
	ENSURE(d);

	ENSURE(modelLock);
	ENSURE(! *modelLock);
	*modelLock = true;
}


PolygonsDoc::CompositeUserActionImpl::~CompositeUserActionImpl()
{
	try {
		if ( ! finished() ) {
			TRACE(_T("CompositeUserActionImpl: not finished in destructor; cancelling\n"));
			
			cancel();
		}
	}
	catch (...) {
		TRACE(_T("CompositeUserActionImpl: exception during cancel in destructor\n"));

		finalize();
	}
}



bool PolygonsDoc::CompositeUserActionImpl::finish()
{
	//TRACE(__FUNCTION__"\n");

	CUA_BEGIN_METHOD

	if ( ! hasActionInLog() ) {
		finalize();
		return false;
	}

	if ( ! canCommit() ) {
		doCancel();
		return false;
	}

	d->commitLastAction();

	finalize();

	return true;

	CUA_END_METHOD
}



void PolygonsDoc::CompositeUserActionImpl::cancel()
{
	if ( isFinished )
		return;
	
	try {
		doCancel();
	CUA_END_METHOD
}


void PolygonsDoc::CompositeUserActionImpl::doCancel()
{
	if ( hasActionInLog() ) {
		ENSURE(d->actionLog.back()->done());
		d->actionLog.back()->undo(d->polygons, d);
		popActionFromLog(); // Does not throw
	}

	finalize();
}


/// Check that action is not finished
/*!
 * \throw state_error If finished.
 */
void PolygonsDoc::CompositeUserActionImpl::checkNotFinished() const
{
	if ( isFinished )
		throw state_error("Action is finished");
}


/// Tell if uncommited action is held in action log.
///
bool PolygonsDoc::CompositeUserActionImpl::hasActionInLog() const
{
	if ( ! _hasActionInLog )
		return false;
	ENSURE(! d->actionLog.empty());
	return true;
}


void PolygonsDoc::CompositeUserActionImpl::pushActionToLog(unique_ptr<Action> action)
{
	ENSURE(action != nullptr);
	ENSURE(! _hasActionInLog);
		
	d->actionLog.push_back(move(action));
	_hasActionInLog = true;
}


void PolygonsDoc::CompositeUserActionImpl::popActionFromLog()
{
	ENSURE(hasActionInLog());
		
	d->actionLog.pop_back();
	_hasActionInLog = false;
}


/// Get last action from log, casted to specific class Act.
///
template <typename Act>
Act * PolygonsDoc::CompositeUserActionImpl::getLastAction() const
{
	ENSURE( ! d->actionLog.empty() );

	auto *const lastAction = dynamic_cast<Act*>(d->actionLog.back().get());
	ENSURE( lastAction != nullptr );
	ENSURE( lastAction->done() && ! lastAction->committed() );

	return lastAction;
}


/// Called on memory exception
///
void PolygonsDoc::CompositeUserActionImpl::onMemoryEx()
{
	if ( hasActionInLog() ) {
		if ( ! d->actionLog.back()->done() )
			popActionFromLog();
			
		// Do not try to undo here, because it might involve allocation
	}

	finalize();
}


/// Called on any exception other from memory exception
///
void PolygonsDoc::CompositeUserActionImpl::onAnyEx()
{
	// Data structures might be corrupted, so don't try to do cleanup

	finalize();
}


/// Release action lock in model and enter finished state.
///
void PolygonsDoc::CompositeUserActionImpl::finalize()
{
	ASSERT(! isFinished);

	*modelLock = false;
	isFinished = true;
}



// CompositeUserAction overrides to suppress C4250
#define CUA_OVERRIDES \
	bool finish() override { return CompositeUserActionImpl::finish(); }           \
	void cancel() override { CompositeUserActionImpl::cancel(); }                  \
	bool finished() const override { return CompositeUserActionImpl::finished(); }

////////////////////////////////////////////////////////////////////////////////////////////////////


class PolygonsDoc::CreatePolygonActionImpl : public PolygonsDoc::CreatePolygonAction
                                           , public PolygonsDoc::CompositeUserActionImpl
{
public:
	CreatePolygonActionImpl(PolygonsDoc::Private *d, bool *modelLock)
		: CompositeUserActionImpl(d, modelLock)
	{}

private:
	// CreatePolygonAction overrides
	void addVertex(poly::Point const &pos) override;
	void moveLastVertex(poly::Point const &pos) override;
	bool hasVertex() const override { return hasActionInLog(); }

	// CompositeUserActionImpl overrides
	//bool canCommit() const override {
	//	ENSURE(d->hasCurPolygon());
	//	return d->curPolygonIt->isSimple();
	//}

	CUA_OVERRIDES
};



void PolygonsDoc::CreatePolygonActionImpl::addVertex(poly::Point const &pos)
{
	CUA_BEGIN_METHOD

	if ( ! hasActionInLog() ) {
		pushActionToLog(unique_ptr<Action>(
			new Act_AddPolygon(poly::Polygon(list<poly::Point>(1, pos))) ));
	}
	else {
		auto *const lastAction = getLastAction<Act_AddPolygon>();
		lastAction->undo(d->polygons, d);
		lastAction->polygon().addVertex(pos);
	}
	
	d->actionLog.back()->apply(d->polygons, d);

	CUA_END_METHOD
}



void PolygonsDoc::CreatePolygonActionImpl::moveLastVertex(poly::Point const &pos)
{
	if ( ! hasVertex() )
		throw state_error("No vertices");

	CUA_BEGIN_METHOD

	auto *const lastAction = getLastAction<Act_AddPolygon>();
		
	lastAction->undo(d->polygons, d);
	lastAction->polygon().back() = pos;
	lastAction->apply(d->polygons, d);

	CUA_END_METHOD
}



/// Start composite CreatePolygonAction
/*!
 * \throw state_error If other action is in progress.
 */
unique_ptr<PolygonsDoc::CreatePolygonAction> PolygonsDoc::startCreatePolygonAction()
{
	d->startAction();

	d->resetCurPolygon();
	
	return unique_ptr<PolygonsDoc::CreatePolygonAction>(
		new CreatePolygonActionImpl(d, &d->compositeActionLock));
}

////////////////////////////////////////////////////////////////////////////////////////////////////


/// Composite action of dragging the current polygon
///
class PolygonsDoc::CurPolygonDragAction : public PolygonsDoc::DragAction
                                        , public PolygonsDoc::CompositeUserActionImpl
{
public:
	CurPolygonDragAction(PolygonsDoc::Private *d, bool *modelLock,
	                     poly::Point const &anchorSrcPos)
		: CompositeUserActionImpl(d, modelLock)
		, anchorSrcPos(anchorSrcPos)
	{}

private:
	void step(poly::Point const &anchorDstPos) override;
	
	CUA_OVERRIDES

//
	poly::Point const anchorSrcPos;
};



void PolygonsDoc::CurPolygonDragAction::step(poly::Point const &anchorDstPos)
{
	CUA_BEGIN_METHOD

	poly::Vector const vector = anchorDstPos - anchorSrcPos;
		
	if ( ! hasActionInLog() ) {
		if ( vector == poly::Vector(0, 0) )
			return;

		pushActionToLog(unique_ptr<Action>(
			new Act_MovePolygon(d->curPolygonIdx(), vector) ));
	}
	else {
		auto *const lastAction = getLastAction<Act_MovePolygon>();

		//ASSERT_CATCH( lastAction->polygonIdx == d->curPolygonIdx() );

		if ( lastAction->vector == vector )
			return;
		
		lastAction->undo(d->polygons, d);

		if ( vector == poly::Vector(0, 0) ) {
			popActionFromLog();
			return;
		}

		lastAction->vector = vector;
	}
	
	d->actionLog.back()->apply(d->polygons, d);

	CUA_END_METHOD
}



/// Start current polygon dragging action.
/*!
 * \param anchorSrcPos  Position of arbitrary point called anchor. The anchor's position 
 *                      relative to polygon is invariant during dragging. Later position
 *                      of the anchor is updated via step().
 *
 * \throw state_error If other action is in progress.
 */
unique_ptr<PolygonsDoc::DragAction> PolygonsDoc::startCurPolygonDragAction(poly::Point const &anchorSrcPos)
{
	d->startAction();

	return unique_ptr<PolygonsDoc::DragAction>(
		new CurPolygonDragAction(d, &d->compositeActionLock, anchorSrcPos));
}

////////////////////////////////////////////////////////////////////////////////////////////////////


/// Composite action of dragging the current vertex of the current polygon.
//
class PolygonsDoc::CurVertexDragAction : public PolygonsDoc::DragAction
                                       , public PolygonsDoc::CompositeUserActionImpl
{
public:
	CurVertexDragAction(PolygonsDoc::Private *d, bool *modelLock,
	                    poly::Point const &anchorSrcPos)
		: CompositeUserActionImpl(d, modelLock)
		, anchorSrcPos(anchorSrcPos)
	{}

private:
	void step(poly::Point const &anchorDstPos) override;
	
	// CompositeUserActionImpl overrides
	//bool canCommit() const override {
	//	ENSURE(d->hasCurPolygon());
	//	return d->curPolygonIt->isSimple();
	//}

	CUA_OVERRIDES

//
	poly::Point const anchorSrcPos;
};



void PolygonsDoc::CurVertexDragAction::step(poly::Point const &anchorDstPos)
{
	CUA_BEGIN_METHOD

	poly::Vector const vector = anchorDstPos - anchorSrcPos;
		
	if ( ! hasActionInLog() ) {
		if ( vector == poly::Vector(0, 0) )
			return;

		pushActionToLog(unique_ptr<Action>(
			new Act_MoveVertex(d->curPolygonIdx(), d->curVertexIdx(), vector) ));
	}
	else {
		auto *const lastAction = getLastAction<Act_MoveVertex>();

		//ASSERT_CATCH( lastAction->polygonIdx == d->curPolygonIdx() );
		//ASSERT_CATCH( lastAction->vertexIdx == d->curVertexIdx() );

		if ( lastAction->vector == vector )
			return;
		
		lastAction->undo(d->polygons, d);

		if ( vector == poly::Vector(0, 0) ) {
			popActionFromLog();
			return;
		}

		lastAction->vector = vector;
	}
	
	d->actionLog.back()->apply(d->polygons, d);

	CUA_END_METHOD
}



/// Start current vertex dragging action.
/*!
 * \param anchorSrcPos  Position of arbitrary point called anchor. The anchor's position 
 *                      relative to vertex is invariant during dragging. Later position
 *                      of the anchor is updated via step().
 *
 * \throw state_error If other action is in progress.
 */
unique_ptr<PolygonsDoc::DragAction> PolygonsDoc::startCurVertexDragAction(poly::Point const &anchorSrcPos)
{
	d->startAction();

	return unique_ptr<PolygonsDoc::DragAction>(
		new CurVertexDragAction(d, &d->compositeActionLock, anchorSrcPos));
}

////////////////////////////////////////////////////////////////////////////////////////////////////


/// Composite action of adding new vertex to current polygon.
/*!
 * It exists because in UI user usually clicks on the edge where he wants new vertex and
 * then drags the vertex to desired position.
 */
class PolygonsDoc::CurPolygonAddVertexAction : public PolygonsDoc::DragAction
                                             , public PolygonsDoc::CompositeUserActionImpl
{
public:
	CurPolygonAddVertexAction(PolygonsDoc::Private *d, bool *modelLock,
	                          VerticesCIterator beforeVertex)
		: CompositeUserActionImpl(d, modelLock)
		, beforeVertex(beforeVertex)
	{}

private:
	void step(poly::Point const &pos) override;

	// CompositeUserActionImpl overrides
	//bool canCommit() const override {
	//	ENSURE(d->hasCurPolygon());
	//	return d->curPolygonIt->isSimple();
	//}

	CUA_OVERRIDES

//
	VerticesCIterator const beforeVertex;
};



void PolygonsDoc::CurPolygonAddVertexAction::step(poly::Point const &pos)
{
	CUA_BEGIN_METHOD

	if ( ! hasActionInLog() ) {
		pushActionToLog(unique_ptr<Action>(
			new Act_AddVertex(d->curPolygonIdx(), d->curPolygonVertexIdx(beforeVertex), pos) ));
	}
	else {
		auto *const lastAction = getLastAction<Act_AddVertex>();

		//ASSERT_CATCH( lastAction->polygonIdx == d->curPolygonIdx() );
		//ASSERT_CATCH( lastAction->vertexIdx == d->curVertexIdx() );
		//ASSERT_CATCH( lastAction->vertex == *d->curVertexIt );

		if ( lastAction->vertex == pos )
			return;
		
		lastAction->undo(d->polygons, d);

		lastAction->vertex = pos;
	}
	
	d->actionLog.back()->apply(d->polygons, d);

	CUA_END_METHOD
}



/// Start adding vertex to current polygon composite action.
/*!
 * \param beforeVertex  Iterator to vertex before which to insert new vertex.
 *
 * \throw state_error If other action is in progress.
 */
unique_ptr<PolygonsDoc::DragAction>
PolygonsDoc::startCurPolygonAddVertexAction(VerticesCIterator beforeVertex)
{
	d->startAction();

	return unique_ptr<PolygonsDoc::DragAction>(
		new CurPolygonAddVertexAction(d, &d->compositeActionLock, beforeVertex));
}



////////////////////////////////////////////////////////////////////////////////////////////////////

/*! Database persistence
 *  --------------------
 *
 * Databases are supported via MFC wrapper of ODBC. The generated class PointsRecordset is
 * used, binding DB columns to its members. This class is used for loading from DB, and
 * potentially can be used to saving to DB.
 */



// Handler for menu command Save in DB
//
void PolygonsDoc::OnFileSaveInDB()
{
	//TODO Error handling
	
	CDatabase db;
	
	// Opens data source selection (and creation) dialog
	if ( ! db.OpenEx(NULL) )
		return;

	//HACK: An exception is thrown if table does not exist yet. We simply discard it.
	//TODO: Check table existence properly. It's not possible via MFC though, must use plain ODBC API.
	try {
		db.ExecuteSQL(_T("DROP TABLE Points"));
	}
	catch ( CDBException *e ) {
		e->Delete();
	}
	
	db.ExecuteSQL(_T("CREATE TABLE Points(polygonIdx INTEGER, vertexIdx INTEGER, x DOUBLE, y DOUBLE,")
	                                 _T(" PRIMARY KEY(polygonIdx, vertexIdx))"));

	// Precision enough to recover original binary
	static UINT const doublePrecision = numeric_limits<double>::digits10 + 2;
	
	UINT polygonIdx = 0;
	for  ( auto const &polygon : d->polygons ) {
		UINT vertexIdx = 0;
		for ( poly::Point const &vertex: polygon ) {
			//TODO: Implement insertion using generated PointsRecordset class

			CString sql;
			sql.Format(_T("INSERT INTO Points VALUES (%u, %u, %.*f, %.*f)"),
			           polygonIdx, vertexIdx, doublePrecision, vertex.x, doublePrecision, vertex.y);

			db.ExecuteSQL(sql);

			++vertexIdx;
		}

		++polygonIdx;
	}

	db.Close();
}



// Load next polygon from recordset and add it to \c polygons.
//
static void loadPolygon(PointsRecordset &recset, list<poly::Polygon> &polygons)
{
	//TODO Error handling
	
	ASSERT( ! recset.IsEOF() );

	list<poly::Point> vertices;

	UINT const polygonIdx = recset.m_polygonIdx;

	do {
		vertices.emplace_back(recset.m_x, recset.m_y);

		recset.MoveNext();
	}
	while ( ! recset.IsEOF() && recset.m_polygonIdx == polygonIdx );

	poly::Polygon polygon(vertices);

	// check polygon validity

	polygons.push_back(move(polygon));
}



// Load domain state from database.
//
BOOL PolygonsDoc::Private::loadFromDB(CDatabase *db)
{
	//TODO Error handling
	
	PointsRecordset recset(db);

	recset.m_strSort = _T("polygonIdx, vertexIdx");

	recset.Open(AFX_DB_USE_DEFAULT_TYPE, NULL, CRecordset::readOnly);

	//TRACE(recset.GetSQL());

	ASSERT( recset.CanScroll() );

	while ( ! recset.IsEOF() )
		loadPolygon(recset, polygons);

	recset.Close();

	return TRUE;
}
