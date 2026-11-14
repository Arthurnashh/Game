#include <Es/essencepch.hpp>
#include <Es/Containers/array.hpp>
#include <Es/Containers/quadtreeEx.hpp>

void ArrayTest()
{
  AutoArray<int> a;
  for (int i=0; i<10; i++)
  {
    a.Add(i);
  }
  Assert(a.Size()==10);
  Assert(a.MaxSize()>=10);
}

void QuadTreeExTest()
{
  const int dim = 100;
  QuadTreeExRoot<int> q(dim,dim,0);
  // populate with some data
  q.Set(0,0,1);
  Assert(q.Get(0,0)==1);
  q.Set(5,5,3);
  // all values different
  for (int x=0; x<dim; x++) for (int y=0; y<dim; y++)
  {
    q.Set(x,y,x);
  }
  srand(1);
  // many values back to default
  for (int i=0; i<10000; i++)
  {
    int x = rand()%dim;
    int y = rand()%dim;
    q.Set(x,y,0);
  }
  // all values back to default
  for (int x=0; x<dim; x++) for (int y=0; y<dim; y++)
  {
    q.Set(x,y,0);
  }
}

int main()
{
  ArrayTest();
  QuadTreeExTest();
  return 0;
}
