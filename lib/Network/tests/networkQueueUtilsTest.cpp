#include "../networkQueueUtils.hpp"
#include <cassert>
#include <initializer_list>
#include <vector>

struct QueueTestItem
{
  int id;
  bool dependOn;
};

struct QueueTestRef
{
  QueueTestItem *item;

  explicit QueueTestRef(QueueTestItem *value) : item(value) {}
  QueueTestItem *operator->() const { return item; }
  operator QueueTestItem *() const { return item; }
};

class QueueTestArray
{
  std::vector<QueueTestRef> _items;
public:
  QueueTestArray(std::initializer_list<QueueTestItem *> items)
  {
    for (QueueTestItem *item : items) _items.emplace_back(item);
  }
  QueueTestRef &operator[](int index) { return _items[index]; }
  QueueTestRef &Set(int index) { return _items[index]; }
  const QueueTestRef &Get(int index) const { return _items[index]; }
  void Delete(int index, int count)
  {
    _items.erase(_items.begin() + index, _items.begin() + index + count);
  }
  int Size() const { return static_cast<int>(_items.size()); }
};

int main()
{
  QueueTestItem items[] = {{0, false}, {1, true}, {2, false}, {3, true}, {4, false}, {5, false}};
  QueueTestArray queue{&items[0], &items[1], &items[2], &items[3], &items[4], &items[5]};
  std::vector<int> sentDependencies;

  int normalCount = CompactNetworkMessageQueueRange(
    queue, 1, 5,
    [&sentDependencies](QueueTestItem *item) { sentDependencies.push_back(item->id); }
  );

  assert(normalCount == 2);
  assert((sentDependencies == std::vector<int>{1, 3}));
  assert(queue.Size() == 4);
  assert(queue[0]->id == 0);
  assert(queue[1]->id == 2);
  assert(queue[2]->id == 4);
  assert(queue[3]->id == 5);

  std::vector<int> noDependencies;
  normalCount = CompactNetworkMessageQueueRange(
    queue, 1, 3,
    [&noDependencies](QueueTestItem *item) { noDependencies.push_back(item->id); }
  );
  assert(normalCount == 2);
  assert(noDependencies.empty());
  assert(queue.Size() == 4);

  QueueTestItem allDependent[] = {{6, true}, {7, true}};
  QueueTestArray allDependentQueue{&allDependent[0], &allDependent[1]};
  normalCount = CompactNetworkMessageQueueRange(
    allDependentQueue, 0, 2,
    [&sentDependencies](QueueTestItem *item) { sentDependencies.push_back(item->id); }
  );
  assert(normalCount == 0);
  assert(allDependentQueue.Size() == 0);
  assert((sentDependencies == std::vector<int>{1, 3, 6, 7}));
}
