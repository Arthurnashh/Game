// Add C preprocessor functionality to ParamFile

#include <El/elementpch.hpp>

#include "preprocC.hpp"
#include <Es/Common/win.h>

QIStream *StringPreprocessor::OnEnterInclude(const char *str)
{
  if (_stream)
  {
    RptF("#include directive not supported here");
    return NULL;
  }
  _stream = new QIStrStream(str, strlen(str));
  return _stream;
}

void StringPreprocessor::OnExitInclude(QIStream *stream)
{
  Assert(_stream);
  _stream = NULL;
}

void StringPreprocessor::AtBeginLine(QOStream &out)
{
}

RString ExtractPath(RString filename)
{
	const char *name = filename;
	const char *ptr = strrchr(name, '\\');
	if (ptr) return filename.Substring(0, ptr - name + 1);
	ptr = strrchr(name, '/');
	if (ptr) return filename.Substring(0, ptr - name + 1);
	return RString();
}

RString CombinePath(RString path, RString filename)
{
	// empty filename
	DoAssert(filename.GetLength() > 0)
	if (filename.GetLength() == 0) return RString();

	// TODO: allow .\ and ..\ in filename
	char start = filename[0];
//	DoAssert(start != '.');

  if (filename[0]=='\\' && filename[1]=='\\')
  {
    return filename;
  }
	if ((start == '\\' || start == '/'))
		// absolute path
		return filename.Substring(1, INT_MAX);
	
	return path + filename;
}

static int IsUnicode(QIStream &in)
{
  int c1 = in.get();
  if (in.eof())
  {
    in.seekg(0);
    return 0;
  }
  int c2 = in.get();
  if (in.eof())
  {
    in.seekg(0);
    return 0;
  }

  if (c1 == 0xff && c2 == 0xfe)
  {
    int c3 = in.get();
    if (in.eof())
    {
      in.seekg(2);
      return 2;
    }
    if (c3==0)
    {
      int c4 = in.get();
      if (in.eof())
      {
        in.seekg(2);
        return 2;
      }
      in.seekg(2);
      if (c4==0) return 1; // FF FE 00 00 ... UTF-32, little endian
    }
    in.seekg(2);
    return 2;
  }
  if (c1 == 0xfe && c2 == 0xff)
  {
    RptF("Big-endian unicode not supported");
    return 3;
  }
  if (c1 == 0xef && c2 == 0xbb)
  {
    int c3 = in.get();
    if (!in.eof() && c3 == 0xbf) return 0; // UTF-8 prefix
  }
  in.seekg(0);
  return 0;
}

class FileWideCharToMultiByte
{
protected:
  QOStream &_out;
  int       _uniType;

public:
  FileWideCharToMultiByte(QOStream &out, int unicodeType) : _out(out), _uniType(unicodeType) {}

  void operator ()(char *buf, int size)
  {
    WCHAR *src = (WCHAR *)(buf);
    int srcLen = size / sizeof(WCHAR);
#ifdef _WIN32
    // check the size of the needed buffer
    int dstLen = WideCharToMultiByte(CP_UTF8, 0, src, srcLen, NULL, 0, NULL, NULL);
    // allocate the buffer, convert and write to the stream
    Buffer<char> dst(dstLen);
    WideCharToMultiByte(CP_UTF8, 0, src, srcLen, dst.Data(), dstLen, NULL, NULL);
    _out.write(dst.Data(), dstLen);
#else
    if (_uniType==1) //UTF32
    {
      // check the size of the needed buffer
      int dstLen = WideCharToMultiByte(CP_UTF8, 0, src, srcLen, NULL, 0, NULL, NULL);
      // allocate the buffer, convert and write to the stream
      Buffer<char> dst(dstLen);
      WideCharToMultiByte(CP_UTF8, 0, src, srcLen, dst.Data(), dstLen, NULL, NULL);
      _out.write(dst.Data(), dstLen);
    }
    else // UTF-16LE
    {
      int dstLen = UTF16ToUTF8(buf, size, NULL, 0);
      // allocate the buffer, convert and write to the stream
      Buffer<char> dst(dstLen);
      UTF16ToUTF8(buf, size, dst.Data(), dstLen);
      _out.write(dst.Data(), dstLen);
    }
#endif
  }
};

QIStream *FilePreprocessor::OnEnterInclude(const char *filename)
{
	RString path;
  int n = _stack.Size();
  if (n > 0) path = ExtractPath(_stack[n - 1].filename);
	RString fullname = CombinePath(path, filename);

	if (!QFBankQueryFunctions::FileExists(fullname))
  {
    ErrorMessage("Include file %s not found.", cc_cast(fullname));
    return NULL;
  }
	
	_stack.Add(fullname);
	
  QIFStreamB *direct = new QIFStreamB();
  direct->AutoOpen(fullname);
  // we will need the whole file
  // it is therefore better to load it in one piece
  direct->PreReadSequential();

#ifdef _WIN32  
  HandleUnicode handleUnicode(*direct);  
  QIStream *stream = handleUnicode.CreateStream();  
  if (stream != direct)  
  {    
    // new stream created inside handleUnicode.CreateStream()    
    delete direct;    
    direct = NULL;  
  }
#else  
  QIStream *stream = direct;
  int uniType = IsUnicode(*direct);
  if (uniType)
  {
    // convert file to UTF-8
    QOStrStream converted;
    FileWideCharToMultiByte func(converted, uniType);
    direct->Process(func);

    // use the output as an input stream
    QIStrStream *in = new QIStrStream();
    in->init(converted);
    stream = in;

    delete direct;
    direct = NULL;
  }
#endif
  
  if (_lineNumbers)
  {
    (*out) << "#line 1 \"" << fullname << "\"\n";
  }

  return stream;
}

void FilePreprocessor::OnExitInclude(QIStream *stream)
{
	if (stream) delete stream;

	// pop from stack
  int i = _stack.Size() - 1;
  Assert(i >= 0);
  if (i >= 0)
  {
    if (_lineNumbers && i > 0)
    {
      (*out) << Format("#line %d \"", _stack[i - 1].line) << _stack[i - 1].filename << "\"\n";
    }
    _stack.Delete(i);
  }
}

void FilePreprocessor::AtBeginLine(QOStream &out)
{
  int i = _stack.Size() - 1;
  Assert(i >= 0);
  if (i >= 0)
  {
    _stack[i].line++;
  }
}

RString FilePreprocessor::GetFilename() const
{
  int i = _stack.Size() - 1;
  if (i < 0) return RString();
  return _stack[i].filename;
}

int FilePreprocessor::GetLine() const
{
  int i = _stack.Size() - 1;
  if (i < 0) return 0;
  return _stack[i].line;
}

QIStream *BankPreprocessor::OnEnterInclude(const char *filename)
{
  RString path;
  int n = _stack.Size();
  if (n > 0) path = ExtractPath(_stack[n - 1].filename);
  RString fullname = CombinePath(path, filename);

  if (!_bank.FileExists(fullname))
  {
    ErrorMessage("Include file %s not found.", cc_cast(fullname));
    return NULL;
  }

  _stack.Add(fullname);
  QIFStreamB *direct = new QIFStreamB();
  direct->open(_bank, fullname);

#ifdef _WIN32  
  HandleUnicode handleUnicode(*direct);  
  QIStream *stream = handleUnicode.CreateStream();  
  if (stream != direct)  
  {    
    // new stream created inside handleUnicode.CreateStream()    
    delete direct;    
    direct = NULL;  
  }
#else  
  QIStream *stream = direct;
  int uniType = IsUnicode(*direct);
  if (uniType)
  {
    // convert file to UTF-8
    QOStrStream converted;
    FileWideCharToMultiByte func(converted,uniType);
    direct->Process(func);

    // use the output as an input stream
    QIStrStream *in = new QIStrStream();
    in->init(converted);
    stream = in;

    delete direct;
    direct = NULL;
  }
#endif

  if (_lineNumbers)
  {
    (*out) << "#line 1 \"" << fullname << "\"\n";
  }

  return stream;
}

void BankPreprocessor::OnExitInclude(QIStream *stream)
{
  if (stream) delete stream;

  // pop from stack
  int i = _stack.Size() - 1;
  Assert(i >= 0);
  if (i >= 0)
  {
    if (_lineNumbers && i > 0)
    {
      (*out) << Format("#line %d \"", _stack[i - 1].line) << _stack[i - 1].filename << "\"\n";
    }
    _stack.Delete(i);
  }
}

void BankPreprocessor::AtBeginLine(QOStream &out)
{
  int i = _stack.Size() - 1;
  Assert(i >= 0);
  if (i >= 0)
  {
    _stack[i].line++;
  }
}

RString BankPreprocessor::GetFilename() const
{
  int i = _stack.Size() - 1;
  if (i < 0) return RString();
  return _stack[i].filename;
}

int BankPreprocessor::GetLine() const
{
  int i = _stack.Size() - 1;
  if (i < 0) return 0;
  return _stack[i].line;
}

bool CPreprocessorFunctions::Preprocess(QOStream &out, const char *name, const Params &params)
{
	FilePreprocessor preprocessor(params);
	if (!preprocessor.Process(&out, name))
	{
    RString filename = preprocessor.GetFilename();
    if (filename.GetLength() > 0)
    {
      WarningMessage(
        "Preprocessor failed on file %s - error %d (source %s, line %d).",
        name, preprocessor.error, 
        cc_cast(filename), preprocessor.GetLine());
    }
    else
    {
      WarningMessage(
        "Preprocessor failed on file %s - error %d.",
        name, preprocessor.error);
    }
		return false;
	}
	return true;
}

bool CPreprocessorFunctions::Preprocess(QOStream &out, QFBank &bank, const char *name, const Params &params)
{
  BankPreprocessor preprocessor(bank, params);
  if (!preprocessor.Process(&out, name))
  {
    RString filename = preprocessor.GetFilename();
    if (filename.GetLength() > 0)
    {
      WarningMessage(
        "Preprocessor failed on file %s - error %d (source %s, line %d).",
        name, preprocessor.error, 
        cc_cast(filename), preprocessor.GetLine());
    }
    else
    {
      WarningMessage(
        "Preprocessor failed on file %s - error %d.",
        name, preprocessor.error);
    }
    return false;
//    ErrorMessage("Preprocessor failed on file %s - error %d.", name, preprocessor.error);
//    return false;
  }
  return true;
}
