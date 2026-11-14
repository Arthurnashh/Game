#ifdef _MSC_VER
#pragma once
#endif

#ifndef _KB_EXT_HPP
#define _KB_EXT_HPP

#include <RV/KBBase/kbTopic.hpp>

class KBTopicExt : public KBTopic
{
  typedef KBTopic base;

public:
  KBTopicExt(ForSerializationOnly dummy = SerializeConstructor) // constructor usable both for creation and serialization
    : KBTopic(dummy) {}

  virtual void Init(RString name, RString filename, RString handlerAI, GameValuePar handlerPlayer)
  {
    base::Init(name, filename, handlerAI, handlerPlayer);
  }

  virtual KBMessageTemplate *CreateTemplate() const;
  virtual KBMessageTemplate *CreateTemplate(const KBMessageTemplate &src, RString text, GameValuePar speech) const;

  // serialization
  DECL_SERIALIZE_TYPE_INFO(KBTopicExt, KBTopic);
};

#endif
