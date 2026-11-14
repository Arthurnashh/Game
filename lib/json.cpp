#include "wpch.hpp"
#include "json.hpp"

void JSONAddVector3(JSONValue &container, const char *name, const Vector3 &value, JSONAllocator &allocator)
{
  rapidjson::Value arr;
  arr.SetArray();
  arr.Reserve(3, allocator);
  arr.PushBack(value.X(), allocator);
  arr.PushBack(value.Y(), allocator);
  arr.PushBack(value.Z(), allocator);
  container.AddMember(name, arr, allocator);
}

bool JSONReadVector3(JSONValue &container, const char *name, Vector3 &value)
{
  rapidjson::Value &arr = container[name];
  if (!arr.IsArray() || arr.Size() != 3) return false;
  value[0] = arr[rapidjson::SizeType(0)].GetDouble();
  value[1] = arr[rapidjson::SizeType(1)].GetDouble();
  value[2] = arr[rapidjson::SizeType(2)].GetDouble();
  return true;
}

void JSONAddMatrix4(JSONValue &container, const char *name, const Matrix4 &value, JSONAllocator &allocator)
{
	rapidjson::Value arr;
	arr.SetArray();
	arr.Reserve(9, allocator);
	arr.PushBack(value.Position().X(), allocator);
	arr.PushBack(value.Position().Y(), allocator);
	arr.PushBack(value.Position().Z(), allocator);
	arr.PushBack(value.Direction().X(), allocator);
	arr.PushBack(value.Direction().Y(), allocator);
	arr.PushBack(value.Direction().Z(), allocator);
	arr.PushBack(value.DirectionUp().X(), allocator);
	arr.PushBack(value.DirectionUp().Y(), allocator);
	arr.PushBack(value.DirectionUp().Z(), allocator);
	container.AddMember(name, arr, allocator);
}

bool JSONReadMatrix4(JSONValue &container, const char *name, Matrix4 &value)
{
	Vector3 v;
	rapidjson::Value &arr = container[name];
	if (!arr.IsArray() || arr.Size() != 9) return false;
	v[0] = arr[rapidjson::SizeType(0)].GetDouble();
	v[1] = arr[rapidjson::SizeType(1)].GetDouble();
	v[2] = arr[rapidjson::SizeType(2)].GetDouble();
	value.SetPosition(v);
	v[0] = arr[rapidjson::SizeType(3)].GetDouble();
	v[1] = arr[rapidjson::SizeType(4)].GetDouble();
	v[2] = arr[rapidjson::SizeType(5)].GetDouble();
	value.SetDirection(v);
	v[0] = arr[rapidjson::SizeType(6)].GetDouble();
	v[1] = arr[rapidjson::SizeType(7)].GetDouble();
	v[2] = arr[rapidjson::SizeType(8)].GetDouble();
	value.SetDirectionUp(v);
	return true;
}
