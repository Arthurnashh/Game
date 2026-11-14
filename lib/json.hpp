#ifdef _MSC_VER
#pragma once
#endif

#ifndef JSON_HPP
#define JSON_HPP

// Helper file to introduce some definitions and functions for serialization into / from JSON

#include <Es/Strings/rString.hpp>
#include <El/Math/math3d.hpp>

#include <Es/Memory/normalNew.hpp>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <Es/Memory/debugNew.hpp>

typedef rapidjson::Document JSONDocument;
typedef JSONDocument::AllocatorType JSONAllocator;

typedef rapidjson::Value JSONValue;

//! Assemble Vector3 JSON array
void JSONAddVector3(JSONValue &container, const char *name, const Vector3 &value, JSONAllocator &allocator);
//! Parse Vector3 out ofJSON array
bool JSONReadVector3(JSONValue &container, const char *name, Vector3 &value);

//! Assemble Matrix4 JSON array
void JSONAddMatrix4(JSONValue &container, const char *name, const Matrix4 &value, JSONAllocator &allocator);
//! Parse Matrix4 out of JSON array
bool JSONReadMatrix4(JSONValue &container, const char *name, Matrix4 &value);

#endif
