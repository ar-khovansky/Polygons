#include "stdafx.h"
#include "PolygonsDoc_Private.h"

#include "Resource.h"
#include "Polygons.h"
#include "PointsRecordset.h"
#include "Exceptions.h"
#include "IteratorByIdx.h"

#include "Lib/Iterators.h"


using namespace std;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif



PolygonsDoc::Private::Private(PolygonsDoc *doc)
	: doc(doc)
	, compositeActionLock(false)
	, curPolygonIt(polygons.end())
	, _curPolygonIdx(UINT_MAX)
	, _curVertexIdx(UINT_MAX)
{}



/// Set current polygon - pure operation
//
void PolygonsDoc::Private::doSetCurPolygon(PolygonsCIterator polygonIt)
{
	curPolygonIt = remove_constness(polygons, polygonIt);
	
	if ( hasCurPolygon() )   // Make curVertexIt valid
		curVertexIt = curPolygonIt->end();

	recalcCurIndices();

	ASSERT(! hasCurVertex());
}


/// Reset current polygon - pure operation
//
void PolygonsDoc::Private::doResetCurPolygon()
{
	curPolygonIt = polygons.end();
	_curPolygonIdx = UINT_MAX;

	ASSERT( ! hasCurPolygon() );
}


/// Set current vertex - pure operation
/*!
 * \pre Current polygon must exist.
 */
void PolygonsDoc::Private::doSetCurVertex(VerticesCIterator it)
{
	ENSURE(hasCurPolygon());

	curVertexIt = it;
	_curVertexIdx = distance(VerticesCIterator(curPolygonIt->begin()), curVertexIt);

	ASSERT(hasCurVertex());
}


/// Reset current vertex - pure operation
/*!
 * \pre Current polygon must exist.
 */
void PolygonsDoc::Private::doResetCurVertex()
{
	ENSURE(hasCurPolygon());

	curVertexIt = curPolygonIt->end();
	_curVertexIdx = UINT_MAX;

	ASSERT(! hasCurVertex());
}


/// Set current polygon and vertex - pure operation
//
void PolygonsDoc::Private::doSetCurPolygonAndVertex(PolygonsCIterator polygonIt,
                                                    VerticesCIterator vertexIt)
{
	curPolygonIt = remove_constness(polygons, polygonIt);
	_curPolygonIdx = distance(polygons.begin(), curPolygonIt);
	curVertexIt = vertexIt;
	_curVertexIdx = distance(VerticesCIterator(curPolygonIt->begin()), curVertexIt);
}



/// Recalculate indices of current objects.
//
void PolygonsDoc::Private::recalcCurIndices()
{
	if ( hasCurPolygon() ) {
		_curPolygonIdx = distance(polygons.begin(), curPolygonIt);
		_curVertexIdx = hasCurVertex() ? distance(VerticesCIterator(curPolygonIt->begin()), curVertexIt)
		                               : UINT_MAX;
	}
	else
		_curPolygonIdx = UINT_MAX;
}



/// Set current polygon - pure operation
//
void PolygonsDoc::Private::doSetCurPolygon(UINT idx)
{ doSetCurPolygon(polygonIteratorByIdx(polygons, idx)); }


/// Set current polygon and vertex - pure operation
//
void PolygonsDoc::Private::doSetCurPolygonAndVertex(UINT polygonIdx, UINT vertexIdx)
{
	auto const polygonIt = polygonIteratorByIdx(polygons, polygonIdx);
	setCurPolygonAndVertex(polygonIt, vertexIteratorByIdx(*polygonIt, vertexIdx));
}



/// Get index current polygon.
/*!
 * \ throw state_error If no current polygon.
 */
UINT PolygonsDoc::Private::curPolygonIdx() const
{
	checkCurPolygon();

	return polygonIdx(curPolygonIt);
}


/// Get index current vertex.
/*!
 * \ throw state_error If no current vertex.
 */
UINT PolygonsDoc::Private::curVertexIdx() const
{
	checkCurVertex();

	return curPolygonVertexIdx(curVertexIt);
}


/// Get index of vertex in current polygon.
/*!
 * \throw state_error If no current polygon.
 */
UINT PolygonsDoc::Private::curPolygonVertexIdx(VerticesCIterator vertexIt) const
{
	checkCurPolygon();
	
	return distance(VerticesCIterator(curPolygonIt->begin()), vertexIt);
}



// Internal - does not check action in progress
//
void PolygonsDoc::Private::setCurPolygon(PolygonsCIterator polygonIt)
{
	if ( curPolygonIt == polygonIt )
		return;
	
	doSetCurPolygon(polygonIt);

	doc->UpdateAllViews(NULL);
}


// Internal - does not check action in progress
//
void PolygonsDoc::Private::resetCurPolygon()
{
	if ( ! hasCurPolygon() )
		return;
	
	doResetCurPolygon();
	
	doc->UpdateAllViews(NULL);
}


// Internal - does not check action in progress
/*!
 * \throw state_error If no current polygon.
 */
void PolygonsDoc::Private::setCurVertex(VerticesCIterator it)
{
	if ( ! hasCurPolygon() )
		throw state_error("No current polygon");

	if ( curVertexIt == it )
		return;
	
	doSetCurVertex(it);

	doc->UpdateAllViews(NULL);
}


// Internal - does not check action in progress
//
void PolygonsDoc::Private::resetCurVertex()
{
	if ( ! hasCurPolygon() )
		return;   // curVertexIt is not valid anyway

	if ( curVertexIt == curPolygonIt->end() )
		return;
	
	doResetCurVertex();

	doc->UpdateAllViews(NULL);
}


// Internal - does not check action in progress
//
void PolygonsDoc::Private::setCurPolygonAndVertex(PolygonsCIterator polygonIt,
	                                                VerticesCIterator vertexIt)
{
	if ( curPolygonIt == polygonIt && curVertexIt == vertexIt )
		return;
	
	doSetCurPolygonAndVertex(polygonIt, vertexIt);

	doc->UpdateAllViews(NULL);
}



/// Get polygons intersecting with current polygon.
/*!
 * \throw state_error If no current polygon.
 */
vector<PolygonsDoc::PolygonsCIterator> PolygonsDoc::Private::getPolygonsIntersectingWithCur() const
{
	checkCurPolygon();


	vector<PolygonsCIterator> rv;
	
	for ( auto it = polygons.begin(); it != polygons.end(); ++it ) {
		if ( it == curPolygonIt )
			continue;

		if ( poly::intersects(*curPolygonIt, *it) )
			rv.push_back(it);
	}

	return rv;
}



/// Get single polygon intersecting with current polygon.
/*!
 * \throw state_error If no current polygon.
 * \throw state_error If there is no intersection.
 * \throw state_error If current polygon intersects with several other.
 */
UINT PolygonsDoc::Private::getIntersectingPolygonIdx() const
{
	auto const isectPolygons = getPolygonsIntersectingWithCur();
	
	if ( isectPolygons.size() == 0 )
		throw state_error("No intersection");
	if ( isectPolygons.size() > 1 )
		throw state_error("Several intersections");
	
	ASSERT(isectPolygons.size() == 1);
	return polygonIdx(isectPolygons.front());
}



/// Check that current polygon exists.
/*!
 * \throw state_error If no current polygon.
 */
void PolygonsDoc::Private::checkCurPolygon() const
{
	if ( ! hasCurPolygon() )
		throw state_error("No current polygon");
}


/// Check that current vertex exists.
/*!
 * \throw state_error If no current vertex.
 */
void PolygonsDoc::Private::checkCurVertex() const
{
	if ( ! hasCurVertex() )
		throw state_error("No current vertex");
}


/// Check that no composite action is in progress.
/*!
 * \throw state_error If there is action in progress.
 */
void PolygonsDoc::Private::checkNoActionInProgress() const
{
	if ( compositeActionLock )
		throw state_error("Action in progress");
}


/// Check that action log is in committed state.
/*!
 * \throw state_error If action log is not in committed state.
 */
void PolygonsDoc::Private::checkActionLog() const
{
	if ( actionLog.empty() )
		return;

	ENSURE( actionLog.back()->done() && actionLog.back()->committed() );
}


/// Perform checks necessary at the beginning of action.
/*!
 * \throw state_error If there is action in progress.
 * \throw state_error If action log is not in committed state.
 */
void PolygonsDoc::Private::startAction() const
{
	checkNoActionInProgress();
	checkActionLog();
}



/// Perform given action from start checks to commit.
/*!
 * Strong exception guarantee.
 */
void PolygonsDoc::Private::doAction(unique_ptr<Action> action)
{
	ENSURE(action != nullptr); // Action can come only from internal code, so use ENSURE
	
	startAction();

	actionLog.push_back(move(action));
	
	try {
		actionLog.back()->apply(polygons, this);
	}
	catch (...) {
		actionLog.pop_back(); // Does not throw
		throw;
	}

	commitLastAction(); // Cannot throw in current case
}



/// Commit last action
//
void PolygonsDoc::Private::commitLastAction()
{
	ENSURE(! actionLog.empty());

	undoneActions.clear(); // no throw
	
	actionLog.back()->commit();
}



// PresentationModel override
//
void PolygonsDoc::Private::notify(EventList const &events)
{
	// Reset selection if selected object is deleted
	if ( _curPolygonIdx != UINT_MAX && events.polygonDeleted(_curPolygonIdx) )
		doResetCurPolygon();
	else if ( _curPolygonIdx != UINT_MAX && _curVertexIdx != UINT_MAX &&
	          events.vertexDeleted(_curPolygonIdx, _curVertexIdx) )
		doResetCurVertex();

	// Select added object
	if ( events.numAddedPolygons() == 1 )
		doSetCurPolygon(events.getPolygonAddedEvent().polygonIdx);
	else if ( events.vertexAdded() ) {
		Event const &event = events.getVertexAddedEvent();
		doSetCurPolygonAndVertex(event.polygonIdx, event.vertexIdx);
	}

	recalcCurIndices();


	doc->SetModifiedFlag();
	doc->UpdateAllViews(NULL);
}



/// Try restore selection state that was immediately before given action.
/*! Selection state is not stored in history, so it only can be deduced from the assumption
 *  that user operates mostly on selected objects. There are actions for which it is not the
 *  case, e.g. creating new objects - any previous selection is reset on creation.
 *
 * \todo If cannot deduce, check state sfter previous action.
 * \todo Always use "after" state?
 */
//void PolygonsDoc::Private::restoreSelectionBefore(Action const *action)
//{
//	{
//		auto const *act = dynamic_cast<Act_AddPolygon const *>(action);
//		if ( act ) {
//			// Cannot deduce
//			resetCurPolygon();
//			return;
//		}
//	}
//	{
//		auto const *act = dynamic_cast<Act_DeletePolygon const *>(action);
//		if ( act ) {
//			setCurPolygon(act->polygonIdx);
//			return;
//		}
//	}
//	{
//		auto const *act = dynamic_cast<Act_MovePolygon const *>(action);
//		if ( act ) {
//			setCurPolygon(act->polygonIdx);
//			return;
//		}
//	}
//	{
//		auto const *act = dynamic_cast<Act_AddVertex const *>(action);
//		if ( act ) {
//			// Only polygon is known
//			setCurPolygon(act->polygonIdx);
//			return;
//		}
//	}
//	{
//		auto const *act = dynamic_cast<Act_DeleteVertex const *>(action);
//		if ( act ) {
//			setCurPolygonAndVertex(act->polygonIdx, act->vertexIdx);
//			return;
//		}
//	}
//	{
//		auto const *act = dynamic_cast<Act_MoveVertex const *>(action);
//		if ( act ) {
//			setCurPolygonAndVertex(act->polygonIdx, act->vertexIdx);
//			return;
//		}
//	}
//	{
//		auto const *act = dynamic_cast<BooleanOperation const *>(action);
//		if ( act ) {
//			setCurPolygon(act->p2Idx);
//			return;
//		}
//	}
//
//	ASSERT(0);
//}



/// Restore selection state that was immediately after given action.
/*! Unlike restoreSelectionBefore, this is always possible.
 */
//void PolygonsDoc::Private::restoreSelectionAfter(Action const *action)
//{
//	{
//		auto const *act = dynamic_cast<Act_AddPolygon const *>(action);
//		if ( act ) {
//			setCurPolygon(--polygons.end());
//			return;
//		}
//	}
//	{
//		auto const *act = dynamic_cast<Act_DeletePolygon const *>(action);
//		if ( act ) {
//			resetCurPolygon();
//			return;
//		}
//	}
//	{
//		auto const *act = dynamic_cast<Act_MovePolygon const *>(action);
//		if ( act ) {
//			setCurPolygon(act->polygonIdx);
//			return;
//		}
//	}
//	{
//		auto const *act = dynamic_cast<Act_AddVertex const *>(action);
//		if ( act ) {
//			setCurPolygonAndVertex(act->polygonIdx, act->vertexIdx);
//			return;
//		}
//	}
//	{
//		auto const *act = dynamic_cast<Act_DeleteVertex const *>(action);
//		if ( act ) {
//			setCurPolygon(act->polygonIdx);
//			return;
//		}
//	}
//	{
//		auto const *act = dynamic_cast<Act_MoveVertex const *>(action);
//		if ( act ) {
//			setCurPolygonAndVertex(act->polygonIdx, act->vertexIdx);
//			return;
//		}
//	}
//	{
//		auto const *act = dynamic_cast<BooleanOperation const *>(action);
//		if ( act ) {
//			if ( act->preservePolygon2 )
//				setCurPolygon(act->p2Idx);
//			else
//				resetCurPolygon();
//
//			return;
//		}
//	}
//
//	ASSERT(0);
//}
