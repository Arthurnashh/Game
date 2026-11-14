// CreateFormat
#define MSG_FORMAT(MessageName,type,name,datatype,rawType,compres, defval, description, xfer) \
  { defval; format.Add(#name,datatype,compres,defValue,description); }

#define MSG_FORMAT_NODEFVALUE(MessageName,type,name,datatype,rawType,compres, defval, description, xfer) \
  { format.Add(#name,datatype,compres,description); }

#define MSG_TRANSFER(MessageName,type,name,datatype,rawType, compres, defval, description, xfer) \
  xfer(name)

// def. network object
#define MSG_DEF(MessageName,type,name,datatype,rawType, compres, defval, description, xfer) \
  type _##name;

// def. message
#define MSG_MSGDEF(MessageName,type,name,datatype,rawType, compres, defval, description, xfer) \
  rawType _##name;

// update format (set offsets)
#define MSG_UPDATE_FORMAT(MessageName,type,name,datatype,rawType, compres, defval, description, xfer) \
  format->SetOffset(#name, offsetof(NetworkMessage##MessageName, _##name));

// CreateFormat
#define MSG_FORMAT_ERR(MessageName,type,name,datatype,rawType, compres, defval, description, xfer, errType, errCoef ) \
  { defval; format.Add(#name,datatype,compres,defValue,description,errType,errCoef); }

#define MSG_TRANSFER_ERR(MessageName,type,name,datatype,rawType, compres, defval, description, xfer, errType, errCoef ) \
  xfer(name)

// def. network object
#define MSG_DEF_ERR(MessageName,type,name,datatype,rawType, compres, defval, description, xfer, errType, errCoef ) \
  type _##name;

// def. message
#define MSG_MSGDEF_ERR(MessageName,type,name,datatype,rawType, compres, defval, description, xfer, errType, errCoef ) \
  rawType _##name;

// update format (set offsets)
#define MSG_UPDATE_FORMAT_ERR(MessageName,type,name,datatype,rawType, compres, defval, description, xfer, errType, errCoef) \
  format->SetOffset(#name, offsetof(NetworkMessage##MessageName, _##name));

// calculate error on server
#define MSG_CALCULATE_ERROR_STATIC(MessageName,type,name,datatype,rawType, compres, defval, description, xfer, errType, errCoef) \
  if (errType > ET_NONE) error += errCoef * ::CalculateError< errType, rawType >::ErrorStatic(_##name, msg->_##name);

#define MSG_CALCULATE_ERROR_DYNAMIC(MessageName,type,name,datatype,rawType, compres, defval, description, xfer, errType, errCoef) \
  if (errType > ET_NONE) error += errCoef * ::CalculateError< errType, rawType >::ErrorDynamic(_##name, msg->_##name, dt);

#define MSG_CREATE_DIFF(MessageName,type,name,datatype,rawType, compres, defval, description, xfer, errType, errCoef) \
  ::CalculateError< errType, rawType >::CreateDiff(this->_##name, msgWith->_##name, diff);

#define MSG_APPLY_DIFF(MessageName,type,name,datatype,rawType, compres, defval, description, xfer, errType, errCoef) \
  ::CalculateError< errType, rawType >::ApplyDiff(_##name, msg->_##name, diff);

// default base for Indices is NetworkMessage
// default base for messages is NetworkSimpleObject

#define DECLARE_NET_INDICES_COMMON(MessageName,MSG_FORMAT_DEF) \
  MSG_FORMAT_DEF(MessageName, MSG_MSGDEF) \
  virtual float CalculateErrorStatic(NetworkMessage *with); \
  virtual float CalculateErrorDynamic(NetworkMessage *with, float dt); \
  virtual float CalculateErrorInitial(); \
  virtual NetworkMessage##MessageName *Clone() const {return new NetworkMessage##MessageName(*this);} \
  virtual size_t GetStructureSize() const {return sizeof(NetworkMessage##MessageName);}

#define DECLARE_NET_INDICES_NO_NMT(MessageName,MSG_FORMAT_DEF, Parent) \
  struct NetworkMessage##MessageName : public Parent \
  { \
    DECLARE_NET_INDICES_COMMON(MessageName,MSG_FORMAT_DEF) \
  };

#define DECLARE_NET_INDICES(MessageName,MSG_FORMAT_DEF) \
  struct NetworkMessage##MessageName : public NetworkMessage \
  { \
    DECLARE_NET_INDICES_COMMON(MessageName,MSG_FORMAT_DEF) \
    virtual NetworkMessageType GetNMType() const; \
  };

#define DEFINE_NET_INDICES_NO_NMT(MessageName,MSG_FORMAT_DEF) \
  NetworkMessage *CreateNetworkMessage##MessageName() {return new NetworkMessage##MessageName();} \
  \
  void UpdateMessageFormat##MessageName(NetworkMessageFormatBase *format, int &index) \
  { \
    MSG_FORMAT_DEF(MessageName, MSG_UPDATE_FORMAT) \
  } \
  \
  float NetworkMessage##MessageName::CalculateErrorStatic(NetworkMessage *with) \
  { \
    return 0; \
  } \
  float NetworkMessage##MessageName::CalculateErrorDynamic(NetworkMessage *with, float dt) \
  { \
    return 0; \
  } \
  float NetworkMessage##MessageName::CalculateErrorInitial() \
  { \
    return 0; \
  }

#define DEFINE_NET_INDICES(MessageName,MSG_FORMAT_DEF) \
  DEFINE_NET_INDICES_NO_NMT(MessageName,MSG_FORMAT_DEF) \
  NetworkMessageType NetworkMessage##MessageName::GetNMType() const \
  { \
    return NMT##MessageName; \
  }

#define DECLARE_NET_INDICES_EX(MessageName,BaseMessageName,MSG_FORMAT_DEF) \
  struct NetworkMessage##MessageName : public NetworkMessage##BaseMessageName \
  { \
    DECLARE_NET_INDICES_COMMON(MessageName,MSG_FORMAT_DEF) \
    virtual NetworkMessageType GetNMType() const; \
  };

#define DECLARE_NET_INDICES_EX_SERIALIZABLE(MessageName,BaseMessageName,MSG_FORMAT_DEF) \
  struct NetworkMessage##MessageName : public NetworkMessage##BaseMessageName \
  { \
    DECLARE_NET_INDICES_COMMON(MessageName,MSG_FORMAT_DEF) \
    virtual NetworkMessageType GetNMType() const; \
    virtual bool IsSerializable() const {return true;} \
    virtual LSError Serialize(ParamArchive &ar); \
  };

#define DECLARE_NET_INDICES_EX_NO_NMT(MessageName,BaseMessageName,MSG_FORMAT_DEF) \
  struct NetworkMessage##MessageName : public NetworkMessage##BaseMessageName \
  { \
    DECLARE_NET_INDICES_COMMON(MessageName,MSG_FORMAT_DEF) \
  };

#define DEFINE_NET_INDICES_EX_NO_NMT(MessageName, BaseMessageName, MSG_FORMAT_DEF) \
  NetworkMessage *CreateNetworkMessage##MessageName() {return new NetworkMessage##MessageName();} \
  \
  void UpdateMessageFormat##MessageName(NetworkMessageFormatBase *format, int &index) \
  { \
    void UpdateMessageFormat##BaseMessageName(NetworkMessageFormatBase *format, int &index); \
    UpdateMessageFormat##BaseMessageName(format, index); \
    MSG_FORMAT_DEF(MessageName, MSG_UPDATE_FORMAT) \
  } \
  \
  float NetworkMessage##MessageName::CalculateErrorStatic(NetworkMessage *with) \
  { \
    return NetworkMessage##BaseMessageName::CalculateErrorStatic(with); \
  } \
  \
  float NetworkMessage##MessageName::CalculateErrorDynamic(NetworkMessage *with, float dt) \
  { \
    return NetworkMessage##BaseMessageName::CalculateErrorDynamic(with, dt); \
  } \
  \
  float NetworkMessage##MessageName::CalculateErrorInitial() \
  { \
    return NetworkMessage##BaseMessageName::CalculateErrorInitial(); \
  }

#define DEFINE_NET_INDICES_EX(MessageName,BaseMessageName,MSG_FORMAT_DEF) \
  DEFINE_NET_INDICES_EX_NO_NMT(MessageName,BaseMessageName,MSG_FORMAT_DEF) \
  NetworkMessageType NetworkMessage##MessageName::GetNMType() const \
  { \
    return NMT##MessageName; \
  }

#define MSG_SERIALIZE(MessageName,type,name,datatype,rawType, compres, defval, description, xfer) \
  ::Serialize(armsg, #name, _##name, 1);

#define DEFINE_SERIALIZE(MessageName,MSG_FORMAT_DEF) \
  LSError NetworkMessage##MessageName::Serialize(ParamArchive &ar) \
  { \
    NetworkMessageType type = GetNMType(); \
    if (ar.IsSaving()) ar.Serialize("type", (int &)type, 1); \
    ParamArchive armsg; \
    if (!ar.OpenSubclass("MsgBody", armsg)) return LSOK; \
    MSG_FORMAT_DEF(MessageName, MSG_SERIALIZE) \
    ar.CloseSubclass(armsg); \
    return LSOK; \
  }


#define DEFINE_NET_INDICES_EX_SERIALIZABLE(MessageName,BaseMessageName,MSG_FORMAT_DEF) \
  DEFINE_NET_INDICES_EX_NO_NMT(MessageName,BaseMessageName,MSG_FORMAT_DEF) \
  NetworkMessageType NetworkMessage##MessageName::GetNMType() const \
  { \
    return NMT##MessageName; \
  } \
  DEFINE_SERIALIZE(MessageName,MSG_FORMAT_DEF)


#define DECLARE_NET_INDICES_INIT_MSG(MessageName,MSG_FORMAT_DEF) \
  struct NetworkMessage##MessageName : public NetworkMessage \
  { \
    DECLARE_NET_INDICES_COMMON(MessageName,MSG_FORMAT_DEF) \
    virtual NetworkMessageType GetNMType() const; \
    virtual bool IsSerializable() const {return true;} \
    virtual LSError Serialize(ParamArchive &ar); \
  };

#define DEFINE_NET_INDICES_INIT_MSG(MessageName,MSG_FORMAT_DEF) \
  DEFINE_NET_INDICES(MessageName,MSG_FORMAT_DEF) \
  DEFINE_SERIALIZE(MessageName,MSG_FORMAT_DEF)

#define DECLARE_NET_INDICES_EX_INIT_MSG(MessageName,BaseMessageName,MSG_FORMAT_DEF) \
  struct NetworkMessage##MessageName : public NetworkMessage##BaseMessageName \
  { \
    DECLARE_NET_INDICES_COMMON(MessageName,MSG_FORMAT_DEF) \
    virtual NetworkMessageType GetNMType() const; \
    virtual bool IsSerializable() const {return true;} \
    virtual LSError Serialize(ParamArchive &ar); \
  };

#define DEFINE_NET_INDICES_EX_INIT_MSG(MessageName,BaseMessageName,MSG_FORMAT_DEF) \
  DEFINE_NET_INDICES_EX(MessageName,BaseMessageName,MSG_FORMAT_DEF) \
  LSError NetworkMessage##MessageName::Serialize(ParamArchive &ar) \
  { \
    NetworkMessageType type = GetNMType(); \
    if (ar.IsSaving()) ar.Serialize("type", (int &)type, 1); \
    ParamArchive armsg; \
    if (!ar.OpenSubclass("MsgBody", armsg)) return LSOK; \
    MSG_FORMAT_DEF(MessageName, MSG_SERIALIZE) \
    ar.CloseSubclass(armsg); \
    return LSOK; \
  }

#define DECLARE_NET_INDICES_ERR(MessageName,MSG_FORMAT_DEF) \
  struct NetworkMessage##MessageName : public NetworkMessageDiffed \
  { \
    MSG_FORMAT_DEF(MessageName, MSG_MSGDEF_ERR) \
    virtual float CalculateErrorStatic(NetworkMessage *with); \
    virtual float CalculateErrorDynamic(NetworkMessage *with, float dt); \
    virtual float CalculateErrorInitial(); \
    virtual NetworkMessage##MessageName *Clone() const {return new NetworkMessage##MessageName(*this);} \
    virtual size_t GetStructureSize() const {return sizeof(NetworkMessage##MessageName);} \
    virtual void CreateDiff(const NetworkMessage *with, MessageDiffCreate &diff) override; \
    virtual void ApplyDiff(const NetworkMessage *with, MessageDiffApply &diff) override; \
  };

#define DEFINE_NET_INDICES_DIFF(MessageName, BaseMessageName, MSG_FORMAT_DEF) \
  void NetworkMessage##MessageName::CreateDiff(const NetworkMessage *with, MessageDiffCreate &diff) \
  { \
    NetworkMessage##BaseMessageName::CreateDiff(with,diff); \
    const NetworkMessage##MessageName *msgWith = static_cast_checked<const NetworkMessage##MessageName *>(with);  (void)msgWith; \
    MSG_FORMAT_DEF(MessageName, MSG_CREATE_DIFF) \
  } \
  void NetworkMessage##MessageName::ApplyDiff(const NetworkMessage *with, MessageDiffApply &diff) \
  { \
    NetworkMessage##BaseMessageName::ApplyDiff(with,diff); \
    const NetworkMessage##MessageName *msg = static_cast_checked<const NetworkMessage##MessageName *>(with); (void) msg; \
    MSG_FORMAT_DEF(MessageName, MSG_APPLY_DIFF) \
  }

#define DEFINE_NET_INDICES_ERR(MessageName, MSG_FORMAT_DEF, ErrorInitialFunc) \
  NetworkMessage *CreateNetworkMessage##MessageName() {return new NetworkMessage##MessageName();} \
  \
  void UpdateMessageFormat##MessageName(NetworkMessageFormatBase *format, int &index) \
  { \
    MSG_FORMAT_DEF(MessageName, MSG_UPDATE_FORMAT_ERR) \
  } \
  \
  float NetworkMessage##MessageName::CalculateErrorStatic(NetworkMessage *with) \
  { \
    float error = 0; \
    NetworkMessage##MessageName *msg = (NetworkMessage##MessageName *)with; (void)msg; \
    MSG_FORMAT_DEF(MessageName, MSG_CALCULATE_ERROR_STATIC) \
    return error; \
  } \
  \
  float NetworkMessage##MessageName::CalculateErrorDynamic(NetworkMessage *with, float dt) \
  { \
    float error = 0; \
    NetworkMessage##MessageName *msg = (NetworkMessage##MessageName *)with; (void)msg; \
    MSG_FORMAT_DEF(MessageName, MSG_CALCULATE_ERROR_DYNAMIC) \
    return error; \
  } \
  \
  float NetworkMessage##MessageName::CalculateErrorInitial() \
  { \
    return ErrorInitialFunc(this); \
  } \
  DEFINE_NET_INDICES_DIFF(MessageName,Diffed,MSG_FORMAT_DEF)

#define DECLARE_NET_INDICES_EX_ERR(MessageName,BaseMessageName,MSG_FORMAT_DEF) \
  struct NetworkMessage##MessageName : public NetworkMessage##BaseMessageName \
  { \
    MSG_FORMAT_DEF(MessageName, MSG_MSGDEF_ERR) \
    virtual float CalculateErrorStatic(NetworkMessage *with); \
    virtual float CalculateErrorDynamic(NetworkMessage *with, float dt); \
    virtual float CalculateErrorInitial(); \
    virtual NetworkMessage##MessageName *Clone() const override {return new NetworkMessage##MessageName(*this);} \
    virtual size_t GetStructureSize() const {return sizeof(NetworkMessage##MessageName);} \
    virtual void CreateDiff(const NetworkMessage *with, MessageDiffCreate &diff) override; \
    virtual void ApplyDiff(const NetworkMessage *with, MessageDiffApply &diff) override; \
  };

#define DEFINE_NET_INDICES_EX_ERR(MessageName, BaseMessageName, MSG_FORMAT_DEF, ErrorInitialFunc) \
  NetworkMessage *CreateNetworkMessage##MessageName() {return new NetworkMessage##MessageName();} \
  \
  void UpdateMessageFormat##MessageName(NetworkMessageFormatBase *format, int &index) \
  { \
    void UpdateMessageFormat##BaseMessageName(NetworkMessageFormatBase *format, int &index); \
    UpdateMessageFormat##BaseMessageName(format, index); \
    MSG_FORMAT_DEF(MessageName, MSG_UPDATE_FORMAT_ERR) \
  } \
  \
  float NetworkMessage##MessageName::CalculateErrorStatic(NetworkMessage *with) \
  { \
    float error = NetworkMessage##BaseMessageName::CalculateErrorStatic(with); \
    NetworkMessage##MessageName *msg = (NetworkMessage##MessageName *)with; (void)msg; \
    MSG_FORMAT_DEF(MessageName, MSG_CALCULATE_ERROR_STATIC) \
    return error; \
  } \
  \
  float NetworkMessage##MessageName::CalculateErrorDynamic(NetworkMessage *with, float dt) \
  { \
    float error = NetworkMessage##BaseMessageName::CalculateErrorDynamic(with, dt); \
    NetworkMessage##MessageName *msg = (NetworkMessage##MessageName *)with; (void)msg; \
    MSG_FORMAT_DEF(MessageName, MSG_CALCULATE_ERROR_DYNAMIC) \
    return error; \
  } \
  \
  float NetworkMessage##MessageName::CalculateErrorInitial() \
  { \
    return ErrorInitialFunc(this); \
  } \
  DEFINE_NET_INDICES_DIFF(MessageName,BaseMessageName,MSG_FORMAT_DEF)


#define LIST_NMTTYPE(type, MessageName) \
  case NMC##type: return NMT##MessageName;

#define DECLARE_NETWORK_OBJECT \
  NetworkMessageType GetNMType(NetworkMessageClass cls) const;

#define DEFINE_NETWORK_OBJECT(ObjectName, BaseObjectName, MSG_LIST) \
  NetworkMessageType ObjectName::GetNMType(NetworkMessageClass cls) const \
  { \
  switch (cls) \
    { \
    MSG_LIST(LIST_NMTTYPE) \
    default: \
    return BaseObjectName::GetNMType(cls); \
    } \
  }

#define DEFINE_NETWORK_OBJECT_SIMPLE(ObjectName, MessageName) \
  NetworkMessageType ObjectName::GetNMType(NetworkMessageClass cls) const {return NMT##MessageName;}

#define DECLARE_DEFINE_NETWORK_OBJECT_SIMPLE(MessageName) \
  NetworkMessageType GetNMType(NetworkMessageClass cls) const {return NMT##MessageName;}

#define DECLARE_NET_MESSAGE_BASED_COMMON(MessageName, MSG_FORMAT_DEF, BaseMessageName) \
  struct MessageName##Message: public BaseMessageName##Message \
  { \
    typedef BaseMessageName##Message base; \
    \
    MSG_FORMAT_DEF(MessageName, MSG_DEF) \
    \
    DECLARE_DEFINE_NETWORK_OBJECT_SIMPLE(MessageName) \
    static NetworkMessageFormat &CreateFormat( \
      NetworkMessageClass cls, NetworkMessageFormat &format \
    ); \
    TMError TransferMsg(NetworkMessageContext &ctx); \
  };

#define DECLARE_NET_MESSAGE_BASED(MessageName,MSG_FORMAT_DEF,BaseMessageName) \
  DECLARE_NET_INDICES_EX(MessageName,BaseMessageName,MSG_FORMAT_DEF) \
  DECLARE_NET_MESSAGE_BASED_COMMON(MessageName, MSG_FORMAT_DEF, BaseMessageName)

#define DEFINE_NET_MESSAGE_BASED_COMMON(MessageName,MSG_FORMAT_DEF,Base) \
  NetworkMessageFormat &MessageName##Message::CreateFormat( \
    NetworkMessageClass cls, NetworkMessageFormat &format \
  ) \
  { \
    base::CreateFormat(cls,format); \
    MSG_FORMAT_DEF(MessageName, MSG_FORMAT) \
    return format; \
  } \
  \
  TMError MessageName##Message::TransferMsg(NetworkMessageContext &ctx) \
  { \
    TMError err = base::TransferMsg(ctx); \
    if (err!=TMOK) return err; \
    PREPARE_TRANSFER(MessageName) \
    MSG_FORMAT_DEF(MessageName, MSG_TRANSFER) \
    return TMOK; \
  }

/// define message
#define DEFINE_NET_MESSAGE_BASED(MessageName,MSG_FORMAT_DEF,Base) \
  DEFINE_NET_INDICES_EX(MessageName,Base,MSG_FORMAT_DEF) \
  DEFINE_NET_MESSAGE_BASED_COMMON(MessageName,MSG_FORMAT_DEF,Base)

/// declare message with a default base - Simple (NetworkSimpleObject,NetworkMessage)
#define DECLARE_NET_MESSAGE(MessageName,MSG_FORMAT_DEF) \
  DECLARE_NET_MESSAGE_BASED(MessageName,MSG_FORMAT_DEF,Simple)
/// define message with a default base - Simple (NetworkSimpleObject,NetworkMessage)
#define DEFINE_NET_MESSAGE(MessageName,MSG_FORMAT_DEF) \
  DEFINE_NET_MESSAGE_BASED(MessageName,MSG_FORMAT_DEF,Simple)

/// Note: Only NetworkMessages with base Simple can be MP serialized
/// declare message similar to DECLARE_NET_MESSAGE but add MP serialization
#define DECLARE_NET_MESSAGE_INIT_MSG(MessageName,MSG_FORMAT_DEF) \
  DECLARE_NET_INDICES_EX_INIT_MSG(MessageName,Simple,MSG_FORMAT_DEF) \
  DECLARE_NET_MESSAGE_BASED_COMMON(MessageName, MSG_FORMAT_DEF, Simple)
/// define message similar to DECLARE_NET_MESSAGE but add MP serialization
#define DEFINE_NET_MESSAGE_INIT_MSG(MessageName,MSG_FORMAT_DEF) \
  DEFINE_NET_INDICES_EX_INIT_MSG(MessageName,Simple,MSG_FORMAT_DEF) \
  DEFINE_NET_MESSAGE_BASED_COMMON(MessageName,MSG_FORMAT_DEF,Simple)

#define DECLARE_NET_MESSAGE_BASED_SERIALIZABLE(MessageName,MSG_FORMAT_DEF,BaseMessageName) \
  DECLARE_NET_INDICES_EX_SERIALIZABLE(MessageName,BaseMessageName,MSG_FORMAT_DEF) \
  DECLARE_NET_MESSAGE_BASED_COMMON(MessageName, MSG_FORMAT_DEF, BaseMessageName)

/// define message
#define DEFINE_NET_MESSAGE_BASED_SERIALIZABLE(MessageName,MSG_FORMAT_DEF,Base) \
  DEFINE_NET_INDICES_EX_SERIALIZABLE(MessageName,Base,MSG_FORMAT_DEF) \
  DEFINE_NET_MESSAGE_BASED_COMMON(MessageName,MSG_FORMAT_DEF,Base)

/// declare message with a default base - Simple (NetworkSimpleObject,NetworkMessage) + Serializable
#define DECLARE_NET_MESSAGE_SERIALIZABLE(MessageName,MSG_FORMAT_DEF) \
  DECLARE_NET_MESSAGE_BASED_SERIALIZABLE(MessageName,MSG_FORMAT_DEF,Simple)
/// define message with a default base - Simple (NetworkSimpleObject,NetworkMessage) + Serializable
#define DEFINE_NET_MESSAGE_SERIALIZABLE(MessageName,MSG_FORMAT_DEF) \
  DEFINE_NET_MESSAGE_BASED_SERIALIZABLE(MessageName,MSG_FORMAT_DEF,Simple)

#define DECLARE_NET_INDICES_SERIALIZABLE(MessageName,MSG_FORMAT_DEF) \
  struct NetworkMessage##MessageName : public NetworkMessage \
  { \
    DECLARE_NET_INDICES_COMMON(MessageName,MSG_FORMAT_DEF) \
    virtual NetworkMessageType GetNMType() const; \
    virtual bool IsSerializable() const {return true;} \
    virtual LSError Serialize(ParamArchive &ar); \
  }; \

#define DEFINE_NET_INDICES_SERIALIZABLE(MessageName,MSG_FORMAT_DEF) \
  DEFINE_NET_INDICES_NO_NMT(MessageName,MSG_FORMAT_DEF) \
  NetworkMessageType NetworkMessage##MessageName::GetNMType() const \
  { \
    return NMT##MessageName; \
  } \
  DEFINE_SERIALIZE(MessageName,MSG_FORMAT_DEF)

#define MSG_COMPARE_WITH(MessageName,type,name,datatype,rawType, compres, defval, description, xfer) \
  && _##name==with._##name

#define DECLARE_SIMPLE_NETWORK_STRUCTURE(Structure,STRUCTURE_MSG) \
  struct Structure \
  { \
    STRUCTURE_MSG(Structure, MSG_DEF) \
     \
    bool operator == (const Structure &with) const \
    { \
      return ( \
        true \
        STRUCTURE_MSG(Structure,MSG_COMPARE_WITH) \
      ); \
    } \
    bool operator != (const Structure &with) const {return !operator ==(with);} \
     \
    DECLARE_DEFINE_NETWORK_OBJECT_SIMPLE(Update##Structure) \
    static NetworkMessageFormat &CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format); \
    TMError TransferMsg(NetworkMessageContext &ctx); \
  };

#define DEFINE_SIMPLE_NETWORK_STRUCTURE(Structure,STRUCTURE_MSG) \
  DECLARE_NET_INDICES(Update##Structure, STRUCTURE_MSG) \
  DEFINE_NET_INDICES(Update##Structure, STRUCTURE_MSG) \
  \
  NetworkMessageFormat &Structure::CreateFormat(NetworkMessageClass cls, NetworkMessageFormat &format) \
  { \
    STRUCTURE_MSG(Update##Structure, MSG_FORMAT) \
    return format; \
  } \
   \
  TMError Structure::TransferMsg(NetworkMessageContext &ctx) \
  { \
    PREPARE_TRANSFER(Update##Structure) \
    STRUCTURE_MSG(Update##Structure, MSG_TRANSFER) \
    return TMOK; \
  }

