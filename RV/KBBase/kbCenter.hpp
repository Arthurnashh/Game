// Implementation of KBInterface using ParamFile and scripting commands

#ifdef _MSC_VER
#pragma once
#endif

#ifndef _KB_CENTER_HPP
#define _KB_CENTER_HPP

#include "kbMessage.hpp"

#include "kbTopic.hpp"

class AIBrain;

/// context needed for serialization of KBMessageInfo
struct KBSaveContext
{
  AIBrain *_sender;
  GameState *_gameState;

  KBSaveContext(AIBrain *sender, GameState *gameState)
    : _sender(sender), _gameState(gameState) {}
};

/// data member encapsulating topic and message inside this topic
class KBMessageInfo : public RefCount
{
protected:
  RString _topic;
  Ref<const KBMessage> _message;

public:
  KBMessageInfo(RString topic, const KBMessage *message)
    : _topic(topic), _message(message) {}
  RString GetTopic() const {return _topic;}
  const KBMessage *GetMessage() const {return _message;}

  //@{ interface for Ref serialization
  LSError Serialize(ParamArchive &ar);
  static KBMessageInfo *CreateObject(ParamArchive &ar);
  //@}

  // TODO: fast alloc
};

/// data member encapsulating topic and UI state
class KBUINodeInfo : public RefCount
{
protected:
  Ref<KBTopic> _topic;
  Ref<const KBUINode> _uiNode;

public:
  KBUINodeInfo(KBTopic *topic, const KBUINode *uiNode)
    : _topic(topic), _uiNode(uiNode) {}
  KBTopic *GetTopic() const {return _topic;}
  const KBUINode *GetUINode() const {return _uiNode;}
  // TODO: fast alloc
};

/*
// TODO: refactor to use following structure instead of center / parents
class KBCenterList
{
  const KBCenter **_centers;
  int _nCenters;

  KBCenterList():_centers(NULL),_nCenters(0){}
  KBCenterList(const KBCenter **centers, int nCenters):_centers(centers),_nCenters(nCenters){}
  KBCenterList(const KBCenter *center):_centers(&center),_nCenters(1){}

  /// total count of topics for all centers
  int TopicCount() const
  {
    int total = 0;
    for (int i=0; i<_nCenters; i++)
    {
      total += _centers[i]->_topics.Size();
    }
  }

  KBTopic *GetTopic(int index) const
  {
    for (int i=0; i<_nCenters; i++)
    {
      int size = _centers[i]->_topics.Size();
      if (index<size) return _centers[i]->_topics[index];
      index -= size;
    }
    Fail("Topic index of out range");
    return NULL;
  }
};
*/

/// Conversation center
class KBCenter : public RefCount
{
protected:
  /// conversation topics
  RefArray<KBTopic> _topics;

public:
  KBCenter() {}

  /// a list of secondary centers/topics to be handled by the functions called
  struct ParentList
  {
    const KBCenter **_parents;
    int _nParents;

    ParentList():_parents(NULL),_nParents(0){}
    ParentList(const KBCenter **parents, int nParents):_parents(parents),_nParents(nParents){}
    /**
    Note the reference here - we need to take a pointer to an object existing outside of the function
    */
    ParentList(const KBCenter *&parent);

    /// total count of topics for all centers
    int TopicCount(const KBCenter *center) const;

    KBTopic *GetTopic(const KBCenter *center, int index) const;

  };
  // topics management
  KBTopic *GetTopic(RString name, const ParentList &list) const;
  void AddTopic(KBTopic *topic);
  void RemoveTopic(RString name);

  // message creation
  KBMessageInfo *CreateMessage(RString topic, GameValuePar sentence, const ParentList &list) const;
  void SetMessageArgument(KBMessageInfo *message, GameValuePar argument) const;
  /// replace the special words, like "#article"
  void CheckMessage(KBMessageInfo *message, const ParentList &list) const;

  /// event handler reacting to this message (AI)
  RString GetHandlerAI(const KBMessageInfo *message, const ParentList &list) const;
  /// event handler reacting to this message (Player)
  GameValue GetHandlerPlayer(const KBMessageInfo *message, const ParentList &list) const;
  /// text of given message
  RString GetText(const KBMessageInfo *message, const ParentList &list) const;
  /// speech of given message
  void GetSpeech(AutoArray<RString> &speech, const KBMessageInfo *message, const ParentList &list) const;

  // UI support
  const KBUINodeInfo *GetUIRoot(const KBMessageInfo *message, const ParentList &list) const;
  int NChilds(const KBUINodeInfo *node, const ParentList &list) const;
  const KBUINodeInfo *GetChild(const KBUINodeInfo *node, int i, const ParentList &list) const;
  RString GetCondition(const KBUINodeInfo *node) const;
  RString GetText(const KBUINodeInfo *node) const;
  const KBMessageInfo *GetSentence(const KBUINodeInfo *node) const;

  // serialization
  LSError Serialize(ParamArchive &ar);
  static KBCenter *CreateObject(ParamArchive &ar);
};


#endif
