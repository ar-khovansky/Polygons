#pragma once

#include "Events.h"



/*! Interface of presentation model, allowing to be notified about events by
 * application model.
 *
 * This is a conceptual demonstration. Actual interface would be very diferent.
 */
class PresentationModel
{
public:
	virtual void notify(EventList const &events) = 0;
};
