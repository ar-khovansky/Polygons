#pragma once

#include "Events.h"

#include "Poly/Poly.h"

#include <list>



class PresentationModel;



/// Base for internal representation of all domain actions.
/*!
 * Internally all domain actions are represented by objects. This allows to maintain their
 * history and implement undo/redo/rollback.
 * 
 * An action is initialized with information needed to perform required changes to the
 * current state: objects or their indices, coordinates etc. When applied, action can store
 * additional information if it is needed to restore original state (e.g. deleted object).
 *
 * The system is robust, particularly, exception-safe. If, for example, memory exception
 * occurs during <e>composite action</e>, the state and its history remain valid, but possibly
 * in a transition state. Later, when memory conditions normalize, operation can be rolled
 * back by the engine (the latter is not implemented though). Simple (non-composite) actions
 * have no problems at all - they either complete successfully or fail without changing domain
 * state and history. (Presentation state can change though, for example selection can be
 * reset.)
 *
 * Exception safety is achieved by two things:
 * - all apply/undo methods have strong exception guarantee;
 * - using action state flags: Done, Committed.
 * Also the poly::Polygon has \c noexcept specification on move constructor and move asignment
 * operator, which helps a lot.
 *
 * The \c Committed flag is needed to implement composite actions. A composite action is
 * committed when accepted by application logic. Uncommitted action can be rolled back after
 * exception.
 *
 * Composite actions are not based on this class, because they turn into simple actions
 * after commit anyway. So they use the following strategy: place corresponding
 * simple action into history queue and undo-modify-apply it on each update.
 *
 * Actions belong to application logic layer. They notify presentation model about important
 * events (addition/deletion of objects) via \c PresentationModel interface.
 *
 * Actions use polygon and vertex indices instead of iterators because the latter can be
 * invalidated.
 *
 * Potentially, the action history can be serialized along with domain state.
 */
class Action
{
public:
	enum Flags {
		/// Action is applied to the state, i.e. the state contains corresponding changes.
		Done = 0x1,
		/// Application layer has accepted the action.
		Committed = 0x2
	};

//
	Action() : flags(0) {}

	virtual ~Action() {}

	
	///@{
	/// Apply / undo the action.
	/*!
	 * Each function modifies domain state and then notifies presentation model about important
	 * events occured. Important events are addition or deletion of objects.
	 *
	 * \param polygons           The domain state.
	 * \param presentationModel  Presentation model.
	 *
	 * Strong exception safety.
	 */
	void apply(std::list<poly::Polygon> &polygons, PresentationModel *presentationModel);
	void undo (std::list<poly::Polygon> &polygons, PresentationModel *presentationModel);
	///@}

	///@{
	/// Set/reset the Committed flag.
	///
	void commit();
	void uncommit();
	///@}

	/// Test if Done is set
	bool done() const { return (flags & Done) != 0; }
	/// Test if Committed is set
	bool committed() const { return (flags & Committed) != 0; }

protected:
	///@{
	/// Apply action to / undo from the domain state.
	/*!
	 * \param polygons  The domain state.
	 *
	 * \return List of important events (changes) in domain state.
	 *         Important events are addition or deletion of objects.
	 *
	 * Must provide strong exception guarantee.
	 */
	virtual EventList apply(std::list<poly::Polygon> &polygons) = 0;
	virtual EventList undo (std::list<poly::Polygon> &polygons) = 0;
	///@}

//
	UINT flags;
};
