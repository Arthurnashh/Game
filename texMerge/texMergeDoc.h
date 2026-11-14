// texMergeDoc.h : interface of the CTexMergeDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_TEXMERGEDOC_H__7E5B94A4_7BE3_11D3_8373_00A0C9DF4D61__INCLUDED_)
#define AFX_TEXMERGEDOC_H__7E5B94A4_7BE3_11D3_8373_00A0C9DF4D61__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/*
class QOStream;
class QIStream;

#include "..\lib\pacText.hpp"
*/

#include <El/ParamFile/paramFile.hpp>
#include ".\texmergerocheck.h"
#include "WinDraw.h"



inline int Zoom( int val, int zoom )
{
	if( zoom>=0 ) return val<<zoom;
	return val>>-zoom;
}

struct TextureProp
{
  RString name;
  RString value;
  TextureProp() {}
  TextureProp(RString name,RString value):name(name),value(value) {}
};

TypeIsMovable(TextureProp);

/*
class PacLevelSys: public RemoveLinks,public PacLevel,public CLRefLink
{
};
*/
struct TextureItem
{
private:
	CString _name; // path name (relative to current directory)
	mutable Link<PictureData> _tex;
public:
  RString _itemName;
	int x,y; // position
	int scale; // scale (can be negative - zoom out)
	int angle;
	float importance;
	// following data is calculated from texture file
	int w,h; // texture w,h
	int checksum;
  AutoArray<TextureProp> _properties;
  bool _selected;
public:
	TextureItem();

	void Copy( const TextureItem &item );
	void operator = ( const TextureItem &item ) {
    if (this == &item) return; // FLY
    Clear();
    Copy(item);
  }
	TextureItem( const TextureItem &item ) {Copy(item);}

	void Clear();
	void Load( const char *basepath, ParamEntryPar f );
	void Save( const char *basepath, ParamClassPtr f ) const;
	void SetTexture( CString tex );
	CString GetTexture() const {return _name;}

  float GetScaledImportance() const;
  
  float GetScaleCoef() const
  {
    return scale>=0 ? (1<<scale) : 1.0/(1<<-scale);
  }
	int UsedW() const; // area used in set
	int UsedH() const;

	PictureData *GetTex() const; // load if neccessary
	void Compact(); // unload if neccessary

  void Select(bool sel) {_selected=sel;}
  bool IsSelected() const {return _selected;}
};

TypeIsGeneric(TextureItem);

const int MinSetDimension=64;
const int MaxSetDimension=2048;

enum AutoArrangeMode {AAM_KeepSize, AAM_ShrinkOnly, AAM_Grow};

// do not know anything about CString structure
// maybe it is movable?

TypeIsGeneric(CString);


RString BuildClassName(const char *name);

class TextureSet: public AutoArray<TextureItem>
{
	typedef AutoArray<TextureItem> base;

public:
  //! Name of the class associated with the texture set
  RString _name;
	int _w,_h; // max. dimensions
	FindArray<CString> _models; // which models was it created from
	bool _alpha;
  AutoArray<TextureProp> _properties;
  float _bgcolor[4];

	TextureSet();
	void Clear();
	void Load( const char *basepath, ParamEntryPar f );
	void Save
	(
		const char *basepath, const char *mergedpath, ParamClassPtr f, int index, bool tgaOutput
	) const;

  bool IsPaa() const;
	void AutoArrange(AutoArrangeMode mode=AAM_Grow);
  bool AutoArrange2();  

  int FindTexture(const char *textureName)
  {
    for (int i=0;i<Size();i++)
    {
      if (stricmp((*this)[i].GetTexture(),textureName)==0) return i;
    }
    return -1;
  }
  int FindMostImportant(int maxScale) const;
  int FindLeastImportant(int minWH) const;
	bool IsFree( const TextureItem *exclude, int x, int y, int w, int h ) const;
	void FindPosition( int &x, int &y, int w, int h ) const;
	bool ContainsExact( const TextureItem &find ) const;
	int Contains( const TextureItem &find, int *index=NULL ) const; // returns angle (0..3)

	CString BuildName( int maxLen, char delim ) const;
	CString BuildParamName() const {return BuildName(32,'X');}
	CString BuildFileName( int index ) const
	{
		char buf[256];
		sprintf(buf,"%05d",index);
		return CString(buf)+BuildName(15,'&');
	}
	bool Export( const char *filename, bool tgaOutput,IROCheck *_rocheck ) const;
    ///Calculates transformation of item
    /**
    @param item, index of item
    @transform pointer to array of three lines by two numbers [3x2] than will receive transform
    */
    void CalculateTransform(int item, float transform[3][2]);
    int FindTextureByUV(float u, float v);

};

TypeIsMovable(TextureSet);

enum NotifyMessage
{
	NMReset,
	NMSetDeleted,
	NMItemDeleted,
	NMSetInserted,
	NMItemInserted,
	NMSetChanged,
	NMItemChanged,
};

class NotifyData: public CObject
{
	public:
	int _set; // which set is affected
	int _item; // which item is affected

	NotifyData( int set, int item ){_set=set,_item=item;}
};

class CTexMergeDoc : public CDocument
{

protected: // create from serialization only
	CTexMergeDoc();
	DECLARE_DYNCREATE(CTexMergeDoc)


// Attributes
public:
    bool _tgaOutput;
    SRef<SccFunctions> _curSS;
    TexMergeROCheck _ROCheck;
    char _ssFileState; // -1  = not controlled, 0 = checked in, 1 = checked out
private:
	AutoArray<TextureSet> _textures;
	// TODO: use Win clipboard instead
	mutable CString _mergedPath;
	TextureSet _clipboard;

// Operations
public:
	const TextureSet *GetTextureSet( int i ) const
	{
		if( i>=0 && i<=_textures.Size() ) return &_textures[i];
		return NULL;
	}
	TextureSet *GetTextureSet( int i )
	{
		if( i>=0 && i<=_textures.Size() ) return &_textures[i];
		return NULL;
	}
	int GetTextureSetCount() const {return _textures.Size();}
  int GetTexturesCount(int setid) const 
    {
      const TextureSet *set=GetTextureSet(setid);
        if (set) return set->Size();
        else return 0;
    }
  

// Overrides
  virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
  virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
  
  virtual BOOL IsModified();
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTexMergeDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	void ModifyItem( int set, int item, const TextureItem &val );
	int InsertItem( int set, const TextureItem &val );
	const TextureItem *GetItem( int set, int item );
	void DeleteItem( int set, int item );
	int AddItem( int set, const TextureItem &val );

	int AddItem( int set, const char *name );
	int AddModel( const char *name );

	void LoadClipboard( TextureSet &set );
	void SaveClipboard( const TextureSet &set );

	void CopySetToClipboard( int index );
	void MergeSetFromClipboard( int index );

	CString GetTexturePath() const;

	CString GetMergedPath() const;
	void SetMergedPath( CString path );

	void AutoArrange(int set, AutoArrangeMode mode);
  void AutoArrangeMulti(int set, int additionalSets, bool optimize, bool keepsel=false);
	int DuplicateSet( int i );
	void DeleteSet( int i );
	int AddTextureSet();
	virtual ~CTexMergeDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

    bool FindTexture(const char *texName,TextureSet *& set, int &index)
    {
      for (int i=0;i<_textures.Size();i++)
      {
        int idx=_textures[i].FindTexture(texName);
        if (idx!=-1) {set=&_textures[i];index=idx;return true;}
      }
     return false;
    }
    int FindByMergedTexture(const char *mergedName)
    {
      for (int i=0;i<_textures.Size();i++)
        if (stricmp(_textures[i]._name,mergedName)==0) return i;
      return -1;
    }

    void GetLatestVersion(ParamFile *paramFile);
    void EnsureSSOpened();
    void UpdateFilesSSStatus();    

    bool ExportMerged(const char *path, HWND status=NULL);

    void SelectItem(int set, int item, bool sel);
    bool IsSelected(int setid, int item);

protected:

	void Load( const ParamFile &f );
	void Save(ParamFile &f, const char *name);

// Generated message map functions
protected:
	//{{AFX_MSG(CTexMergeDoc)
	afx_msg void OnFileExport();
	afx_msg void OnFileMerged();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnFileTgaoutput();
  afx_msg void OnUpdateFileTgaoutput(CCmdUI *pCmdUI);
  afx_msg void OnFileOpenfromsourcesafe();
  afx_msg void OnUpdateFileOpenfromsourcesafe(CCmdUI *pCmdUI);
  afx_msg void OnFileSaveandcheckin();
  afx_msg void OnUpdateFileSaveandcheckin(CCmdUI *pCmdUI);
  afx_msg void OnSourcesafeothercommandsCheckout();
  afx_msg void OnUpdateSourcesafeothercommandsCheckout(CCmdUI *pCmdUI);
  afx_msg void OnSourcesafeGetlatestversion();
  afx_msg void OnUpdateSourcesafeGetlatestversion(CCmdUI *pCmdUI);
  afx_msg void OnSourcesafeothercommandsHistory();
  afx_msg void OnUpdateSourcesafeothercommandsHistory(CCmdUI *pCmdUI);
  afx_msg void OnSourcesafeothercommandsUndocheckout();
  afx_msg void OnUpdateSourcesafeothercommandsUndocheckout(CCmdUI *pCmdUI);
  afx_msg void OnSourcesafeDisconnect();
  afx_msg void OnUpdateSourcesafeDisconnect(CCmdUI *pCmdUI);
  void UpdateSetName(void);

  ///Keeps size for auto arrange for all selected items of given set
  void CTexMergeDoc::KeepSize(int set);

  void SelectionChangeTextureProp(int set, const TextureProp &prop);
  void SelectionRemoveTextureProp(int set, const TextureProp &prop);
  void SelectionSetImportance(int set, float importance);
  void SelectionSetSource(int set, const char *source);
  void SelectionSetScale(int setid, int scale);
  void SelectionMove(int setid, int logX, int logY, int *focus);

  void SetChangeTextureProp(int set, const TextureProp &prop);
  void SetRemoveTextureProp(int set, const TextureProp &prop);
  void ChangeSetName(int set, const char *name);
  void ChangeSetDimensions(int set, int width, int height);
  void ChangeSetBgColor(int set, float *bgcolor);

  void MergeToOne(const Array<int> &list);


};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEXMERGEDOC_H__7E5B94A4_7BE3_11D3_8373_00A0C9DF4D61__INCLUDED_)
