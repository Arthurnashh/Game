#ifndef NETWORK_QUEUE_UTILS_HPP
#define NETWORK_QUEUE_UTILS_HPP

template <class Queue, class SendDependent>
int CompactNetworkMessageQueueRange(Queue &queue, int begin, int end, SendDependent sendDependent)
{
  int writeIndex = begin;
  for (int readIndex=begin; readIndex<end; readIndex++)
  {
    auto &item = queue[readIndex];
    if (item->dependOn)
    {
      sendDependent(item);
    }
    else
    {
      if (writeIndex != readIndex)
        queue.Set(writeIndex) = queue.Get(readIndex);
      writeIndex++;
    }
  }

  int normalCount = writeIndex - begin;
  if (writeIndex < end)
    queue.Delete(writeIndex, end - writeIndex);
  return normalCount;
}

#endif
