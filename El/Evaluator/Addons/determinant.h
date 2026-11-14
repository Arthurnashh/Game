#pragma once 

namespace ObjektivLib {


template<int maxCols>
struct DeterminantSubResult
{
  static const int maxColLongs=(maxCols+31)/32;
  unsigned long cols[maxColLongs];
  int line;
  double subResult;
#ifdef _DEBUG
  __int64 requests;
#endif
public:
  DeterminantSubResult(bool init=false) {if (init) {memset(&cols,0,sizeof(cols));line=0;subResult=0;}}
  int Compare(const DeterminantSubResult<maxCols>& other) const
  {
    if (line>other.line) return 1;
    if (line<other.line) return -1;
    for (int i=0;i<maxColLongs;i++)
    {
      if (cols[i]>other.cols[i]) return 1;
      if (cols[i]<other.cols[i]) return -1;
    }
    return 0;
  }
  void SetLine(int line) {this->line=line;}
  void UnMaskCol(int col) {this->cols[col/32] &= ~(1<<(col & 31));}
  void MaskCol(int col) {this->cols[col/32] |= (1<<(col & 31));}
  int GetLine() const {return this->line;}
  bool IsMasked(int col) const {return (this->cols[col/32] & (1<<(col & 31)))!=0;}

  bool operator==(const DeterminantSubResult<maxCols> &other) const {return Compare(other)==0;}
  bool operator>=(const DeterminantSubResult<maxCols> &other) const {return Compare(other)>=0;}
  bool operator<=(const DeterminantSubResult<maxCols> &other) const {return Compare(other)<=0;}
  bool operator!=(const DeterminantSubResult<maxCols> &other) const {return Compare(other)!=0;}
  bool operator>(const DeterminantSubResult<maxCols> &other) const {return Compare(other)>0;}
  bool operator<(const DeterminantSubResult<maxCols> &other) const {return Compare(other)<0;}
  int operator=(int zero) {return zero;}
  double operator=(double res) {subResult=res;return res;}
  operator double() const {return subResult;}
};

#ifdef _DEBUG
static __int64 cacheMiss=0,cacheHit=0;
#endif

template<int maxCols, class Type>
void GetDeterminantOfPartMatrix(BTree<DeterminantSubResult<maxCols> >&cache, Type *matrix, int mxdim, DeterminantSubResult<maxCols> &subResult)
{
  bool swap=false;
  double sum=0;  
  int curline=subResult.GetLine();
  if (curline+1==mxdim)
  {
    for (int i=0;i<mxdim;i++) if (!subResult.IsMasked(i))
    {
      subResult=matrix[curline*mxdim+i];
      return;
    }

  }
  else
  {
    subResult.SetLine(curline+1);
    for (int i=0;i<mxdim;i++) if (!subResult.IsMasked(i))
    {
      double acc=matrix[curline*mxdim+i];
      if (acc!=0)
      {
        if (swap) acc=-acc;
        subResult.MaskCol(i);
        DeterminantSubResult<maxCols> *cached=cache.Find(subResult);
        if (cached==0)
        {
#ifdef _DEBUG
          cacheMiss++;
          __int64 reqsave=subResult.requests;
          subResult.requests=1;
#endif
          GetDeterminantOfPartMatrix(cache,matrix,mxdim,subResult);
          cache.Add(subResult);
          cached=&subResult;
#ifdef _DEBUG
          subResult.requests+=reqsave;
#endif
        }
        else
        {
#ifdef _DEBUG
          cacheHit+=cached->requests;
          subResult.requests+=cached->requests;
#endif
        }
        subResult.UnMaskCol(i);
        acc*=*cached;
      }
      swap=!swap;
      sum+=acc;
    }
    subResult=sum;
    subResult.SetLine(curline);
  }
}
 
template<class Type>
static double Determinant(Type *mx, int mxdim)
{
  if (mxdim<=32)
  {
    BTree<DeterminantSubResult<32> >cache;
    DeterminantSubResult<32> subResult(true);
    GetDeterminantOfPartMatrix<32>(cache,mx,mxdim,subResult);
    return subResult;
  }
  else if (mxdim<=64)
  {
    BTree<DeterminantSubResult<64> >cache;
    DeterminantSubResult<64> subResult(true);
    GetDeterminantOfPartMatrix<64>(cache,mx,mxdim,subResult);
    return subResult;
  }
  else if (mxdim<=128)
  {
    BTree<DeterminantSubResult<128> >cache;
    DeterminantSubResult<128> subResult(true);
    GetDeterminantOfPartMatrix<128>(cache,mx,mxdim,subResult);
    return subResult;
  }
  if (mxdim<=256)
  {
    BTree<DeterminantSubResult<256> >cache;
    DeterminantSubResult<256> subResult(true);
    GetDeterminantOfPartMatrix<256>(cache,mx,mxdim,subResult);
    return subResult;
  }
  else return 0;
}


}