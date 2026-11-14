// WFilePath.h: interface for the WFilePath class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WFILEPATH_H__82378BE0_E809_475A_BDD6_95882AA6DB61__INCLUDED_)
#define AFX_WFILEPATH_H__82378BE0_E809_475A_BDD6_95882AA6DB61__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class WFilePath:  public WString
  {
  WString drive;
  WString folder;
  WString filename;	
  public:
    static const char *GetNameFromPath(const char *path);
    bool SetExtension(const char *ext);
    WString& GetDrive() 
      {return drive;}
    WString& GetDirectory() 
      {return folder;}
    WString& GetFilename() 
      {return filename;}
    const char *GetExtension() 
      {return strrchr(filename,'.');}
    void SetDrive(const char *dr) 
      {drive=dr;InternalFormat();}
    void SetDirectory(const char *dr) 
      {
      folder=dr;
      if (folder=="" || folder[folder.GetLength()-1]!='\\') folder+='\\';
      InternalFormat();
      }
    void SetFilename(const char *dr) 
      {filename=dr;InternalFormat();}	
    WFilePath(const char *name=NULL);
    WFilePath& operator=(const CString& other) 
      {Parse(other);InternalFormat();return *this;}
    WFilePath& operator=(const char *other) 
      {Parse(other);InternalFormat();return *this;}
    void SetNull() 
      {WString::operator =("");}
    //WFilePath& operator=(char t) {CString::operator=(t);return *this;}
    
    
  protected:
    void Parse(const char *pathname);
    void InternalFormat();
  };

//--------------------------------------------------

#endif // !defined(AFX_WFILEPATH_H__82378BE0_E809_475A_BDD6_95882AA6DB61__INCLUDED_)
