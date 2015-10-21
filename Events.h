#pragma once

#include <vector>
#include <algorithm>



/// Important event of change in domain state.
//
class Event
{
public:
	enum Object { Polygon, Vertex } object;
	enum Action { Added, Deleted } action;
	UINT polygonIdx;
	UINT vertexIdx;   ///< Used only if object == Polygon.

//
	Event(Object object, Action action, UINT polygonIdx, UINT vertexIdx = UINT_MAX)
		: object(object), action(action), polygonIdx(polygonIdx), vertexIdx(vertexIdx)
	{
		ENSURE(object == Polygon && vertexIdx == UINT_MAX  ||
		       object == Vertex && vertexIdx != UINT_MAX);
	}
};



/// List of events occured during one action.
/*!
 * The main purpose of this class is to wrap \c vector<Event> to allow moving with \c noexcept
 * guarantee, which helps in implementing strong exception guarantee of actions.
 *
 * The secondary purpose is to provide convenience functions for list of events.
 */
class EventList
{
public:
	EventList() _NOEXCEPT {}
	EventList(std::vector<Event> &&events) _NOEXCEPT { this->events.swap(events); }
	EventList(EventList &&r) _NOEXCEPT { events.swap(r.events); }


	/// Get number of added polygons.
	//
	UINT numAddedPolygons() const {
		return std::count_if(events.begin(), events.end(), [](Event const &e){ return
		                     e.object == Event::Polygon && e.action == Event::Added;});
	}

	/// Test if some polygons were deleted.
	//
	bool polygonsDeleted() const {
		return std::find_if(events.begin(), events.end(), [](Event const &e){ return
			                  e.object == Event::Polygon && e.action == Event::Deleted;}) != events.end();
	}

	/// Test if given polygon was deleted.
	//
	bool polygonDeleted(UINT idx) const {
		return std::find_if(events.begin(), events.end(), [idx](Event const &e){ return
		           e.object == Event::Polygon && e.action == Event::Deleted && e.polygonIdx == idx;})
		       != events.end();
	}

	/// Test if list consists of single added vertex event.
	//
	bool vertexAdded() const {
		return events.size() == 1 &&
		       events.front().object == Event::Vertex && events.front().action == Event::Added;
	}

	/// Test if list consists of single deleted vertex event with specified vertex.
	//
	bool vertexDeleted(UINT polygonIdx, UINT vertexIdx) const {
		return events.size() == 1 &&
		       events.front().object == Event::Vertex &&
		       events.front().action == Event::Deleted &&
		       events.front().polygonIdx == polygonIdx && events.front().vertexIdx == vertexIdx;
	}

	/// Get first added polygon event.
	/*!
	 * \pre At least one added polygon event must exist.
	 */
	Event const & getPolygonAddedEvent() const {
		auto it = std::find_if(events.begin(), events.end(), [](Event const &e){ return
		                       e.object == Event::Polygon && e.action == Event::Added;});
		return *it;
	}

	/// Get added vertex event.
	/*!
	 * \pre vertexAdded() == true.
	 */
	Event const & getVertexAddedEvent() const {
		auto it = std::find_if(events.begin(), events.end(), [](Event const &e){ return
		                       e.object == Event::Vertex && e.action == Event::Added;});
		return *it;
	}

	/// Get deleted vertex event.
	/*!
	 * \pre List consists of single deleted vertex event.
	 */
	Event const & getVertexDeletedEvent() const {
		auto it = std::find_if(events.begin(), events.end(), [](Event const &e){ return
		                       e.object == Event::Vertex && e.action == Event::Deleted;});
		return *it;
	}

private:
	std::vector<Event> events;
};
