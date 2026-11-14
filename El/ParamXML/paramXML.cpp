#include <El/elementpch.hpp>
#include <El/ParamFile/paramFile.hpp>
#include <El/XML/xml.hpp>

struct Attrib
{
  RStringB name;
  RStringB value;
};


class ProduceXML
{
  FILE *_f;
  int _indent;
  /// new line was just started  - indenting will be required
  bool _newline;

  public:
  ProduceXML(FILE *f);
  void OpenTagg(const char *name, Attrib *attrib=NULL, int nAttrib=0);
  void CloseTagg(const char *name, bool eol=true);
  void AddContent(const char *str);
  void Eol();
  void Indent();
  void AddComment(const char *str);
};

ProduceXML::ProduceXML(FILE *f)
:_f(f)
{
  _indent = 0;
  _newline = true;
  fprintf(f,"<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n");
}

void ProduceXML::Indent()
{
  if (!_newline) return;
  for (int i=0; i<_indent; i++)
  {
    fprintf(_f,"  ");
  }
  _newline = false;
}

void ProduceXML::OpenTagg(const char *name, Attrib *attrib, int nAttrib)
{
  Indent();
  fprintf(_f,"<%s",name);
  for (int i=0; i<nAttrib; i++)
  {
    fprintf(_f," %s=\"%s\"",cc_cast(attrib[i].name),cc_cast(attrib[i].value));
  }
  fprintf(_f,">");

  _indent++;
}

void ProduceXML::CloseTagg(const char *name, bool eol)
{
  _indent--;
  Indent();
  fprintf(_f,"</%s>",name);
  if (eol) Eol();
}

void ProduceXML::AddContent(const char *str)
{
  bool wasnewline = _newline;
  Indent();
  fprintf(_f,"%s",str);
  if (wasnewline) Eol();
}

void ProduceXML::AddComment(const char *str)
{
  Indent();
  fprintf(_f,"<!-- %s -->",str);
  Eol();
}

void ProduceXML::Eol()
{
  if (_newline) return;
  fprintf(_f,"\n");
  _newline = true;
}

static bool SaveXMLArrayItem(ProduceXML &f, const IParamArrayValue &val,bool lined)
{
  if (val.IsArrayValue())
  {
    Attrib attrib;
    attrib.name="type";
    attrib.value="array";
    if (lined) f.Eol();
    f.OpenTagg("item",&attrib,1);
    if (val.GetItemCount()>4) f.Eol();
    for (int i=0; i<val.GetItemCount(); i++)
    {
      SaveXMLArrayItem(f,*val.GetItem(i),val.GetItemCount()>4);
    }
    f.CloseTagg("item",lined);
  }
  else
  {
    f.OpenTagg("item");
    f.AddContent(val.GetValue());
    f.CloseTagg("item",lined);
  }
  return true;
}

static bool SaveXMLArray(ProduceXML &f,ParamEntryPar array, bool lined)
{
  for (int i=0; i<array.GetSize(); i++)
  {
    const IParamArrayValue &value = array[i];
    SaveXMLArrayItem(f,value,false);
    if (lined) f.Eol();
  }
  return true;
}

static bool SaveXML(ProduceXML &f, const ParamClass &cls, const char *name)
{
  if (name && *name)
  {
    const char *base = cls.GetBaseName();
    if (!base || !*base)
    {
      f.OpenTagg(name);
    }
    else
    {
      Attrib attrib;
      attrib.name="base";
      attrib.value=base;
      f.OpenTagg(name,&attrib,1);
    }
    f.Eol();
  }
  for (int i=0; i<cls.GetEntryCount(); i++)
  {
    ParamEntryVal entry = cls.GetEntry(i);
    if (entry.IsClass())
    {
      SaveXML(f,*entry.GetClassInterface(),entry.GetName());
    }
    else if (entry.IsArray())
    {
      Attrib attrib;
      attrib.name="type";
      attrib.value="array";
      f.OpenTagg(entry.GetName(),&attrib,1);
      if (entry.GetSize()>4) f.Eol();
      SaveXMLArray(f,entry,entry.GetSize()>4);
      f.CloseTagg(entry.GetName());
    }
    else
    {
      // not class, not array - should be a value
      f.OpenTagg(entry.GetName());
      RStringB val = entry.GetValue();
      if (val.GetLength()>40) f.Eol();
      f.AddContent(val);
      f.CloseTagg(entry.GetName());
    }
  }
  if (name && *name)
  {
    f.CloseTagg(name);
  }
  return true;
}

bool SaveXML(const ParamFile &cfg, FILE *f)
{
  ProduceXML prod(f);

  prod.AddComment("BIS Config File");
  bool ok = SaveXML(prod,cfg,"");

  return ok;
}

bool SaveXML(const ParamFile &cfg, const char *name)
{
  FILE *f = fopen(name,"w");
  if (!f) return false;
  bool ok = SaveXML(cfg,f);
  if (fclose(f)<0)
  {
    return false;
  }
  return ok;
}



class ParamFileParser: public SAXParser
{
  class Context: public RefCount, public TLinkBidirRef
  {
    public:
    RString _name;
    ParamClassPtr _actClass;
    ParamEntryPtr _actEntry;
    InitPtr<IParamArrayValue> _actValue;
    InitVal<bool,false> _expValue;

    Context(RString name):_name(name){}
  };

  ParamFile &_f;

  TListBidirRef<Context> _context;
  
  ParamClassPtr GetActClass(Context *ctx);
  ParamClassPtr GetActClass();

  Context *GetActContext(){return _context.Last();}
  Context *GetActContext(Context *ctx) {return ctx;}

  public:
  ParamFileParser(ParamFile &f);

	virtual void OnStartElement(RString name, XMLAttributes &attributes);
	virtual void OnEndElement(RString name);
	virtual void OnCharacters(RString chars);
};

ParamFileParser::ParamFileParser(ParamFile &f)
:_f(f)
{
}


ParamClassPtr ParamFileParser::GetActClass()
{
  return GetActClass(_context.Last());
}

ParamClassPtr ParamFileParser::GetActClass(Context *ctx)
{
  if (!ctx)
  {
    return &_f;
  }
  if (!ctx->_actClass)
  {
    ParamClassPtr parent = GetActClass(_context.Prev(ctx));
    if (!parent)
    {
      RptF("Cannot create a parent class for %s",cc_cast(ctx->_name));
      return NULL;
    }
    ctx->_actClass = parent->AddClass(ctx->_name);
    if (!ctx->_actClass)
    {
      RptF("Cannot create a class %s",cc_cast(ctx->_name));
      return NULL;
    }
  }
  return ctx->_actClass;
}

void ParamFileParser::OnStartElement(RString name, XMLAttributes &attributes)
{
  XMLAttribute *type = attributes.Find("type");
  Context *actCtx = GetActContext();
  if (actCtx && actCtx->_actEntry.NotNull())
  {
    // we are parsing an array
    if (type)
    {
      if (!strcmp(type->value,"array"))
      {
        Context *ctx = new Context(name);
        _context.Add(ctx);
        ctx->_actValue = actCtx->_actEntry->AddArrayValue();
        return;
      }
    }
    // current entry must be a class
    Context *ctx = new Context(name);
    _context.Add(ctx);
    ctx->_expValue = true;
    return;
  }
  if (actCtx && actCtx->_actValue.NotNull())
  {
    // we are parsing an array
    if (type)
    {
      if (!strcmp(type->value,"array"))
      {
        Context *ctx = new Context(name);
        _context.Add(ctx);
        ctx->_actValue = actCtx->_actValue->AddArrayValue();
        return;
      }
    }
    // current entry must be a class
    Context *ctx = new Context(name);
    _context.Add(ctx);
    ctx->_expValue = true;
    return;
  }
  ParamClassPtr cls = GetActClass();
  if (!cls) return;
  // current entry must be a class
  Context *ctx = new Context(name);
  _context.Add(ctx);
  if (type)
  {
    // explicit type handling
    if (!strcmp(type->value,"array"))
    {
      // expect items named "item"
      ctx->_actEntry = cls->AddArray(name);
    }
    return;
  }
  XMLAttribute *base = attributes.Find("base");
  if (base)
  {
    ParamClassPtr cls = GetActClass();
    if (!cls) return;
    cls->SetBase(base->value);
  }
}
void ParamFileParser::OnEndElement(RString name)
{
  Context *ctx = _context.Last();
  if (strcmp(ctx->_name,name))
  {
    RptF("Entry %s not matching %s",cc_cast(name),cc_cast(ctx->_name));
    return;
  }
  // close current context
  _context.Delete(ctx);
}
void ParamFileParser::OnCharacters(RString chars)
{
  // content reached - must be an entry
  if (!_context.Last())
  {
    LogF("No entry started, %s",cc_cast(chars));
    return;
  }
  Context *ctx = _context.Last();
  // we must have a parent
  if (!_context.Prev(ctx))
  {
    RptF("No class started, %s",cc_cast(chars));
    return;
  }
  Context *ctxPrev = _context.Prev(ctx);
  if (ctxPrev->_actEntry.NotNull() && ctx->_expValue.get())
  {
    ctxPrev->_actEntry->AddValue(chars);
    return;
  }
  if (ctxPrev->_actValue.NotNull() && ctx->_expValue.get())
  {
    ctxPrev->_actValue->AddValue(chars);
    return;
  }
  if (ctx->_actValue)
  {
    ctx->_actValue->AddValue(chars);
    return;
  }
  if (ctx->_actClass)
  {
    LogF("%s already used a as class",cc_cast(ctx->_name));
  }
  // parent entry must be a class
  ParamClassPtr cls = GetActClass(ctxPrev);
  if (cls)
  {
    cls->Add(ctx->_name,chars);
  }
}

bool ParseXML(ParamFile &f, const char *name)
{
  QIFStream in;
  in.open(name);
  // check if it is XML file
  if (in.fail()) return false;
  int c = in.get();
  if (c!='<')
  {
    return false;
  }
  ParamFileParser sax(f);
  return sax.Parse(in);
}

