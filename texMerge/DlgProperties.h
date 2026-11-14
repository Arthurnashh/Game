#pragma once
#include "afxcmn.h"
#include <es/algorithms/qsort.hpp>




template<class Alocator>
class DlgPropertiesList: public AutoArray<TextureProp,Alocator>
{
public:
  class SortFunctor
  {
    const TextureProp *arr;
  public:
    SortFunctor(const TextureProp *arr):arr(arr) {}
    int operator ()(int *a, int *b) const
    {
      return stricmp(arr[*a].name,arr[*b].name);
    }
  };

  template<class Operation>
  void CompareWith(const Array<TextureProp>& other, const Operation &functor)
  {
    int *indx1=(int *)alloca(sizeof(int)*Size());
    int *indx2=(int *)alloca(sizeof(int)*other.Size());
    for (int i=0;i<Size();i++) indx1[i]=i;
    for (int i=0;i<other.Size();i++) indx2[i]=i;
    SortFunctor fnct1(Data());
    Array<int> arr1(indx1,Size());
    QSort<Array<int>,SortFunctor>(arr1,fnct1);
    SortFunctor fnct2(other.Data());
    Array<int> arr2(indx2,other.Size());
    QSort<Array<int>,SortFunctor>(arr2,fnct2);
    
    int pos1=0;
    int pos2=0;
    while (pos1<Size() || pos2<other.Size())
    {
      if (pos1>=Size() || (pos2<other.Size() && stricmp((*this)[indx1[pos1]].name,other[indx2[pos2]].name)>0))
      {functor.Added(other[indx2[pos2]]);pos2++;}
      else if (pos2>=other.Size() || (pos1<Size() && stricmp((*this)[indx1[pos1]].name,other[indx2[pos2]].name)<0))
      {functor.Removed((*this)[indx1[pos1]]);pos1++;}
      else 
      {      
        if ((*this)[indx1[pos1]].value!=other[indx2[pos2]].value)
          functor.Changed(other[indx2[pos2]]);
        pos1++;
        pos2++;
      }        
    }    
  }

  /// assignment for base class arrays
  void operator = ( const Array<TextureProp> &src )
  {
    AutoArray<TextureProp,Alocator>::operator =(src);
  }
};

class DlgProperties : public CDialog
{

  int _itemEdit;
  int _itemPosEdit;
  CEdit *_editControl;
  bool _newVal;  
  

  DECLARE_DYNAMIC(DlgProperties)

  DlgPropertiesList<MemAllocD> _properties;  
  
public:
	DlgProperties(CWnd* pParent = NULL);   // standard constructor
	virtual ~DlgProperties();

// Dialog Data
	enum { IDD = IDD_ITEMPROPR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
  CListCtrl wList;

  void SetValues(const Array<TextureProp> &prop)
  {
    _properties=prop;
  }
  const Array<TextureProp> &GetValues() 
  {
    return _properties;
  }
protected:
  virtual BOOL OnInitDialog();
  afx_msg void OnLvnBeginlabeleditProplist(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnEndValueEdit();
  virtual BOOL PreTranslateMessage(MSG* pMsg);

  void StartEdit(int item, int subItem);

public:
  afx_msg void OnBnClickedEditname();
  afx_msg void OnBnClickedEditvalue();
  afx_msg void OnBnClickedPickafile();
  afx_msg void OnBnClickedPickafolder();
  afx_msg void OnBnClickedAdd();
  afx_msg void OnBnClickedDelete();
protected:
  virtual void OnOK();
};
