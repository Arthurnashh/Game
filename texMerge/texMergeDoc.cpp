// texMergeDoc.cpp : implementation of the CTexMergeDoc class
//

#include "stdafx.h"
#include "texMerge.h"

#include <El/elementpch.hpp>
#include <El/ParamFile/paramFile.hpp>//FLY#include "..\lib\paramFile.hpp"

#include "texMergeDoc.h"

#include <Es/Types/llinks.hpp>//FLY#include <class\llinks.hpp>
#include <El/Pathname/Pathname.h>
#include <Es/Files/Filenames.hpp>

#include "..\posAccess\posAccess.hpp"
//#include "..\lib\shape.hpp"
#include "..\..\img\Picture\picture.hpp"
#include <io.h>
#include "DlgCheckInComments.h"
#include <El/Scc/MsSccProject.hpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTexMergeDoc

IMPLEMENT_DYNCREATE(CTexMergeDoc, CDocument)

BEGIN_MESSAGE_MAP(CTexMergeDoc, CDocument)
  //{{AFX_MSG_MAP(CTexMergeDoc)
  ON_COMMAND(ID_FILE_EXPORT, OnFileExport)
  ON_COMMAND(ID_FILE_MERGED, OnFileMerged)
  //}}AFX_MSG_MAP
  ON_COMMAND(ID_FILE_TGAOUTPUT, OnFileTgaoutput)
  ON_UPDATE_COMMAND_UI(ID_FILE_TGAOUTPUT, OnUpdateFileTgaoutput)
  ON_COMMAND(ID_FILE_OPENFROMSOURCESAFE, OnFileOpenfromsourcesafe)
  ON_UPDATE_COMMAND_UI(ID_FILE_OPENFROMSOURCESAFE, OnUpdateFileOpenfromsourcesafe)
  ON_COMMAND(ID_FILE_SAVEANDCHECKIN, OnFileSaveandcheckin)
  ON_UPDATE_COMMAND_UI(ID_FILE_SAVEANDCHECKIN, OnUpdateFileSaveandcheckin)
  ON_COMMAND(ID_SOURCESAFEOTHERCOMMANDS_CHECKOUT, OnSourcesafeothercommandsCheckout)
  ON_UPDATE_COMMAND_UI(ID_SOURCESAFEOTHERCOMMANDS_CHECKOUT, OnUpdateSourcesafeothercommandsCheckout)
  ON_COMMAND(ID_SOURCESAFE_GETLATESTVERSION, OnSourcesafeGetlatestversion)
  ON_UPDATE_COMMAND_UI(ID_SOURCESAFE_GETLATESTVERSION, OnUpdateSourcesafeGetlatestversion)
  ON_COMMAND(ID_SOURCESAFEOTHERCOMMANDS_HISTORY, OnSourcesafeothercommandsHistory)
  ON_UPDATE_COMMAND_UI(ID_SOURCESAFEOTHERCOMMANDS_HISTORY, OnUpdateSourcesafeothercommandsHistory)
  ON_COMMAND(ID_SOURCESAFEOTHERCOMMANDS_UNDOCHECKOUT, OnSourcesafeothercommandsUndocheckout)
  ON_UPDATE_COMMAND_UI(ID_SOURCESAFEOTHERCOMMANDS_UNDOCHECKOUT, OnUpdateSourcesafeothercommandsUndocheckout)
  ON_COMMAND(ID_SOURCESAFE_DISCONNECT, OnSourcesafeDisconnect)
  ON_UPDATE_COMMAND_UI(ID_SOURCESAFE_DISCONNECT, OnUpdateSourcesafeDisconnect)
END_MESSAGE_MAP()

// memory streams

SCCRTN MSSCCIResult;
static MsSccLib MSSCCI(&MSSCCIResult);

class QIMStream: public QIStrStream
{
  //Temp<char> _memory;

public:
  void Load( CArchive &ar );

  //const void *Data() const {return _buf->Data();}
  //int Size() const {return GetDataSize();}
  bool Compare(const void *with, int withSize)
  {
    if (_buf->GetSize()!=withSize ) return true;
    const void *dta = _buf->DataLock(0,_buf->GetSize());
    bool ret = memcmp(dta,with,withSize)!=0;
    _buf->DataUnlock(0,_buf->GetSize());
    return ret;
  }
};

class QOMStream: public QOStrStream
{
public:
  void Save( CArchive &ar );

  const void *Data() const {return str();}
  int Size() const {return pcount();}

};

void QIMStream::Load( CArchive &ar )
{
  // get file size
  const CFile *file=ar.GetFile();
  int size=file->GetLength();

  Ref<QIStreamBuffer> buf = new QIStreamBuffer(size);
  //_memory.Realloc(size);
  ar.Read(buf->DataLock(0,size),size);
  buf->DataUnlock(0,size);
  // parse source file
  init(buf);
}

void QOMStream::Save( CArchive &ar )
{
  ar.Write(Data(),Size());
}

/////////////////////////////////////////////////////////////////////////////
// CTexMergeDoc construction/destruction

static CLRefList<PictureData> MCache;

CTexMergeDoc::CTexMergeDoc()
{
  _tgaOutput=false;
  _ssFileState=-1;
}

CTexMergeDoc::~CTexMergeDoc()
{
  _textures.Clear();
  MCache.Clear();
}

BOOL CTexMergeDoc::IsModified()
{
  if( !CDocument::IsModified() ) return false;
  // document might be modified
  // try to compare with file
  // load old version
  QIMStream f;
  try
  {
    CFile file(GetPathName(),CFile::modeRead);
    CArchive ar(&file,CArchive::load);
    f.Load(ar);
  }
  catch( CFileException* e )
  {
    // Handle the exception here.
    // "e" contains information about the exception.
    e->Delete();
    if( _textures.Size()>1 || _textures.Size()==1 && _textures[0].Size()>0 )
    {
      return true;
    }
    return false;
  }

  // store to paramfile
  ParamFile par;
  Save(par,"");
  // store paramfile to memory stream
  QOMStream nf;
  par.Save(nf,0);

  bool modified = f.Compare(nf.Data(),nf.Size());
  SetModifiedFlag(modified);
  return modified;
}

BOOL CTexMergeDoc::OnNewDocument()
{
  if (!CDocument::OnNewDocument())
    return FALSE;

  // (SDI documents will reuse this document)
  // always start with one texture set (empty)
  MCache.Clear();
  _textures.Clear();
  _textures.Add();

  return TRUE;
}



// implementation of underlying classes

TextureItem::TextureItem()
{
  Clear();
}

void TextureItem::Clear()
{
  x=y=0;
  scale=0;
  angle=0;
  _name="";
  _itemName = RString("");
  // file information
  w=h=0;
  importance = 1;
  _selected=false;
}

class FunctorLoadProperties
{
  AutoArray<TextureProp> &_prop;
public:

  FunctorLoadProperties(AutoArray<TextureProp> &x):_prop(x) {}
  bool operator()(const ParamEntryVal &val) const
  {
    RString name=val.GetName();
    RString value=val.GetValue();
    _prop.Append(TextureProp(name,value));
    return false;
  }
};

void TextureItem::Load( const char *basepath, ParamEntryPar f )
{
  Clear();
  const char *name=RString(f>>"name");
  char *full=(char *)alloca(strlen(name)+strlen(basepath)+100);
  strcpy(full,name);
  if( name[0]!='\\' && (name[0]==0 || name[1]!=':' ))
  {
    strcpy(full,basepath);
    TerminateBy(full,'\\');
    strcat(full,name);
  }
  SetTexture(full);
  _itemName = BuildClassName(f.GetName());
  x=f>>"x";
  y=f>>"y";
  scale=f>>"scale";
  angle=f>>"angle";
  if (f.FindEntry("importance"))
  {
    importance=f>>"importance";
  }
  else
  {
    importance=1;
  }
  ParamEntryPtr prop=f.FindEntry("Properties");
  if (prop && prop->IsClass())
  {
    prop->GetClassInterface()->ForEachEntry(FunctorLoadProperties(_properties));
  }
}
void TextureItem::Save( const char *basepath, ParamClassPtr f ) const
{
  f->Add("x",x);
  f->Add("y",y);
  f->Add("w",w);
  f->Add("h",h);
  f->Add("scale",scale);
  f->Add("importance",importance);
  const char *name=_name;
  const char *endBase = *basepath ? basepath+strlen(basepath)-1 : basepath;
  while( *name==*basepath && *name && *basepath ) name++,basepath++;
  if( *basepath==0 && *endBase=='\\')
  {
  }
  else if( *basepath==0 && (*name=='\\' || *endBase=='\\'))
  {
    name++;
  }
  else
  {
    name=_name;
  }

  f->Add("name",name);
  f->Add("angle",angle);

  if (_properties.Size())
  {  
    ParamClassPtr prop=f->AddClass("Properties");
    for (int i=0;i<_properties.Size();i++)
    {
      prop->Add(_properties[i].name,_properties[i].value);
    }
  }
}

// global texture heap

float TextureItem::GetScaledImportance() const
{
  float scaleCoef = GetScaleCoef();
  return importance/(scaleCoef*scaleCoef);
}

int TextureItem::UsedW() const
{
  int zw=Zoom(w,scale);
  int zh=Zoom(h,scale);
  return angle&1 ? zh : zw;
}

int TextureItem::UsedH() const
{
  int zw=Zoom(w,scale);
  int zh=Zoom(h,scale);
  return angle&1 ? zw : zh;
}

void TextureItem::SetTexture( CString tex )
{
  if( strcmpi(_name,tex) )
  {
    _name=tex;
    _itemName = BuildClassName(tex);
    _tex=NULL;
  }
  PictureData *pac=GetTex();
  if( !pac )
  {
    w=32;
    h=32;
    checksum=0;
  }
  else
  {
    w=pac->W();
    h=pac->H();
    // calculate checksum
    int sum = 0;
    for (int x=0; x<w; x++) for (int y=0; y<h; y++)
    {
      int pixel = pac->GetPixel(x,y);
      sum += pixel;
    }

    checksum = sum;
  }
}

PictureData *TextureItem::GetTex() const
{
  // load if neccessary
  if( _tex ) return _tex;

  while( MCache.Size()>1024 )
  {
    PictureData* last=MCache.Last();
    if( !last ) break;
    // remove from cache - will release last ref
    MCache.Delete(last);
  }

  Ref<PictureData> texture=new PictureData();

  if (texture->Load(_name)<0) return NULL;
  if (texture->W()<=0 || texture->H()<=0 ) return NULL;

  MCache.Insert(texture);		

  PictureData *ptr=texture;
  _tex=ptr;
  return ptr;
}


void TextureItem::Compact()
{
  // unload if neccessary
}


void TextureItem::Copy( const TextureItem &item )
{
  _itemName = item._itemName;
  _name=item._name; // path name (relative to current directory)
  _tex=item._tex;
  x=item.x;
  y=item.y;
  angle=item.angle;
  scale=item.scale;
  w=item.w;
  h=item.h;
  checksum=item.checksum;
  importance=item.importance;
  _selected=item._selected;
  _properties=item._properties;
}

TextureSet::TextureSet()
{
  _w=512;
  _h=512;

  _alpha = false;
  _bgcolor[0]=_bgcolor[1]=_bgcolor[2]=_bgcolor[3]=0.0f;
}
void TextureSet::Clear()
{
  _alpha = false;

  base::Clear();
}
void TextureSet::Load( const char *basepath, ParamEntryPar f )
{
  Clear();
  ParamEntryVal a=f>>"Items";
  _name = f.GetName();
  _w=f>>"w";
  _h=f>>"h";
  ParamEntryVal mlist=f>>"models";
  for( int i=0; i<mlist.GetSize(); i++ )
  {
    _models.AddUnique((const char *)RString(mlist[i]));
  }
  _alpha=f>>"alpha";
  for (int i=0; i<a.GetEntryCount(); i++)
  {
    ParamEntryVal entry = a.GetEntry(i);
    //TextureItem &item=
    int index = Add();
    // entry should be class
    Set(index).Load(basepath,entry);
  }
  ParamEntryPtr prop=f.FindEntry("Properties");
  if (prop && prop->IsClass())
  {
    prop->GetClassInterface()->ForEachEntry(FunctorLoadProperties(_properties));
  }
}

#include <Es/Files/filenames.hpp>//FLY#include <class/filenames.hpp>

bool TextureSet::IsPaa() const
{
  if (_alpha) return true;
  for (int i=0; i<Size(); i++)
  {
    const char *name = Get(i).GetTexture();
    const char *ext = GetFileExt(GetFilenameExt(name));
    if (!strcmpi(ext,".paa"))
    {
      return true;
    }   
  }
  return false;
}

void TextureSet::Save
(
 const char *basepath, const char *mergedpath,
 ParamClassPtr f, int index,bool tgaOutput
 ) const
{

  // save
  ParamClassPtr a=f->AddClass("Items");
  f->Add("w",_w);
  f->Add("h",_h);
  f->Add("alpha",_alpha);
  // get last part of merged path
  //char baseMergedName[512];
  //GetFilename(baseMergedName,mergedpath);
  const char *baseMergedName;
  if (mergedpath[1] == ':')
  {
    baseMergedName = &mergedpath[3];
  }
  else
  {
    baseMergedName = mergedpath;
  }
  // check if any source is paa. If not, append pac
  bool isPaa = IsPaa();
  CString fullname=baseMergedName;
  if (fullname.GetLength() && fullname[fullname.GetLength()-1]!='\\') fullname+='\\';
  fullname+= CString(_name) + CString(tgaOutput?".tga":(isPaa ? ".paa" : ".pac"));
  f->Add("file",(const char *)fullname);
  ParamEntryPtr mlist=f->AddArray("models");
  for( int i=0; i<_models.Size(); i++ )
  {
    mlist->SetValue(i,(const char *)_models[i]);
  }
  for( int i=0; i<Size(); i++ )
  {
    /*
    char name[256];
    sprintf(name,"Item%05d",i);
    ParamClassPtr e=a->AddClass(name);
    */
    RString name = Get(i)._itemName;
    ParamClassPtr e=a->AddClass(name);
    Get(i).Save(basepath,e);
  }
  if (_properties.Size())
  {  
    ParamClassPtr prop=f->AddClass("Properties");
    for (int i=0;i<_properties.Size();i++)
    {
      prop->Add(_properties[i].name,_properties[i].value);
    }
  }
}

bool TextureSet::IsFree( const TextureItem *exclude, int x, int y, int w, int h ) const
{
  for( int i=0; i<Size(); i++ )
  {
    const TextureItem &item=Get(i);
    if( &item==exclude ) continue;
    if
      (
      max(item.x,x)<min(item.x+item.UsedW(),x+w) &&
      max(item.y,y)<min(item.y+item.UsedH(),y+h)
      )
    {
      return false;
    }
  }
  return true;
}

void TextureSet::FindPosition( int &x, int &y, int w, int h ) const
{
  for( int mx=0; mx<=max(_h-h,_w-w); mx+=32 )
  {
    int mxy=min(mx,_h-h);
    int mxx=min(mx,_w-w);
    for( int yy=0; yy<=mxy; yy+=16 ) for( int xx=0; xx<=mxx; xx+=16 )
    {
      // check if this position is free
      if( IsFree(NULL,xx,yy,w,h) )
      {
        x=xx;
        y=yy;
        return;
      }
    }
  }
  // no free position
  x=-1;
  y=-1;
}

#include <Es/Algorithms/qsort.hpp>//FLY#include <class\QSort.hpp>
#include ".\texmergedoc.h"

static int CmpItemSize( const TextureItem *i1, const TextureItem *i2 )
{
  int area1=i1->UsedW()*i1->UsedH();
  int area2=i2->UsedW()*i2->UsedH();
  int d=area2-area1;
  if( d ) return d;
  d=(const char *)i1-(const char *)i2;
  return d;
}

int TextureSet::FindLeastImportant(int minWH) const
{
  int best = -1;
  float bestImportance = FLT_MAX;

  for( int i=0; i<Size(); i++ )
  {
    const TextureItem &item = Get(i);
    if (item.w<minWH || item.h<minWH) continue;
    // if importance is the same, prefer smaller items
    float area = item.UsedH()*item.UsedW();
    float importance = item.GetScaledImportance()+area*1e-8;
    if (bestImportance>importance)
    {
      bestImportance=importance;
      best = i;
    }
  }
  return best;
}

int TextureSet::FindMostImportant(int maxScale) const
{
  int best = -1;
  float bestImportance = FLT_MIN;
  for( int i=0; i<Size(); i++ )
  {
    const TextureItem &item = Get(i);
    if (item.scale>maxScale) continue;
    float importance = item.GetScaledImportance();
    if (bestImportance<importance)
    {
      bestImportance=importance;
      best = i;
    }
  }
  return best;
}


struct FreePositionItem
{
  int x;
  int y;
  int width;
  int height;
};

TypeIsSimple(FreePositionItem);

TypeIsSimple(TextureItem *);

struct FreePositions
{
  AutoArray<FreePositionItem> items;
  void AddFreePosition(int x, int y, int width, int height);  
  bool FindFreePosition(int width, int height, int &x, int &y, bool &rot);
};

void FreePositions::AddFreePosition(int x, int y, int width, int height)
{

  FreePositionItem &newitem=items.Append();
  newitem.x=x;
  newitem.y=y;
  newitem.width=width;
  newitem.height=height;
  ASSERT(height!=0);
}

bool FreePositions::FindFreePosition(int width, int height, int &x, int &y, bool &rot)
{
  int i;
  
  rot=false;
  for (i=0;i<items.Size();i++) if (items[i].height==height && items[i].width>=width) break;
  if (i==items.Size()) 
  {
    for (i=0;i<items.Size();i++) if (items[i].height>height && items[i].width>=width) break;
    if (i==items.Size())
    {
      for (i=0;i<items.Size();i++) 
        if (items[i].height>width && items[i].width>height) break;
      if (i==items.Size()) return false;
      width^=height;    //w^h   h
      height^=width;    //w^h   w^h^h=w
      width^=height;    //w^h^w^h^h=h  w
      rot=true;
    }
  }
  if (items[i].height>height)
  {
    AddFreePosition(items[i].x,items[i].y+height,items[i].width,items[i].height-height);
    items[i].height=height;
  }
  x=items[i].x;
  y=items[i].y;
  items[i].width-=width;
  items[i].x+=width;
  if (items[i].width<=0)
  {
    items.Delete(i);
  }
  else
  {
    bool sluc=true;
    while (sluc)
    {
      sluc=false;
      if (i>0 && items[i-1].x==items[i].x && items[i-1].y+items[i-1].height==items[i].y && 
        items[i-1].width==items[i].width)
      {
        items[i-1].height+=items[i].height;
        items.Delete(i);      
        i--;
        sluc=true;
      }
      if (i+1<items.Size() && items[i].x==items[i+1].x && items[i].y+items[i].height==items[i+1].y && 
        items[i].width==items[i+1].width)
      {
        items[i+1].height+=items[i].height;
        items[i+1].y=items[i].y;
        items.Delete(i);      
        sluc=true;
      }
    }
  }
  return true;
}

static int SortTextureItems(const void *left, const void *right)
{
  const TextureItem *l1=*(const TextureItem **)left;
  const TextureItem *r1=*(const TextureItem **)right;
  int res=(l1->UsedH()<r1->UsedH())-(l1->UsedH()>r1->UsedH());
  if (res==0) res=(l1->importance<r1->importance)-(l1->importance>r1->importance);
  if (res==0) res=(l1>r1)-(l1<r1);
  return res;
}

#define DEMO() if (demonstration) {AfxGetMainWnd()->Invalidate();AfxGetMainWnd()->UpdateWindow();}


static int _cdecl TextureSetItemDownUp(const void *a, const void *b)
{
  const int *ia=reinterpret_cast<const int *>(a);
  const int *ib=reinterpret_cast<const int *>(b);
  return (*ia<*ib)-(*ia>*ib);
}

bool TextureSet::AutoArrange2()
{
  if (Size()==0) return true;
  bool demonstration=(GetKeyState(VK_SHIFT)<0);
  bool repeat=true;
  for (int i=0;i<Size();i++) 
  {
     (*this)[i].scale=0;
      if ((*this)[i].UsedH()>(*this)[i].UsedW())      //otoc vsechny textury nalezato
        (*this)[i].angle=((*this)[i].angle+1)&3;
        while ((*this)[i].UsedW()>_w) (*this)[i].scale--; //zmensi textury vetsi nez nejsirsi sirka
    (*this)[i].x=10000;
  }
  
  DEMO();  
  while (repeat)
  {
    AutoArray<TextureItem *>sortedItems;
    FreePositions freePos;
    for (int i=0;i<Size();i++) 
    {
      sortedItems.Append()=&(*this)[i];
    }
    sortedItems.QSortBin(SortTextureItems);  //srovnej vsechny textury nejprve podle sirky a pak podle importance
    int col=0;
    int line=0;
    int pos=0;
    int lineheight=16; //minimalni lineheight
    int lastheight=0;
    while (pos<Size())
    {
      TextureItem *item=sortedItems[pos];   //vem texturu 
      int itemUsedW=item->UsedW();
      int itemUsedH=item->UsedH();
      if (col+itemUsedW>_w) //pokud se textura uz nevejde
      {
        int spaceleft=_w-col;
        if (spaceleft>0) //zbylo nejake volne misto?
        {
          freePos.AddFreePosition(col,line,spaceleft,lastheight);
        }
        col=0;
        line+=lineheight;
        lineheight=16;     
        lastheight=0;
      }
      if (line+itemUsedH>_h) //pokud se uz textura nevejde ani na radku
      {
        int wspaceleft=_w-col;    
        int hspaceleft=_h-line;
        if (wspaceleft>0 && hspaceleft>0)
        {
          freePos.AddFreePosition(col,line,wspaceleft,hspaceleft);
        }
        break;   
      }
      item->x=col;    //umisti ji na pozici;
      item->y=line;      
      if (itemUsedH>lineheight) lineheight=itemUsedH;
      if (itemUsedH<lastheight)
      {
        freePos.AddFreePosition(col,line+itemUsedH,_w-col,lineheight-itemUsedH);
      }
      col+=itemUsedW;
      lastheight=itemUsedH;
      pos++;
      DEMO();
    }
    //nyni mame naskladanu vetsinu textur v setu, ale pokud nam neco zbylo,
    //muzeme vyuzit nejaka volna mista.
    while (pos<Size())
    {
      TextureItem *item=sortedItems[pos];   //vem texturu 
      int x,y;
      bool rot;    
      int scaleSave=item->scale;
      int itemUsedW=item->UsedW();
      int itemUsedH=item->UsedH();
      bool found=freePos.FindFreePosition(itemUsedW,itemUsedH,x,y,rot);
/*      while (!found && itemUsedW>16 && itemUsedH>16)
      {
        item->scale--;
        itemUsedW=item->UsedW();
        itemUsedH=item->UsedH();
        found=freePos.FindFreePosition(itemUsedW,itemUsedH,x,y,rot);
      }      */
      if (!found) //ani po zmenseni nebylo nalezeno volne misto
      {
        //ASSERT(freePos.items.Size()==0);
        item->scale=scaleSave;
        float minImportance=FLT_MAX;
        int minindex=-1;
        for (int i=0;i<pos;i++)
        {
          TextureItem *witem=sortedItems[i];
          float coef=1.0f/witem->GetScaleCoef()*witem->importance;
          if (minImportance-coef>0.00001) 
          {
            minImportance=coef;
            minindex=i;
          }
        }
//        ASSERT(minindex!=-1);
        if (minindex<0) minindex=0;
        sortedItems[minindex]->scale--;
        sortedItems[minindex]->y=_h;
        for (int i=minindex+1;i<Size();i++)
        {
          int itemUsedW=sortedItems[i]->UsedW();
          int itemUsedH=sortedItems[i]->UsedH();
          while (itemUsedH<sortedItems[minindex]->UsedH()
            && sortedItems[i]->GetScaledImportance()>sortedItems[i]->importance 
            && itemUsedH<sortedItems[i]->h 
            && itemUsedW<_w 
            && itemUsedH<_h
            )
          {
            sortedItems[i]->scale++;
            itemUsedW=sortedItems[i]->UsedW();
            itemUsedH=sortedItems[i]->UsedH();
          }
          sortedItems[i]->y=_h;
        }   
        if (demonstration) Sleep(1000);
      break;
      }
      else
      {
        item->x=x;
        item->y=y;
        if (rot) item->angle=(item->angle+1)&3;
        pos++;

      }
      DEMO();
    }
    if (pos>=Size())
        repeat=false;
  }
  return true;
}


void TextureSet::AutoArrange(AutoArrangeMode mode/* =AAM_Grow */)
{
  if (mode==AAM_KeepSize) {AutoArrange2();return;}
  else
  {
    if (mode==AAM_Grow)
    {
      _w=MaxSetDimension;
      _h=MaxSetDimension;
    }
    bool repeat=false;
    do
    {
      AutoArrange2();    
      unsigned long totalArea=_w*_h;
      unsigned long usedArea=0;
      for (int i=0;i<Size();i++)      
        usedArea+=(*this)[i].UsedW()*(*this)[i].UsedH();
      repeat=(totalArea/5*3)>usedArea;
      if (repeat)
      {
        if (_w==256 && _h==256) repeat=false;
        if (_w<_h) _h>>=1;else _w>>=1;        
      }
    }
    while (repeat);
  }

/*  }
  bool tryagain=false;
  // store all items
  TextureSet temp=*this;
  // clear items
  // calculate total area
  // calculate max. size of single item
  if (!grow)
  {
    int totalArea = 0;
    for( int i=0; i<temp.Size(); i++ )
    {
      const TextureItem &item=temp[i];
      int w = item.UsedW();
      int h = item.UsedH();
      totalArea += w*h;
    }
    // if we have some space left, enlarge the most important
    while (totalArea<_w*_h)
    {
      int index = temp.FindMostImportant(-1);
      if (index<0) break;
      TextureItem &item=temp[index];
      int originalArea = item.UsedH()*item.UsedW();
      ++item.scale;
      totalArea -= originalArea;
      totalArea += originalArea<<2;
    }
    // check if all items together will fit
    // if not, shrink the least important item
    while (totalArea>_w*_h)
    {
      // find only items which can be shrinked
      int index = temp.FindLeastImportant(2);
      if (index<0) break;
      TextureItem &item=temp[index];
      int originalArea = item.UsedH()*item.UsedW();
      --item.scale;
      totalArea -= originalArea;
      totalArea += originalArea>>2;
    }
  }
  // make as small as possible
  else
  {
    _w=64;
    _h=64;
  }
  // sort array by size
  QSort(temp.Data(),temp.Size(),CmpItemSize);
  base::Clear();
  for( int i=0; i<temp.Size(); i++ )
  {
    TextureItem item=temp[i];
    // check if some texture is identical
    int index;
    int angle=Contains(item,&index);
    if( angle>=0 )
    {
      item=Get(index);
      item.SetTexture(temp[i].GetTexture());
      item.angle-=angle;
      //item.angle=(angled.angle-angle)&3;
      item.angle&=3;
      Add(item);
      continue;
    }
    // w is enlarged first
    // always: w>=h
    // check if set is large enough to hold item
    int bigger=max(item.UsedW(),item.UsedH());
    int smaller=min(item.UsedW(),item.UsedH());
    if (grow)
    {
      if( _w<bigger  ) _w=bigger;
      if( _h<smaller  ) _h=smaller;
    }
    for(;;)
    {
      for( int angle=0; angle<2; angle++ )
      {
        item.angle=angle;
        FindPosition(item.x,item.y,item.UsedW(),item.UsedH());
        if( item.x>=0 && item.y>=0 ) break;
      }
      if( item.x>=0 && item.y>=0 ) break;
      if( _w>=MaxSetDimension && _h>=MaxSetDimension )
      {
        // no free space
        item.x=item.y=0;
        tryagain=true;      
        break;
      }
      // enlarge set and try again
      if( _w<=_h ) _w<<=1;
      else _h<<=1;
    }
    Add(item);
  }
  if (tryagain && grow)
  {
    AutoArrange(false);    
  }
  */
}

static bool CheckRot( const char *fName, const char *iName )
{
  /*
  if( strstr(fName,"90.") && !strstr(iName,"90.") )
  {
  // fName migth be filename(iName)+"90"+ext(iName)
  char buf[256];
  strcpy(buf,iName);
  char *ext=strrchr(buf,'.');
  if( !ext ) return false;
  char fExt[64];
  strcpy(fExt,ext);
  strcpy(ext,"90");
  strcat(ext,fExt);
  if( !strcmpi(buf,fName) )
  {
  return true;
  }
  }
  */
  return false;
}

bool TextureSet::ContainsExact( const TextureItem &find ) const
{
  for( int i=0; i<Size(); i++ )
  {
    const TextureItem &item=Get(i);
    const char *fName=find.GetTexture();
    const char *iName=item.GetTexture();
    if( !strcmpi(fName,iName) )
    {
      return true;
    }
  }
  return false;
}

int TextureSet::Contains( const TextureItem &find, int *index ) const
{
  for( int i=0; i<Size(); i++ )
  {
    const TextureItem &item=Get(i);
    const char *fName=find.GetTexture();
    const char *iName=item.GetTexture();
    if( !strcmpi(fName,iName) )
    {
      if( index ) *index=i;
      return 0;
    }
    int sizeD=find.w*find.h*2;
    int sizeI=item.w*item.h*2;
    if( sizeD!=sizeI ) continue;
    if( find.checksum==item.checksum )
    {
      // candidate for identity
      PictureData *fTex=find.GetTex();
      PictureData *iTex=find.GetTex();
      if (fTex==NULL || iTex==NULL) return -1;
      if( !fTex->Compare(*iTex) )
      {
        if( index ) *index=i;
        return 0;				
      }
    }

    // one of them might be 90deg rotated
    // note: textures are not identical, data check will surely fail
    if( CheckRot(fName,iName) )
    {
      if( index ) *index=i;
      return 3;
    }
    if( CheckRot(iName,fName) )
    {
      if( index ) *index=i;
      return 1;
    }
  }
  return -1;
}

// ParamFile save/load

RString BuildClassName(const char *name)
{
  if (!name || strlen(name)==0)
  {
    return "";
  }
  char buf[1024];
  GetFilename(buf,name);
  for(;;)
  {
    char *space = strchr(buf,' ');
    if (!space) break;
    *space = '_';
  }
  for (;;)
  {
    char *invalidChar = strpbrk(buf,"()/:?*");
    if (!invalidChar) break;
    *invalidChar = '_';
  }
  return buf;
}

CString TextureSet::BuildName( int maxLen, char delim ) const
{
  char delimStr[2]={delim,0};
  CString model;
  char prev[256]="";
  for( int j=0; j<_models.Size(); j++ )
  {
    char curr[256];
    if( model.GetLength()>=maxLen ) break;
    strcpy(curr,_models[j]);
    char *ext=strchr(curr,'.');
    if( ext ) *ext=0;
    const char *wordC=curr;
    const char *wordP=prev;
    const char *skipC=curr;
    const char *skipP=prev;
    while( *skipP && *skipC )
    {
      // check words
      while( *skipP!=0 && *skipP!='_' ) skipP++;
      while( *skipC!=0 && *skipC!='_' ) skipC++;
      // compare words
      int lenP=skipP-wordP;
      int lenC=skipC-wordC;
      if( lenP!=lenC ) break;
      if( strnicmp(wordC,wordP,lenP) ) break;
      if( *skipC ) skipC++;
      if( *skipP ) skipP++;
      wordC=skipC;
      wordP=skipP;
    }

    if( model.GetLength()+(int)strlen(wordC)+2<=maxLen )
    {
      if( j>0 ) model+=delimStr;
      model+=wordC;
      strcpy(prev,curr);
    }
  }
  return model;
}

#define Compose(a,r,g,b) \
  ((a)<<24)|((r)<<16)|((g)<<8)|((b)<<0)

#define GetA(a) ( ((a)>>24)&0xff )
#define GetR(a) ( ((a)>>16)&0xff )
#define GetG(a) ( ((a)>> 8)&0xff )
#define GetB(a) ( ((a)>> 0)&0xff )

struct InBetweenColInfo
{
  unsigned short xpos;
  unsigned short ypos;
  PPixel resColor;
};

static InBetweenColInfo *GenerateInbetweenColors_Prepare(Picture &img, const PPixel &maskColor)
{
  int cntSpace=0;
  for (int y=0,cnty=img.Height();y<cnty;y++)
    for (int x=0,cntx=img.Width();x<cntx;x++) 
      if ((unsigned long)img.GetPixelARGBRaw(x,y)==maskColor)
        cntSpace++;

  InBetweenColInfo *blck=new InBetweenColInfo [cntSpace+1];
  cntSpace=0;
  for (int y=0,cnty=img.Height();y<cnty;y++)
    for (int x=0,cntx=img.Width();x<cntx;x++) 
      if ((unsigned long)img.GetPixelARGBRaw(x,y)==maskColor)
      {
        blck[cntSpace].xpos=x;
        blck[cntSpace].ypos=y;
        blck[cntSpace].resColor=maskColor;
        cntSpace++;
      }
  blck[cntSpace].xpos=-1;
  blck[cntSpace].ypos=-1;
  return blck;
}

static bool GenerateInbetweenColors_ProcessOneStep(Picture &img, const PPixel &maskColor, InBetweenColInfo *blck)
{
  int i=0;
  unsigned short x=blck[i].xpos;
  unsigned short y=blck[i].ypos;
  while (x!=(unsigned short)-1 && y!=(unsigned short)-1)
  {
    int cntColors=0;
    int rsum=0,gsum=0,bsum=0,asum=0;
    PPixel tmp;
    if (x>0 && (tmp=img.GetPixelARGBRaw(x-1,y))!=maskColor)
      {rsum+=GetR(tmp);gsum+=GetG(tmp);bsum+=GetB(tmp);asum+=GetA(tmp);cntColors++;}
    if (y>0 && (tmp=img.GetPixelARGBRaw(x,y-1))!=maskColor)
      {rsum+=GetR(tmp);gsum+=GetG(tmp);bsum+=GetB(tmp);asum+=GetA(tmp);cntColors++;}
    if (x+1<img.Width() && (tmp=img.GetPixelARGBRaw(x+1,y))!=maskColor)
      {rsum+=GetR(tmp);gsum+=GetG(tmp);bsum+=GetB(tmp);asum+=GetA(tmp);cntColors++;}
    if (y+1<img.Height() && (tmp=img.GetPixelARGBRaw(x,y+1))!=maskColor)
      {rsum+=GetR(tmp);gsum+=GetG(tmp);bsum+=GetB(tmp);asum+=GetA(tmp);cntColors++;}
    if (cntColors>0)
    {
      blck[i].resColor=Compose(asum/cntColors,rsum/cntColors,gsum/cntColors,bsum/cntColors);
    }
    i++;
    x=blck[i].xpos;
    y=blck[i].ypos;
  }
  return i!=0;
}

static bool GenerateInbetweenColors_SaveOneStep(Picture &img, const PPixel &maskColor, InBetweenColInfo *blck)
{
  bool hitaction=false;
  int i=0,shift=0;
  unsigned short x=blck[i].xpos;
  unsigned short y=blck[i].ypos;
  while (x!=(unsigned short)-1 && y!=(unsigned short)-1)
  {
    if (blck[i].resColor!=maskColor)
    {
      img.SetPixel(blck[i].xpos,blck[i].ypos,blck[i].resColor);
      hitaction=true;
    }
    else
    {
      if (shift!=i) blck[shift]=blck[i];
      shift++;
    }
    i++;
    x=blck[i].xpos;
    y=blck[i].ypos;
  }
  blck[shift]=blck[i];
  return hitaction;
}


static void GenerateInbetweenColors(Picture &img, const PPixel &maskColor)
{
  InBetweenColInfo *wrk=GenerateInbetweenColors_Prepare(img,maskColor);
  while (GenerateInbetweenColors_ProcessOneStep(img,maskColor,wrk))
    if (!GenerateInbetweenColors_SaveOneStep(img,maskColor,wrk)) break;
  delete [] wrk;
}

bool TextureSet::Export( const char *filename, bool tgaOutput, IROCheck *_rocheck ) const
{
  // Get avg color of all textures
  PPixel backColor = PPixel(0);
  if (_bgcolor[0]==0 && _bgcolor[1]==0 && _bgcolor[2]==0 && _bgcolor[3]==0)
  {  
    /*
    int pixCount = 0;
    int pixCountA = 0;
    __int64 sumR = 0;
    __int64 sumG = 0;
    __int64 sumB = 0;
    __int64 sumA = 0;
    for (int i = 0; i < Size(); i++)
    {
      const TextureItem &item=Get(i);
      PictureData *tex=item.GetTex();
      if ( !tex) continue;
      for (int y = 0; y < tex->H(); y++)
      {
        for (int x = 0; x < tex->W(); x++)
        {
          int pixel = tex->GetPixel(x, y);
          if (GetA(pixel) > 0)
          {
            sumR += GetR(pixel);
            sumG += GetG(pixel);
            sumB += GetB(pixel);
            pixCount++;
          }
          sumA += GetA(pixel);
          pixCountA++;
        }
      }
    }
    // for opaque textures create opaque background
    if (pixCountA>0 && sumA/pixCountA>=128)
    {
      backColor = Compose(0xff, sumR / pixCount, sumG / pixCount, sumB / pixCount);
    }
    else if (pixCount>0)
    {
      backColor = Compose(0, sumR / pixCount, sumG / pixCount, sumB / pixCount);
    }
    */
    backColor=Compose(0,255,0,255);
  }
  else
  {
    backColor=Compose(toInt(_bgcolor[3]*255.0f),toInt(_bgcolor[0]*255.0f),toInt(_bgcolor[1]*255.0f),toInt(_bgcolor[2]*255.0f));
  }
  // create a file
  Picture pic;
  pic.CreateOpaque(_w,_h);

  // Clear image to specified color
  for (int y = 0; y < pic.H(); y++) {
    for (int x = 0; x < pic.W(); x++) {
      pic.SetPixel(x, y, backColor);
    }
  }

  // work with pic as 8888 TGA
  // insert all textures as necessary
  for( int i=0; i<Size(); i++ )
  {
    const TextureItem &item=Get(i);
    // render item to picture
    int dw=item.UsedW();
    int dh=item.UsedH();
    int dx=item.x;
    int dy=item.y;
    int angle=item.angle;
    PictureData *tex=item.GetTex();
    if (!tex) continue;
    tex->ConvertARGB();
    tex->RemoveBlackFromAlpha();

    int th=(tex->H()-1)<<16;
    int tw=(tex->W()-1)<<16;
    int tlU=0,tlV=0;
    int trU=tw,trV=0;
    int blU=0,blV=th;
    int brU=tw,brV=th;
    angle&=3;
    while( --angle>=0 )
    {
      // tl<-tr<-br<-bl<-tl
      int u=tlU,v=tlV;
      tlU=trU,tlV=trV;
      trU=brU,trV=brV;
      brU=blU,brV=blV;
      blU=u,blV=v;
    }

    int stepUX=(trU-tlU)/(dw-1);
    int stepVX=(trV-tlV)/(dw-1);
    int stepUY=(blU-tlU)/(dh-1);
    int stepVY=(blV-tlV)/(dh-1);
    // perform clipping (to winDraw rectangle)

    int topY=dy;
    int bottomY=dy+dh;
    int leftX=dx;
    int rightX=dx+dw;
    if( topY<0 )
    {
      tlV+=stepVY*(-topY);
      tlU+=stepUY*(-topY);
      trV+=stepVY*(-topY);
      trU+=stepUY*(-topY);
      topY=0;
    }
    if( bottomY>_h )
    {
      bottomY=_h;
    }
    if( leftX<0 )
    {
      tlU+=stepUX*(-leftX);
      tlV+=stepVX*(-leftX);
      blU+=stepUX*(-leftX);
      blV+=stepVX*(-leftX);
      leftX=0;
    }
    if( rightX>_w )
    {
      rightX=_w;
    }

    for( int y=topY,u=tlU,v=tlV; y<bottomY; y++,u+=stepUY,v+=stepVY )
    {
      for( int x=leftX,uu=u,vv=v; x<rightX; x++,uu+=stepUX,vv+=stepVX )
      {
        int ui=uu>>16;
        int vi=vv>>16;

        // Get average value from the neighborhood
        int sizeX = max(stepUX >> 16, stepVX >> 16);
        int sizeY = max(stepUY >> 16, stepVY >> 16);
        if ((sizeX > 0) && (sizeY > 0))
        {
          int offsetUX = (stepUX >> 16) / sizeX;
          int offsetVX = (stepVX >> 16) / sizeX;
          int offsetUY = (stepUY >> 16) / sizeY;
          int offsetVY = (stepVY >> 16) / sizeY;
          int sumR = 0;
          int sumG = 0;
          int sumB = 0;
          int sumA = 0;
          int pixCount = 0;
          for (int j = 0, aJ = 0, bJ = 0; j < sizeY; j++, aJ += offsetUX, bJ += offsetVX)
          {
            for (int i = 0, aI = aJ, bI = bJ; i < sizeX; i++, aI += offsetUY, bI += offsetVY)
            {
              int k = ui + aI;
              int l = vi + bI;
              if ((k < tex->W()) && (k >= 0) && (l < tex->H()) && (l >= 0))
              {
                int pixel = tex->GetPixelARGB(k, l);
                sumA += GetA(pixel);
                sumR += GetR(pixel);
                sumG += GetG(pixel);
                sumB += GetB(pixel);
                pixCount++;
              }
            }
          }
          if (pixCount > 0)
          {
            pic.SetPixel(x, y, Compose(sumA / pixCount, sumR / pixCount, sumG / pixCount, sumB / pixCount));
          }
        }
        else
        {
          DWORD pixel=tex->GetPixelARGB(ui,vi);
          pic.SetPixel(x,y,pixel);
        }
      }
    }
  }


  if (_bgcolor[0]==0 && _bgcolor[1]==0 && _bgcolor[2]==0 && _bgcolor[3]==0)
    GenerateInbetweenColors(pic,backColor);

  CString texName=filename;
  if (tgaOutput) texName+=".tga";
  else if (IsPaa()) texName+=".paa";
  else texName+=".pac";

  pic.DisableDithering();
  int saved;
  if (_rocheck->TestFileRO(texName,ROCHF_DisableSaveAs)!=ROCHK_FileRO)
    saved=pic.Save(texName);
  else
    saved=-1;

  /*	#define SAVE_TGA 0
  #if SAVE_TGA
  Picture temp=pic;
  #endif
  // save picture
  CString paaName=CString(filename) + CString(IsPaa() ? ".paa" : ".pac");

  pic.DisableDithering();
  int saved = pic.Save(paaName);
  #if SAVE_TGA
  CString tgaName=CString(filename)+".tga";
  temp.Save(tgaName);
  #endif*/
  return saved>=0;
}

void CTexMergeDoc::Save( ParamFile &par, const char *filename )
{
  char shortName[512];
  GetFilename(shortName,filename);

  ParamClassPtr f = par.AddClass("InfTexMerge");

  for( int i=0; i<_textures.Size(); i++ )
  {
    TextureSet &set=_textures[i];
    if (set._name.GetLength()<=0)
    {
      if (set._models.Size()>0)
      {
        CString model=set.BuildParamName();
        set._name = Format("%05d%s",i,(const char *)model);
      }
      else if (*shortName)
      {
        RString itemName = BuildClassName(shortName);
        set._name = Format("%s_%d",(const char *)itemName,i);
      }
    }
    ParamClassPtr e=f->AddClass(set._name);
    set.Save(GetTexturePath(),GetMergedPath(),e,i,_tgaOutput);
  }
  const char *path = GetMergedPath();
  f->Add("mergedPath",path);
  f->Add("tgaOutput",_tgaOutput);
}
void CTexMergeDoc::Load( const ParamFile &par )
{
  ParamEntryVal f=par>>"InfTexMerge";

  ConstParamEntryPtr entry = f.FindEntry("mergedPath");
  if (entry)
  {
    const char *path = RStringB(*entry);
    _mergedPath = path;
  }
  entry=f.FindEntry("tgaOutput");
  if (entry)
  {
    int val=*entry;
    _tgaOutput=val!=0;
  }
  _textures.Clear();

  // must be cfg
  for (int i=0; i<f.GetEntryCount(); i++)
  {
    ParamEntryVal e = f.GetEntry(i);
    if (e.IsClass())
    {
      TextureSet &set=_textures[_textures.Add()];
      // entry should be class
      set.Load(GetTexturePath(),e);
    }
  }
}

static bool IsSSPresent(LPCTSTR lpszPathName)
{
  char *path=strcpy((char *)alloca(strlen(lpszPathName)+20),lpszPathName);
  char *end=strrchr(path,'\\');
  if (end==NULL) return false;
  strcpy(end,"\\vssver.scc");
  return  (_access(path,0)==0);
}

BOOL CTexMergeDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
#ifdef _DEBUG
  if (IsModified())
    TRACE(/*traceAppMsg, 0,*/ "Warning: OnOpenDocument replaces an unsaved document.\n");
#endif

  if (_curSS==NULL && MSSCCI.IsLoaded() && IsSSPresent(lpszPathName)) EnsureSSOpened();      
  _ROCheck.TestFileRO(lpszPathName,ROCHF_DisableSaveAs);
  CFileException fe;
  CFile* pFile = GetFile(lpszPathName,
    CFile::modeRead|CFile::shareDenyWrite, &fe);
  if (pFile == NULL)
  {
    ReportSaveLoadException(lpszPathName, &fe,
      FALSE, AFX_IDP_FAILED_TO_OPEN_DOC);
    return FALSE;
  }

  DeleteContents();
  SetModifiedFlag();  // dirty during de-serialize

  CArchive loadArchive(pFile, CArchive::load | CArchive::bNoFlushOnDelete);
  loadArchive.m_pDocument = this;
  loadArchive.m_bForceFlat = FALSE;
  TRY
  {
    CWaitCursor wait;
    if (pFile->GetLength() != 0)
    {
      // load file to memory
      QIMStream f;
      f.Load(loadArchive);
      ParamFile par;
      par.Parse(f);
      if (_curSS) GetLatestVersion(&par);
      Load(par);
    }
    loadArchive.Close();
    ReleaseFile(pFile, FALSE);
  }
  CATCH_ALL(e)
  {
    ReleaseFile(pFile, TRUE);
    DeleteContents();   // remove failed contents

    TRY
    {
      ReportSaveLoadException(lpszPathName, e,
        FALSE, AFX_IDP_FAILED_TO_OPEN_DOC);
    }
    END_TRY
      e->Delete();
    return FALSE;
  }
  END_CATCH_ALL

    SetModifiedFlag(FALSE);     // start off with unmodified
  SetPathName(lpszPathName,TRUE);
  UpdateFilesSSStatus();

  return TRUE;
}

BOOL CTexMergeDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
  ROCheckResult res=_ROCheck.TestFileRO(lpszPathName,0);
  if (res==ROCHK_FileSaveAs)
  {
    theApp.m_pMainWnd->PostMessage(WM_COMMAND,ID_FILE_SAVE_AS,0);
    return FALSE;
  }
  if (res==ROCHK_FileRO) return FALSE;
  CFileException fe;
  CFile* pFile = NULL;
  pFile = GetFile(lpszPathName, CFile::modeCreate |
    CFile::modeReadWrite | CFile::shareExclusive, &fe);

  if (pFile == NULL)
  {
    ReportSaveLoadException(lpszPathName, &fe,
      TRUE, AFX_IDP_INVALID_FILENAME);
    return FALSE;
  }

  CArchive saveArchive(pFile, CArchive::store | CArchive::bNoFlushOnDelete);
  saveArchive.m_pDocument = this;
  saveArchive.m_bForceFlat = FALSE;
  TRY
  {
    CWaitCursor wait;

    ParamFile par;
    // store to paramfile
    Save(par,lpszPathName);
    QOMStream f;
    // store paramfile to memory stream
    par.Save(f,0);
    // save memory to archive
    f.Save(saveArchive);

    saveArchive.Close();
    ReleaseFile(pFile, FALSE);
  }
  CATCH_ALL(e)
  {
    ReleaseFile(pFile, TRUE);

    TRY
    {
      ReportSaveLoadException(lpszPathName, e,
        TRUE, AFX_IDP_FAILED_TO_SAVE_DOC);
    }
    END_TRY
      e->Delete();
    return FALSE;
  }
  END_CATCH_ALL

    SetModifiedFlag(FALSE);     // back to unmodified
  UpdateAllViews(NULL);
  return TRUE;        // success
}

/////////////////////////////////////////////////////////////////////////////
// CTexMergeDoc serialization

void CTexMergeDoc::Serialize(CArchive& ar)
{
  Fail("Obsolete");
}

/////////////////////////////////////////////////////////////////////////////
// CTexMergeDoc diagnostics

#ifdef _DEBUG
void CTexMergeDoc::AssertValid() const
{
  CDocument::AssertValid();
}

void CTexMergeDoc::Dump(CDumpContext& dc) const
{
  CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTexMergeDoc commands

int CTexMergeDoc::AddTextureSet()
{
  SetModifiedFlag();
  int ret=_textures.Add();
  NotifyData msg(ret,-1);
  UpdateAllViews(NULL,NMSetInserted,&msg);
  return ret;
}


void CTexMergeDoc::DeleteSet(int i)
{
  _textures.Delete(i);
  // notify all views
  //POSITION pos = GetFirstViewPosition();
  NotifyData msg(i,-1);
  SetModifiedFlag();
  UpdateAllViews(NULL,NMSetDeleted,&msg);
}

int CTexMergeDoc::DuplicateSet(int i)
{
  int ret=_textures.Add();
  _textures[ret]=_textures[i];
  NotifyData msg(ret,-1);
  SetModifiedFlag();
  UpdateAllViews(NULL,NMSetInserted,&msg);
  return ret;
}

void CTexMergeDoc::AutoArrange(int set, AutoArrangeMode mode)
{
  if( set<0 || set>=_textures.Size() ) return;
  TextureSet &dstSet=_textures[set];
  dstSet.AutoArrange(mode);
  NotifyData msg(set,-1);
  SetModifiedFlag();
  UpdateAllViews(NULL,NMSetChanged,&msg);
}

int CTexMergeDoc::AddItem(int set, const TextureItem &val )
{
  if( set<0 || set>=_textures.Size() ) return -1;
  TextureSet &dstSet=_textures[set];
  int ret=dstSet.Add();
  TextureItem &dst=dstSet[ret];
  dst=val;
  dst._itemName = BuildClassName(dst.GetTexture());
  NotifyData msg(set,ret);
  SetModifiedFlag();
  UpdateAllViews(NULL,NMItemInserted,&msg);
  return ret;
}

int CTexMergeDoc::AddItem( int setIndex, const char *name )
{
  TextureSet *set=GetTextureSet(setIndex);
  if( !set ) return -1;
  int index;
  TextureItem item;
  item.SetTexture(name);
  if( set->ContainsExact(item) )
  {
    // alrady there
    return -1;
  }
  int angle=set->Contains(item,&index);
  // place item, correct angle
  // check 
  if( angle>=0 )
  {
    // overlay over angled texture
    const TextureItem &angled=set->Get(index);
    item.angle=(angled.angle-angle)&3;
    item.x=angled.x;
    item.y=angled.y;
    item.scale=angled.scale;
    // place over
    return AddItem(setIndex,item);
  }
  else
  {
    // find some free position
    for(;;)
    {
      set->FindPosition(item.x,item.y,item.w,item.h);
      if( item.x>=0 && item.y>=0 ) break;
      if( set->_w>=MaxSetDimension || set->_h>=MaxSetDimension )
      {
        // no free space
        item.x=item.y=0;
        break;
      }
      // enlarge set and try again
      set->_w<<=1;
      set->_h<<=1;
    }
    return AddItem(setIndex,item);
  }
}

int CTexMergeDoc::AddModel( const char *name )
{
  return 0;
  /*
  char buffer[256];
  GetCurrentDirectory(sizeof(buffer),buffer);
  //CString baseName;
  CString baseName=GetTexturePath();
  if( baseName.GetLength()>0 && baseName[baseName.GetLength()-1]!='\\' ) baseName+='\\';

  //int setIndex=_textures.Add();
  Ref<LODShape> shape=new LODShape(name,false);
  // scan all textures from all levels
  int ret=-1;
  for( int i=0; i<shape->NLevels(); i++ )
  {
  Shape *level=shape->LevelOpaque(i);
  // TODO: build texture list from faces
  // check only textures that are not tiled and merging is not disabled
  // scan all textures from this level
  // check if there is some set that would contain (almost) all textures
  int setIndexO=-1;
  int setIndexA=-1;
  int maxCountO=0;
  int maxCountA=0;
  if( level->NTextures()<=0 ) continue;

  int totalO=0;
  int totalA=0;
  for( int j=0; j<level->NTextures(); j++ )
  {
  Texture *texture=level->GetTexture(j);
  if( !texture ) continue;
  texture->IsAlpha() ? ++totalA : ++totalO;
  }

  for( int index=0; index<_textures.Size(); index++ )
  {
  const TextureSet &s=_textures[index];
  int countO=0;
  int countA=0;
  for( int j=0; j<level->NTextures(); j++ )
  {
  Texture *texture=level->GetTexture(j);

  if( texture )
  {
  const char *name=texture->Name();
  const char *filename=strrchr(name,'\\');
  if( filename ) filename++;
  else filename=name;
  // make complete path from document path and filename
  CString texName=baseName+filename;
  TextureItem item;
  item.SetTexture(texName);
  if( texture->IsAlpha() )
  {
  if( s.Contains(item)>=0 ) countA++;
  }
  else
  {
  if( s.Contains(item)>=0 ) countO++;
  }
  }
  }
  if( countO>maxCountO ) setIndexO=index,maxCountO=countO;
  if( countA>maxCountA ) setIndexA=index,maxCountA=countA;
  }
  if( maxCountO<(totalO*8+11)/12 && totalO>1 )
  {
  // we must create a new texture
  setIndexO=_textures.Add();
  }
  if( maxCountA<(totalA*8+11)/12 && totalA>1 )
  {
  // we must create a new texture
  setIndexA=_textures.Add();
  _textures[setIndexA]._alpha=true;
  }
  if( setIndexA<0 && setIndexO<0 ) continue;
  for( int j=0; j<level->NTextures(); j++ )
  {
  Texture *texture=level->GetTexture(j);
  if( texture )
  {
  const char *name=texture->Name();
  const char *filename=strrchr(name,'\\');
  if( filename ) filename++;
  else filename=name;
  // make complete path from document path and filename

  CString texName=baseName+filename;
  if( QIFStream::FileExists(texName) )
  {
  if( texture->IsAlpha() )
  {
  if( setIndexA>=0 ) AddItem(setIndexA,texName);
  }
  else
  {
  if( setIndexO>=0 ) AddItem(setIndexO,texName);
  }
  }
  }
  }
  const char *filename=strrchr(name,'\\');
  if( filename ) filename++;
  else filename=name;

  char fileNormal[1024];
  strcpy(fileNormal,filename);
  while (strchr(fileNormal,' '))
  {
  *strchr(fileNormal,' ')='_';
  }
  if( setIndexA>=0 )
  {
  _textures[setIndexA].AutoArrange();
  _textures[setIndexA]._models.AddUnique(fileNormal);
  ret=setIndexA;
  }
  if( setIndexO>=0 )
  {
  _textures[setIndexO].AutoArrange();
  _textures[setIndexO]._models.AddUnique(fileNormal);
  ret=setIndexO;
  }
  }
  return ret;
  */
}

void CTexMergeDoc::DeleteItem(int set, int item)
{
  if( set<0 || set>=_textures.Size() ) return;
  TextureSet &dstSet=_textures[set];
  if( item<0 || item>=dstSet.Size() ) return;
  dstSet.Delete(item);
  NotifyData msg(set,item);
  SetModifiedFlag();
  UpdateAllViews(NULL,NMItemDeleted,&msg);
}

const TextureItem *CTexMergeDoc::GetItem(int set, int item)
{
  if( set<0 || set>=_textures.Size() ) return NULL;
  const TextureSet &dstSet=_textures[set];
  if( item<0 || item>=dstSet.Size() ) return NULL;
  return &dstSet[item];
}

void CTexMergeDoc::ModifyItem(int set, int item, const TextureItem & val)
{
  if( set<0 || set>=_textures.Size() ) return;
  TextureSet &dstSet=_textures[set];
  if( item<0 || item>=dstSet.Size() ) return;
  TextureItem &dst=dstSet[item];
  // keep x,y in valid range
  bool change=false;
  if( strcmp(dst.GetTexture(),val.GetTexture()) )
  {
    dst.SetTexture(val.GetTexture());
    change=true;
  }
  int oldScale=dst.scale;
  int oldAngle=dst.angle;
  int oldX=dst.x;
  int oldY=dst.y;
  dst.angle=val.angle;
  dst.scale=val.scale;
  dst.x=val.x;
  dst.y=val.y;
  while( dst.UsedW()>dstSet._w ) dst.scale--;
  while( dst.UsedH()>dstSet._h ) dst.scale--;
  int w=dst.UsedW();
  int h=dst.UsedH();
  if( dst.x<0 ) dst.x=0;
  if( dst.y<0 ) dst.y=0;
  if( dst.x>dstSet._w-w ) dst.x=dstSet._w-w;
  if( dst.y>dstSet._h-h ) dst.y=dstSet._h-h;
  dst.x+=7;
  dst.y+=7;
  dst.x&=~15;
  dst.y&=~15;
  if( dst.x!=oldX || dst.y!=oldY || dst.scale!=oldScale || dst.angle!=oldAngle )
  {
    change=true;
  }
  if( change )
  {
    SetModifiedFlag();
    NotifyData msg(set,item);
    UpdateAllViews(NULL,NMItemChanged,&msg);
  }
}

int CTexMergeDoc::InsertItem(int set, const TextureItem & val)
{
  if( set<0 || set>=_textures.Size() ) return -1;
  TextureSet &dstSet=_textures[set];
  int item=dstSet.Add(val);  
  SetModifiedFlag();
  NotifyData msg(set,item);
  UpdateAllViews(NULL,NMItemChanged,&msg);
  return item;
}

CString CTexMergeDoc::GetTexturePath() const
{
  CTexMergeApp *app=static_cast<CTexMergeApp *>(AfxGetApp());
  return app->GetTexturePath();
}

CString CTexMergeDoc::GetMergedPath() const
{
  CTexMergeApp *app=static_cast<CTexMergeApp *>(AfxGetApp());
  if (_mergedPath.GetLength()<=0)
  {
    _mergedPath = app->GetMergedPath();
  }
  return _mergedPath;
}

void CTexMergeDoc::SetMergedPath( CString path )
{
  CTexMergeApp *app=static_cast<CTexMergeApp *>(AfxGetApp());
  // replace all file ... prefixes with corresponding one
  app->SetMergedPath(path);
  if (_mergedPath!=path)
  {
    SetModifiedFlag(true);
  }
  _mergedPath = path;
}

void CTexMergeDoc::CopySetToClipboard( int index )
{
  TextureSet *set=GetTextureSet(index);
  if( !set ) return;
  _clipboard=*set;
}
void CTexMergeDoc::MergeSetFromClipboard( int index )
{
  TextureSet *set=GetTextureSet(index);
  if( !set ) return;
  // perform merge
  for( int i=0; i<_clipboard.Size(); i++ )
  {
    set->Add(_clipboard[i]);
  }
  for( int i=0; i<_clipboard._models.Size(); i++ )
  {
    set->_models.Add(_clipboard._models[i]);
  }
  set->AutoArrange(AAM_Grow);
  NotifyData msg(index,-1);
  SetModifiedFlag();
  UpdateAllViews(NULL,NMSetChanged,&msg);
}

bool BrowseForFolder( HWND parent, CString title, CString &path );

bool CTexMergeDoc::ExportMerged(const char *path, HWND status)
{
  CString base=path;
  for( int i=0; i<_textures.Size(); i++ )
  {
    const TextureSet &set=_textures[i];    
    CString model = set._name;
    CString pathname = base + model;
    CString text;
    if (status)
    {
      text.Format("Exporting set %d of %d",i+1,_textures.Size());
      ::SetWindowText(status,text);
      ::UpdateWindow(status);
    }
    bool res=set.Export(pathname,_tgaOutput,&_ROCheck);
    if (status && !res)
    {
      CString errorMessage;
      errorMessage.Format("Cannot write %s",(const char *)pathname);
      AfxMessageBox(errorMessage,MB_OK|MB_ICONERROR);
    }
    if (res==false) return false;
  }
  return true;
}

void CTexMergeDoc::OnFileExport() 
{
  OnFileSave();
  if (GetPathName()=="") return;
  // use picture library to export all texture sets

  CString base=GetMergedPath();
  if( base.IsEmpty() ) base=GetTexturePath();

  if( !BrowseForFolder(NULL,"Export merged textures",base) ) return;
  CWaitCursor wait;  ;
  SetMergedPath(base);
  base+="\\";
  CRect rc;
  theApp.m_pMainWnd->GetClientRect(&rc);
  HWND status=CreateWindow("STATIC","",WS_CHILD|SS_CENTER|SS_CENTERIMAGE|WS_VISIBLE|WS_BORDER,
    (rc.right-200)/2,(rc.bottom-50)/2,200,50,*theApp.m_pMainWnd,NULL,AfxGetInstanceHandle(),NULL);
  ExportMerged(base,status);
  DestroyWindow(status);
}

void CTexMergeDoc::OnFileMerged() 
{
  // use picture library to export all texture sets
  CString base=GetMergedPath();
  if( base.IsEmpty() ) base=GetTexturePath();

  if( !BrowseForFolder(NULL,"Select merged textures folder",base) ) return;
  SetMergedPath(base);	
}

void CTexMergeDoc::OnFileTgaoutput()
{
  _tgaOutput=!_tgaOutput;
}

void CTexMergeDoc::OnUpdateFileTgaoutput(CCmdUI *pCmdUI)
{
  pCmdUI->SetCheck(_tgaOutput);
}

static void CreateListOfFilesInPtm(const char *basePath, SccFunctions *scc,const ParamEntry *paramFile, AutoArray<RString>& out)
{
  int cnt=paramFile->GetEntryCount();
  for (int i=0;i<cnt;i++)
  {
    const ParamEntry& entry=paramFile->GetEntry(i);
    if (entry.IsClass()) CreateListOfFilesInPtm(basePath,scc,&entry,out);
    if (entry.IsTextValue())
    {
      RString base=basePath;
      RString text=base+entry.GetValue();
      if ((scc==NULL && _access(text,0)==0) || (scc!=NULL && scc->UnderSSControl(text)))
        out.Append()=text;        
    }
  }
}

void CTexMergeDoc::GetLatestVersion(ParamFile *paramFile)
{
  AutoArray<RString> list;
  CreateListOfFilesInPtm(GetTexturePath(),_curSS,paramFile,list);
  const char **ptrList=(const char **)alloca(list.Size()*sizeof(const char *));
  for (int i=0;i<list.Size();i++) ptrList[i]=list[i].Data();
  _curSS->GetLatestVersion(ptrList,list.Size());
}

void CTexMergeDoc::EnsureSSOpened()
{
  if (_curSS==NULL)
  {
    MsSccProject *project=new MsSccProject(MSSCCI,NULL,*theApp.m_pMainWnd);
    if (project->IsValid())
    {
      char username[256];
      _curSS=project;
      DWORD usernamesize=sizeof(username);
      GetUserName(username,&usernamesize);
      _curSS->ChooseProjectEx(NULL,NULL,NULL,username);
      if (_curSS->Opened())
      {
        _ROCheck.SetScc(_curSS);
        UpdateFilesSSStatus();
        return;
      }
    }
    else
      delete project;
    _curSS=NULL;
  }
}

void CTexMergeDoc::UpdateFilesSSStatus()
{
  const char *curfile=this->GetPathName();
  if (_curSS==NULL || curfile[0]==0) _ssFileState=-1;
  else if (!_curSS->UnderSSControl(curfile)) _ssFileState=-1;
  else if (_curSS->CheckedOut(curfile)) _ssFileState=1;
  else _ssFileState=0;
}

void CTexMergeDoc::OnFileOpenfromsourcesafe()
{
  bool willopen=_curSS==NULL;
  EnsureSSOpened();
  if (_curSS==NULL) return;
  SRef<SccFunctions> newscc=_curSS->Clone();
  Pathname curpath=GetPathName();
  if (willopen || newscc->ChooseProjectEx(NULL,curpath.GetDirectoryWithDrive())==0)
  {
    newscc->GetLatestVersionDir(newscc->GetLocalPath(),false);
    CFileDialog fdlg(TRUE,NULL,NULL,OFN_HIDEREADONLY|OFN_FILEMUSTEXIST,"PTM files|*.ptm||");
    fdlg.m_ofn.lpstrInitialDir=curpath.GetDirectoryWithDrive();
    if (fdlg.DoModal()==IDOK)
    {
      theApp.OpenDocumentFile(fdlg.GetPathName());
    }
  }
}

void CTexMergeDoc::OnUpdateFileOpenfromsourcesafe(CCmdUI *pCmdUI)
{
  pCmdUI->Enable(MSSCCI.IsLoaded());
}

void CTexMergeDoc::OnFileSaveandcheckin()
{
  DlgCheckInComments dlg;
  if (dlg.DoModal()==IDOK)
  {
    OnSaveDocument(GetPathName());
    if (_curSS->UnderSSControl(GetPathName())==false) _curSS->Add(GetPathName());
    _curSS->CheckIn(GetPathName(),dlg.vComments);
    if (dlg.vKeepCheckOut) _curSS->CheckOut(GetPathName());
    ParamFile pfile;
    Save(pfile,GetPathName());
    AutoArray<RString> list;
    CreateListOfFilesInPtm(GetTexturePath(),NULL,&pfile,list);
    const char **ptrList=(const char **)alloca(list.Size()*sizeof(const char *));
    for (int i=0;i<list.Size();i++) ptrList[i]=list[i].Data();
    dlg.vComments=CString(Pathname::GetNameFromPath(GetPathName()))+":"+dlg.vComments;
    _curSS->Add(ptrList,list.Size(),dlg.vComments);
    _curSS->CheckIn(ptrList,list.Size(),dlg.vComments);
    UpdateFilesSSStatus();
  }
}

void CTexMergeDoc::OnUpdateFileSaveandcheckin(CCmdUI *pCmdUI)
{
  pCmdUI->Enable(this->_ssFileState!=-1);
}

void CTexMergeDoc::OnSourcesafeothercommandsCheckout()
{
  _curSS->CheckOut(GetPathName());
  OnOpenDocument(GetPathName());   
}

void CTexMergeDoc::OnUpdateSourcesafeothercommandsCheckout(CCmdUI *pCmdUI)
{
  pCmdUI->Enable(this->_ssFileState==0);
}

void CTexMergeDoc::OnSourcesafeGetlatestversion()
{
  ParamFile pfile;
  Save(pfile,GetPathName());
  EnsureSSOpened();
  if (_curSS!=NULL)
    GetLatestVersion(&pfile); 
}

void CTexMergeDoc::OnUpdateSourcesafeGetlatestversion(CCmdUI *pCmdUI)
{
  pCmdUI->Enable(MSSCCI.IsLoaded() && GetPathName().GetLength());  
}

void CTexMergeDoc::OnSourcesafeothercommandsHistory()
{
  SCCRTN rtn=_curSS->History(GetPathName());
  if (rtn==5) OnOpenDocument(GetPathName());
}

void CTexMergeDoc::OnUpdateSourcesafeothercommandsHistory(CCmdUI *pCmdUI)
{
  pCmdUI->Enable(this->_ssFileState!=-1);  
}

void CTexMergeDoc::OnSourcesafeothercommandsUndocheckout()
{
  _curSS->UndoCheckOut(GetPathName());
  OnOpenDocument(GetPathName());
}

void CTexMergeDoc::OnUpdateSourcesafeothercommandsUndocheckout(CCmdUI *pCmdUI)
{
  pCmdUI->Enable(this->_ssFileState==1);  
}

void CTexMergeDoc::OnSourcesafeDisconnect()
{
  _curSS=NULL;
  UpdateFilesSSStatus();
}

void CTexMergeDoc::OnUpdateSourcesafeDisconnect(CCmdUI *pCmdUI)
{
  pCmdUI->Enable(_curSS!=NULL);
}


static void CalculateTransform(float x, float y, float sizex, float sizey, int angle, float mainszx, float mainszy,float transform[3][2])
  {  
  float a,b,c,d;
  ASSERT(sizex!=0 && sizey!=0);
  switch (angle)
    {
    case 0:a=sizex;b=0;c=0;d=sizey;break;
    case 1:y+=sizex-1;a=0;b=-sizex;c=sizey;d=0;break;
    case 2:x+=sizex-1;y+=sizey-1;a=-sizex;d=-sizey;c=b=0;break;
    case 3:x+=sizey-1;a=0;b=sizex;c=-sizey;d=0;break;
    default:a=b=c=d=0;
    }
  float mx=1/mainszx;
  float my=1/mainszy;
  transform[0][0]=a*mx;
  transform[0][1]=b*my;
  transform[1][0]=c*mx;
  transform[1][1]=d*my;
  transform[2][0]=x*mx;
  transform[2][1]=y*my;
  }

void TextureSet::CalculateTransform(int item, float transform[3][2])
{
  TextureItem &info=(*this)[item];
  float size=info.GetScaleCoef();
  ::CalculateTransform(info.x,info.y,info.w*size,info.h*size,info.angle,_w,_h,transform);
}

int TextureSet::FindTextureByUV(float u, float v)
{
  int posu=toInt(u*_w);
  int posv=toInt(v*_h);
  for (int i=0;i<Size();i++)
  {
    TextureItem &info=(*this)[i];
    if (info.x<=posu && info.y<=posv && info.x+info.w>posu && info.y+info.h>posv)
    {
      return i;
    }
  }
  return -1;
}

void CTexMergeDoc::UpdateSetName(void)
{
}

void CTexMergeDoc::KeepSize(int setid)
{
  TextureSet *set=GetTextureSet(setid);
  for (int i=0;i<set->Size();i++) if ((*set)[i].IsSelected())
  {
    (*set)[i].importance*=2;
  }
}

static RString CorrectPropName(RString name)
{
  if (isdigit(name[0])) name=RString("_")+name;
  char *c=name.MutableData();
  for (int i=0,cnt=strlen(c);i<cnt;i++)
  {
    if (!isalnum(c[i]) && c[i]>0) c[i]='_';
  }
  return name;
}


void CTexMergeDoc::SelectionChangeTextureProp(int setid,const TextureProp &prop)
{
  RString propname=CorrectPropName(prop.name);
  TextureSet *set=GetTextureSet(setid);
  for (int i=0;i<set->Size();i++) if ((*set)[i].IsSelected())
  {
    AutoArray<TextureProp> &props=(*set)[i]._properties;
    int i;
    for (i=0;i<props.Size();i++) if (stricmp(props[i].name,propname)==0)
    {
      props[i]=TextureProp(propname,prop.value);
      break;
    }
    if (i==props.Size()) props.Append(TextureProp(propname,prop.value));
  }  
}
void CTexMergeDoc::SelectionRemoveTextureProp(int setid,const TextureProp &prop)
{
  RString propname=CorrectPropName(prop.name);
  TextureSet *set=GetTextureSet(setid);
  for (int i=0;i<set->Size();i++) if ((*set)[i].IsSelected())
  {
    AutoArray<TextureProp> &props=(*set)[i]._properties;
    int i;
    for (i=0;i<props.Size();i++) if (stricmp(props[i].name,propname)==0)
    {
      props.Delete(i);
      break;
    }
  }  
}

void CTexMergeDoc::SelectionSetImportance(int setid, float importance)
{
  TextureSet *set=GetTextureSet(setid);
  for (int i=0;i<set->Size();i++) if ((*set)[i].IsSelected())
  {
    (*set)[i].importance=importance;
  }
}

void CTexMergeDoc::SelectionSetSource(int setid, const char *source)
{
  TextureSet *set=GetTextureSet(setid);
  for (int i=0;i<set->Size();i++) if ((*set)[i].IsSelected())
  {
    (*set)[i].SetTexture(CString(source));
  }
}

void CTexMergeDoc::SelectionSetScale(int setid, int scale)
{
  TextureSet *set=GetTextureSet(setid);
  for (int i=0;i<set->Size();i++) if ((*set)[i].IsSelected())
  {
    (*set)[i].scale=scale;
  }
}

void CTexMergeDoc::SelectionMove(int setid, int logX, int logY, int *focus)
{
  TextureSet *set=GetTextureSet(setid);
  int setsize=set->Size();
  int fc=focus?*focus:-1;
  for (int i=0;i<setsize;i++) if ((*set)[i].IsSelected())
  {
    const TextureItem *item=GetItem(setid,i);
    TextureItem mod=*item;
    mod.x=item->x+logX;
    mod.y=item->y+logY;
    ModifyItem(setid,i,mod);
    if ((logX!=0 || logY!=0) && !set->IsFree(item,item->x,item->y,item->UsedH(),item->UsedW()))
    {
        // item should be brought up
        mod=*item;
        DeleteItem(setid,i);
        int p=InsertItem(setid,mod);
        if (fc==i) fc=p;
        else if (fc>i) fc--;
        SelectItem(setid,p,true);
        setsize--;
        i--;      
    }    
  }
  if (focus) *focus=fc;
}


void CTexMergeDoc::SetChangeTextureProp(int setid, const TextureProp &prop)
{
  RString propname=CorrectPropName(prop.name);
  TextureSet *set=GetTextureSet(setid);
  AutoArray<TextureProp> &props=(set->_properties);
  int i;
  for (i=0;i<props.Size();i++) if (stricmp(props[i].name,propname)==0)
  {
    props[i]=TextureProp(propname,prop.value);
    break;
  }
  if (i==props.Size()) props.Append(TextureProp(propname,prop.value));

}
void CTexMergeDoc::SetRemoveTextureProp(int setid, const TextureProp &prop)
{
  RString propname=CorrectPropName(prop.name);
  TextureSet *set=GetTextureSet(setid)  ;
  AutoArray<TextureProp> &props=(set->_properties);
  int i;
  for (i=0;i<props.Size();i++) if (stricmp(props[i].name,propname)==0)
  {
    props.Delete(i);
    break;
  }
}
void CTexMergeDoc::ChangeSetName(int setid, const char *name)
{
  TextureSet *set=GetTextureSet(setid);
  set->_name=name;
}
void CTexMergeDoc::ChangeSetDimensions(int setid, int width, int height)
{
  TextureSet *set=GetTextureSet(setid);
  set->_w=width;
  set->_h=height;
}
void CTexMergeDoc::ChangeSetBgColor(int setid, float *bgcolor)
{
  TextureSet *set=GetTextureSet(setid);
  for (int i=0;i<4;i++)
    if (bgcolor[i]<0) set->_bgcolor[i]=0;
    else if (bgcolor[i]>1) set->_bgcolor[i]=1;
    else set->_bgcolor[i]=bgcolor[i];
}


static void CreateCharCode(char *buff, int value)
{
  const int dim='z'-'a'+1;
  while (value)
  {
    *buff='a'+(value % dim); 
    buff++;
    value/=dim;
  }
  *buff=0;
}

void CTexMergeDoc::AutoArrangeMulti(int set, int additionalSets, bool optimize, bool keepsel)
{  
  if( set<0 || set>=_textures.Size() ) return;
  TextureSet &dstSet=_textures[set];
  
  int lastH=dstSet._h;
  dstSet._h=(additionalSets+1)*lastH;
  dstSet.AutoArrange2();

  int sdifference=0;
  bool keeped=false;

  if (keepsel)
    for (int i=0;i<dstSet.Size();i++) 
    {
      TextureItem &itm=dstSet[i];
      int bott=itm.y+itm.UsedH();
      if (itm._selected && bott>lastH)
      {
        keeped=true;
        itm.y=lastH-itm.UsedH();
        sdifference+=itm.UsedH()*itm.UsedW();
        if (sdifference>0)
        {
          for (int j=0;j<dstSet.Size();j++)
          {
            TextureItem &itm2=dstSet[j];
            if (itm2.y+itm2.UsedH()<=lastH && !itm2._selected)
            {
              int section=(bott-1)/lastH;
              itm2.y=lastH*(section+1)-1-itm2.UsedH();
              sdifference-=itm2.UsedH()*itm2.UsedW();
              if (sdifference<=0) break;
            }
          }
        }
      }
    }
  
  int firstSet=_textures.Size();
  if (keeped)  
    _textures.Resize(_textures.Size()+1);
  else
    _textures.Resize(_textures.Size()+additionalSets);
  for (int i=firstSet;i<_textures.Size();i++)
  {
    _textures[i]._w=dstSet._w;
    _textures[i]._h=lastH;
    _textures[i]._name=dstSet._name;
    _textures[i]._properties=dstSet._properties;
    for (int j=0;j<4;j++) _textures[i]._bgcolor[j]=dstSet._bgcolor[j];
    _textures[i]._alpha=dstSet._alpha;
  }  

  for (int i=0;i<dstSet.Size();i++)
  {
    TextureItem &itm=dstSet[i];
    int section=(itm.y+itm.UsedH()-1)/lastH;
    if (section>0)
    {
      if (firstSet+section-1>=_textures.Size())
        section=_textures.Size()-1;
      _textures[firstSet+section-1].Append(itm);
      dstSet.Delete(i);
      i--;
    }
    else if (section<0)
    {
      dstSet.Delete(i);
      i--;
    }    
  }

  dstSet._h=lastH;
  AutoArrangeMode mode;
  if (optimize) mode=AAM_ShrinkOnly;else mode=AAM_KeepSize;
  dstSet.AutoArrange(mode);
  for (int i=firstSet;i<_textures.Size();i++)
    if (_textures[i].Size())
      _textures[i].AutoArrange(mode);
    else
      _textures.Delete(i--);

  RString srcName=dstSet._name;

  if (keeped && firstSet!=_textures.Size() && additionalSets>1)
  {
    AutoArrangeMulti(firstSet,additionalSets-1,optimize,false);
  }

  dstSet._name=RString(srcName,"_a");
  int idx=1;
  for (int i=firstSet;i<_textures.Size();i++)
  {
    char buff[50];
    CreateCharCode(buff+1,idx++);
    buff[0]='_';
    _textures[i]._name=RString(srcName,buff);        
  }


  NotifyData msg(set,-1);
  SetModifiedFlag();
  UpdateAllViews(NULL,NMSetChanged,&msg);
}

void CTexMergeDoc::MergeToOne(const Array<int> &list)
{
  if (list.Size()<2) return;
  int baseSet=list[0];
  TextureSet &base=_textures[baseSet];
  for (int i=1;i<list.Size();i++)
  {
    TextureSet &set=_textures[list[i]];
    for (int j=0;j<set.Size();j++)
    {
      base.Append(set[j]);
    }
  }
  AutoArray<int,MemAllocStack<int,32> > copyList;
  copyList.Append(list);
  copyList.Delete(0); ///first item should be not deleted
  copyList.QSortBin(TextureSetItemDownUp);
  for (int i=0;i<copyList.Size();i++) _textures.Delete(copyList[i]);

  NotifyData msg(baseSet,-1);
  SetModifiedFlag();
  UpdateAllViews(NULL,NMSetChanged,&msg);
}