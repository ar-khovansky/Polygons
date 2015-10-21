#include "stdafx.h"
#include "Actions.h"

#include "PresentationModel.h"


using namespace std;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif



void Action::apply(list<poly::Polygon> &polygons, PresentationModel *presentationModel)
{
	ENSURE(! done() & ! committed());
		
	EventList const events = apply(polygons);
		
	flags |= Done;

	try {
		presentationModel->notify(events);
	}
	catch (...) {
		ENSURE(0);
	}
}


void Action::undo(list<poly::Polygon> &polygons, PresentationModel *presentationModel)
{
	ENSURE(done() & ! committed());
		
	EventList const events = undo(polygons);
		
	flags &= ~Done;

	try {
		presentationModel->notify(events);
	}
	catch (...) {
		ENSURE(0);
	}
}


void Action::commit()
{
	ENSURE(done() && ! committed());

	flags |= Committed;
}


void Action::uncommit()
{
	ENSURE(done() && committed());

	flags &= ~Committed;
}
