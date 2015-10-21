#pragma once

#include "PolygonsDoc.h"
#include "PresentationModel.h"
#include "Actions.h"

#include <deque>
#include <memory>


class EventList;



class PolygonsDoc::Private : public PresentationModel
{
	friend class PolygonsDoc;

//
	Private(PolygonsDoc *doc);

	BOOL loadFromDB(CDatabase *db);

	// Internal analogs of front-end functions
	
	bool hasCurPolygon() const
		{ return curPolygonIt != polygons.end(); }
	bool hasCurVertex() const
		{ return hasCurPolygon() && curVertexIt != curPolygonIt->end(); }

	// Pure operations
	void doSetCurPolygon(PolygonsCIterator polygonIt);
	void doSetCurVertex(VerticesCIterator vertexIt);
	void doResetCurPolygon();
	void doResetCurVertex();
	void doSetCurPolygonAndVertex(PolygonsCIterator polygonIt,
                                VerticesCIterator vertexIt);

	// Internal - does not check action in progress
	void setCurPolygon(PolygonsCIterator polygonIt);
	void resetCurPolygon();
	void setCurVertex(VerticesCIterator it);
	void resetCurVertex();
	void setCurPolygonAndVertex(PolygonsCIterator polygonIt,
	                            VerticesCIterator vertexIt);


	UINT polygonIdx(PolygonsCIterator it) const
		{ return distance(polygons.begin(), it); }
	UINT curPolygonIdx() const;
	UINT curVertexIdx() const;
	UINT curPolygonVertexIdx(VerticesCIterator vertexIt) const;

	void doSetCurPolygon(UINT idx);
	void doSetCurPolygonAndVertex(UINT polygonIdx, UINT vertexIdx);

	void recalcCurIndices();

	std::vector<PolygonsCIterator> getPolygonsIntersectingWithCur() const;
	UINT getIntersectingPolygonIdx() const;

	void checkCurPolygon() const;
	void checkCurVertex() const;
	void checkNoActionInProgress() const;
	void checkActionLog() const;
	void startAction() const;
	
	void doAction(std::unique_ptr<Action> action);
	
	void commitLastAction();

	// PresentationModel override
	void notify(EventList const &events) override;

	void restoreSelectionBefore(Action const *action);
	void restoreSelectionAfter (Action const *action);

// Fields

	PolygonsDoc *const doc;

	// Domain state

	std::list<poly::Polygon> polygons;

	// Application layer state
	
	std::deque<std::unique_ptr<Action>> actionLog;
	// Undone actions are hold in separate container to allow rollable-back actions in main log.
	std::deque<std::unique_ptr<Action>> undoneActions;

	// If true, this means that composite action is in progress, so no any other action can go.
	// It is manipulated only by composite actions.
	// It must be checked before each action.
	bool compositeActionLock;

	// Presentation state
	
	PolygonsIterator curPolygonIt;
	VerticesCIterator curVertexIt;
	// Indices are needed for handling event notifications because iterator can be invalidated.
	UINT _curPolygonIdx;
	UINT _curVertexIdx;
};
