// smpackageDlg.h : header file
//

#if !defined(AFX_SMPACKAGEDLG_H__E60DA2DD_D8D6_42BC_9049_631EAFBAA1F6__INCLUDED_)
#define AFX_SMPACKAGEDLG_H__E60DA2DD_D8D6_42BC_9049_631EAFBAA1F6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CSmpackageDlg dialog


class CSmpackageDlg : public CDialog
{
// Construction
public:
	CSmpackageDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CSmpackageDlg)
	enum { IDD = IDD_SMPACKAGE_DIALOG };
	CButton	m_buttonExportAsOne;
	CButton	m_buttonExportAsIndividual;
	CListBox	m_listBox;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSmpackageDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;


	// Generated message map functions
	//{{AFX_MSG(CSmpackageDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSelchangeList();
	afx_msg void OnButtonExportAsOne();
	afx_msg void OnButtonExportAsIndividual();
	afx_msg void OnButtonPlay();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SMPACKAGEDLG_H__E60DA2DD_D8D6_42BC_9049_631EAFBAA1F6__INCLUDED_)
