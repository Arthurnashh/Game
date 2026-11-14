#include <El/elementpch.hpp>
#include <El/ParamArchive/paramArchive.hpp>
#include <El/Evaluator/express.hpp>
#include "kbCenter.hpp"

KBCenter::ParentList::ParentList(const KBCenter *&parent)
{
  // parent passed may be NULL - we want empty list in such case
  if (parent)
  {
    _parents = &parent;
    _nParents = 1;
  }
  else
  {
    _parents = NULL;
    _nParents = 0;
  }
}

int KBCenter::ParentList::TopicCount(const KBCenter *center) const
{
  int total = center->_topics.Size();
  for (int i=0; i<_nParents; i++)
  {
    total += _parents[i]->_topics.Size();
  }
  return total;
}

KBTopic *KBCenter::ParentList::GetTopic(const KBCenter *center, int index) const
{
  if (index<center->_topics.Size())
  {
    return center->_topics[index];
  }
  index -= center->_topics.Size();
  for (int i=0; i<_nParents; i++)
  {
    int size = _parents[i]->_topics.Size();
    if (index<size) return _parents[i]->_topics[index];
    index -= size;
  }
  Fail("Topic index of out range");
  return NULL;
}

KBTopic *KBCenter::GetTopic(RString name, const ParentList &list) const
{
  // first search this center
  for (int i=0; i<_topics.Size(); i++)
  {
    KBTopic *topic = _topics[i];
    if (stricmp(topic->GetName(), name) == 0)
      return topic;
  }
  // if not found, try parents
  if (list._nParents>0)
  {
    return list._parents[0]->GetTopic(name,ParentList(list._parents+1,list._nParents-1));
  }
  return NULL;
}

void KBCenter::AddTopic(KBTopic *topic)
{
  Assert(topic);
  RemoveTopic(topic->GetName());
  _topics.Add(topic);
}

void KBCenter::RemoveTopic(RString name)
{
  for (int i=0; i<_topics.Size(); i++)
  {
    KBTopic *topic = _topics[i];
    if (stricmp(topic->GetName(), name) == 0)
    {
      _topics.Delete(i);
      return;
    }
  }
}

KBMessageInfo *KBCenter::CreateMessage(RString topicName, GameValuePar sentence, const ParentList &list) const
{
  KBTopic *topic = GetTopic(topicName,list);
  if (!topic) return NULL;

  if (sentence.GetType() == GameString)
  {
    const KBMessageTemplate *type = topic->FindMessageTemplate(sentence);
    if (!type) return NULL;
    return new KBMessageInfo(topicName, new KBMessage(type));
  }
  else
  {
    Assert(sentence.GetType() == GameArray);
    const GameArrayType &array = sentence;
    Assert(array.Size() == 3);
    const KBMessageTemplate *type = topic->FindMessageTemplate(array[0]);
    if (!type) return NULL;
    Ref<KBMessageTemplate> type2 = topic->CreateTemplate(*type, array[1], array[2]);
    return new KBMessageInfo(topicName, new KBMessage(type2));
  }
}

void KBCenter::SetMessageArgument(KBMessageInfo *message, GameValuePar argument) const
{
  const KBMessage *msg = message->GetMessage();
  if (!msg) return;
  unconst_cast(msg)->SetArgument(argument);
}

void KBCenter::CheckMessage(KBMessageInfo *message, const ParentList &list) const
{
  const KBMessage *msg = message->GetMessage();
  if (!msg) return;

  KBTopic *topic = GetTopic(message->GetTopic(),list);
  if (topic) topic->CheckMessage(unconst_cast(msg));
}

RString KBCenter::GetHandlerAI(const KBMessageInfo *message, const ParentList &list) const
{
  KBTopic *topic = GetTopic(message->GetTopic(),list);
  if (!topic) return RString();
  return topic->GetHandlerAI();
}

GameValue KBCenter::GetHandlerPlayer(const KBMessageInfo *message, const ParentList &list) const
{
  KBTopic *topic = GetTopic(message->GetTopic(),list);
  if (!topic) return GameValue();
  return topic->GetHandlerPlayer();
}

RString KBCenter::GetText(const KBMessageInfo *message, const ParentList &list) const
{
  KBTopic *topic = GetTopic(message->GetTopic(),list);
  if (!topic) return RString();
  const KBMessage *msg = message->GetMessage();
  if (!msg) return RString();

  return topic->GetText(msg);
}

void KBCenter::GetSpeech(AutoArray<RString> &speech, const KBMessageInfo *message, const ParentList &list) const
{
  KBTopic *topic = GetTopic(message->GetTopic(),list);
  if (!topic) return;
  const KBMessage *msg = message->GetMessage();
  if (!msg) return;

  topic->GetSpeech(speech, msg);
}

const KBUINodeInfo *KBCenter::GetUIRoot(const KBMessageInfo *message, const ParentList &list) const
{
  if (!message) return NULL;
  RString topicName = message->GetTopic();
  KBTopic *topic = GetTopic(topicName,list);
  if (!topic) return NULL;

  return new KBUINodeInfo(topic, topic->GetUIRoot(message->GetMessage()));
}

int KBCenter::NChilds(const KBUINodeInfo *node, const ParentList &list) const
{
  if (node)
  {
    KBTopic *topic = node->GetTopic();
    if (!topic) return 0;
    return topic->NChilds(node->GetUINode());
  }
  else
  {
    int count = 0;
    int topics = list.TopicCount(this);
    for (int j=0; j<topics; j++)
    {
      const KBTopic *topic = list.GetTopic(this,j);
      const KBUINode *root = topic->GetUIRoot(NULL);
      if (root) count += topic->NChilds(root);
    }
    return count;
  }
}

const KBUINodeInfo *KBCenter::GetChild(const KBUINodeInfo *node, int i, const ParentList &list) const
{
  if (node)
  {
    KBTopic *topic = node->GetTopic();
    if (!topic) return NULL;
    return new KBUINodeInfo(topic, topic->GetChild(node->GetUINode(), i));
  }
  else
  {
    int topics = list.TopicCount(this);
    for (int j=0; j<topics; j++)
    {
      KBTopic *topic = list.GetTopic(this,j);
      const KBUINode *root = topic->GetUIRoot(NULL);
      if (root)
      {
        int n = topic->NChilds(root);
        if (i < n) return new KBUINodeInfo(topic, topic->GetChild(root, i));
        i -= n;
      }
    }
    return NULL;
  }
}

RString KBCenter::GetCondition(const KBUINodeInfo *node) const
{
  if (!node) return RString();
  KBTopic *topic = node->GetTopic();
  if (!topic) return RString();
  return topic->GetCondition(node->GetUINode());
}

RString KBCenter::GetText(const KBUINodeInfo *node) const
{
  if (!node) return RString();
  KBTopic *topic = node->GetTopic();
  if (!topic) return RString();
  return topic->GetText(node->GetUINode());
}

const KBMessageInfo *KBCenter::GetSentence(const KBUINodeInfo *node) const
{
  if (!node) return NULL;
  KBTopic *topic = node->GetTopic();
  if (!topic) return NULL;
  const KBMessage *message = topic->GetSentence(node->GetUINode());
  if (!message) return NULL;
  return new KBMessageInfo(topic->GetName(), message);
}

LSError KBCenter::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("topics", _topics, 1))
  return LSOK;
}

KBCenter *KBCenter::CreateObject(ParamArchive &ar)
{
  return new KBCenter();
}

