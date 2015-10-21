#include "stdafx.h"
#include "PointsRecordset.h"


IMPLEMENT_DYNAMIC(PointsRecordset, CRecordset)


PointsRecordset::PointsRecordset(CDatabase* pdb)
	: CRecordset(pdb)
{
	m_polygonIdx = 0;
	m_vertexIdx = 0;
	m_x = 0.0;
	m_y = 0.0;
	m_nFields = 4;
	m_nDefaultType = snapshot;
}

CString PointsRecordset::GetDefaultSQL()
{
	return _T("[Points]");
}

void PointsRecordset::DoFieldExchange(CFieldExchange* pFX)
{
	pFX->SetFieldType(CFieldExchange::outputColumn);
// Macros such as RFX_Text() and RFX_Int() are dependent on the
// type of the member variable, not the type of the field in the database.
// ODBC will try to automatically convert the column value to the requested type
	RFX_Long(pFX, _T("[polygonIdx]"), m_polygonIdx);
	RFX_Long(pFX, _T("[vertexIdx]"), m_vertexIdx);
	RFX_Double(pFX, _T("[x]"), m_x);
	RFX_Double(pFX, _T("[y]"), m_y);

}
/////////////////////////////////////////////////////////////////////////////
// PointsRecordset diagnostics

#ifdef _DEBUG
void PointsRecordset::AssertValid() const
{
	CRecordset::AssertValid();
}

void PointsRecordset::Dump(CDumpContext& dc) const
{
	CRecordset::Dump(dc);
}
#endif //_DEBUG


