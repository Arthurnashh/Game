#ifndef _ENGDUMMY_HPP_
#define _ENGDUMMY_HPP_

#if _USE_FCPHMANAGER

#include <Es/Common/win.h>

//! Class to store identification of file creation processes, that may run parallel
class FCProcessHandles : public TLinkBidirD
{
public:
  //! Name of the destination file name
  RStringB _fileName;
  //! Handle of the process
  HANDLE _handle;
  //! Constructor
  FCProcessHandles(RStringB fileName, HANDLE handle)
  {
    _fileName = fileName;
    _handle = handle;
  }
};

//! Manager of the file creation processes
class FCPHManager : public TListBidir<FCProcessHandles, TLinkBidirD, SimpleCounter<FCProcessHandles> >
{
private:
  //! Number of processes to be allowed to run at once, 1 means just one process can run in one time
  int _maxProcesses;
  //! Wait for finishing some processes to run only certain number of them at once
  void LimitNumberOfRunProcesses();
public:
  //! Constructor
  FCPHManager()
  {
    _maxProcesses = 1;
  }
  //! Destructor
  ~FCPHManager()
  {
    FCProcessHandles *item;
    while ((item = First()) != NULL)
    {
      // Finish process and delete item
      FinishProcessAndDeleteItem(item);
    }
  }
  //! Wrapper of _maxProcesses
  void SetMaxProcesses(int maxProcesses) {_maxProcesses = maxProcesses;}
  //! Method to return item by the name, NULL if not found
  FCProcessHandles *Find(RStringB name)
  {
    FCProcessHandles *item = First();
    while (item)
    {
      if (item->_fileName == name) return item;
      item = Next(item);
    }
    return NULL;
  }
  //! Function that finishes the process and deletes the specified item
  void FinishProcessAndDeleteItem(FCProcessHandles *item)
  {
    // Wait for the process to finish
    WaitForSingleObject(item->_handle, INFINITE);

    // Close the process handle
    CloseHandle(item->_handle);

    // Delete the item
    Delete(item);
    delete item;
  }
  //! Method to insert new file creation process
  void AddItem(RStringB fileName, HANDLE handle);
};

//! Global instance of the FCPHManager
extern FCPHManager GFCPHManager;

#endif

#endif
