#pragma once



/// Interface of a pane displaying status message.
//
class IStatusPane
{
public:
	/// Set status message.
	/*!
	 * \param id  String ID.
	 */
	virtual void setStatusMessage(UINT id) = 0;
	
	/// Reset status message to default.
	//
	virtual void resetStatusMessage() = 0;

protected:
	~IStatusPane() {}
};
