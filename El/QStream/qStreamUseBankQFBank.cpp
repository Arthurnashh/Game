#include <El/elementpch.hpp>

#include "iQFBank.hpp"
#include "qbStream.hpp"

class QFBankFunctionsQFBank: public QFBankFunctions
{
public:
  /// if possible, flush the bank handle so that the file can be deleted/written
  virtual bool FlushBankHandle(const char *filename);
};

bool QFBankFunctionsQFBank::FlushBankHandle(const char *filename)
{
  // scan GFileBanks
  BankList::ReadAccess banks(GFileBanks);
  for (int i=0; i<banks.Size(); i++)
  {
    QFBank &bank = banks[i];
    if (strcmpi(bank.GetOpenName(),filename)) continue;
    if (!bank.CanBeUnloaded()) return false;
    // we do not care if unloading locked bank - we know it can be unloaded
    bank.Unload();
    return true;
  }
  return true;
}

static QFBankFunctionsQFBank GQStreamQFBankBankFunctions;
QFBankFunctions *FileBufferBankFunctions::_defaultQFBankFunctions = &GQStreamQFBankBankFunctions;
