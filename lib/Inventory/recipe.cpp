#include "../wpch.hpp"
#include "recipe.hpp"
#include <lib/World.hpp>
#include <lib/gameStateExt.hpp>
#include <lib/Network/network.hpp>
#include <lib/drawText.hpp>

void Recipe::InputSpec::Init(RStringB text)
{
  _text = text;
  if (text.IsEmpty()) return;

  if (*text == '=')
  {
    // expression
    _text = text.Substring(1, INT_MAX);
#if USE_PRECOMPILATION
    GameDataNamespace globals(NULL, RString(), false); // temporary globals
    GameVarSpace local(GGameState.GetContext(), false);
    int gshandle = GGameState.BeginContext(&local);
    GGameState.CompileMultiple(_text, _expression, &globals);
    GGameState.EndContext(gshandle);
#endif
  }
  else
  {
    // type
    _type = VehicleTypes.New(text);
  }
}

bool Recipe::InputSpec::Matches(InventoryItem *item, RString context) const
{
  if (_type)
  {
    const InventoryItemType *type = item->Type();
    return type && type->IsKindOf(_type);
  }
  else if (_expression.Size() > 0)
  {
    GameDataNamespace *globals = GWorld->GetMissionNamespace();
    int gshandle = GGameState.BeginContext(item->GetVars());
#if USE_PRECOMPILATION
    bool ok = GGameState.EvaluateBool(_text, _expression, GameState::EvalContext::_default, globals, context);
#else
    bool ok = GGameState.EvaluateBool(_condition, GameState::EvalContext::_default, globals);
#endif
    GGameState.EndContext(gshandle);

    return ok;
  }
  else return false;
}

Recipe::Recipe(ParamEntryPar cls)
{
  _name = cls.GetName();
  _displayName = cls >> "name";

  ParamEntryPtr entry = cls.FindEntry("material");
  if (entry && entry->IsArray())
  {
    int n = entry->GetSize();
    _material.Realloc(n); _material.Resize(n);
    for (int i=0; i<entry->GetSize(); i++) _material[i].Init((*entry)[i]);
  }

  entry = cls.FindEntry("tools");
  if (entry && entry->IsArray())
  {
    int n = entry->GetSize();
    _tools.Realloc(n); _tools.Resize(n);
    for (int i=0; i<entry->GetSize(); i++) _tools[i].Init((*entry)[i]);
  }

  entry = cls.FindEntry("results");
  if (entry && entry->IsArray())
  {
    int n = entry->GetSize();
    _results.Realloc(n); _results.Resize(n);
    for (int i=0; i<entry->GetSize(); i++) _results[i] = (*entry)[i];
  }

  entry = cls.FindEntry("condition");
  if (entry)
  {
    _condition = *entry;
#if USE_PRECOMPILATION
    GameDataNamespace globals(NULL, RString(), false); // temporary globals
    GameVarSpace local(GGameState.GetContext(), false);
    int gshandle = GGameState.BeginContext(&local);
    GGameState.CompileMultiple(_condition, _conditionExp, &globals);
    GGameState.EndContext(gshandle);
#endif
  }

  entry = cls.FindEntry("action");
  if (entry)
  {
    _action = *entry;
#if USE_PRECOMPILATION
    GameDataNamespace globals(NULL, RString(), false); // temporary globals
    GameVarSpace local(GGameState.GetContext(), false);
    int gshandle = GGameState.BeginContext(&local);
    GGameState.CompileMultiple(_action, _actionExp, &globals);
    GGameState.EndContext(gshandle);
#endif
  }
}

bool Recipe::MatchesImpl(GameVarSpace &local, InventoryItem *item1, InventoryItem *item2, bool &destroyItem1, bool &destroyItem2) const
{
  // match the parameters
  bool handled1 = item1 == nullptr;
  bool handled2 = item2 == nullptr;

  for (int i=0; i<_material.Size(); i++)
  {
    RString context = Format("CfgRecipes.%s.material[%d]", cc_cast(_name), i);
    if (!handled1 && _material[i].Matches(item1, context))
    {
      handled1 = true;
      destroyItem1 = true; // material
      local.VarSet(Format("_material%d", i + 1), GameValueExt(item1), true);
      continue; // OK
    }
    if (!handled2 && _material[i].Matches(item2, context))
    {
      handled2 = true;
      destroyItem2 = true; // material
      local.VarSet(Format("_material%d", i + 1), GameValueExt(item2), true);
      continue; // OK
    }
    // missing material
    return false;
  }

  for (int i=0; i<_tools.Size(); i++)
  {
    RString context = Format("CfgRecipes.%s.tools[%d]", cc_cast(_name), i);
    if (!handled1 && _tools[i].Matches(item1, context))
    {
      handled1 = true;
      local.VarSet(Format("_tool%d", i + 1), GameValueExt(item1), true);
      continue; // OK
    }
    if (!handled2 && _tools[i].Matches(item2, context))
    {
      handled2 = true;
      local.VarSet(Format("_tool%d", i + 1), GameValueExt(item2), true);
      continue; // OK
    }
    // missing tool
    return false;
  }

  return handled1 && handled2; // both items used
}

bool Recipe::Matches(Person *owner, InventoryItem *item1, InventoryItem *item2) const
{
  // prepare variables for condition expression if available
  GameVarSpace local(GGameState.GetContext(), false);
  local.VarSet("_owner", GameValueExt(owner), true);
  local.VarSet("_name", _name, true);

  bool dummy1, dummy2; // need not to know what will be destroyed
  if (!MatchesImpl(local, item1, item2, dummy1, dummy2)) return false;

  if (_condition.IsEmpty()) return true;

  // check condition
  GameDataNamespace *globals = GWorld->GetMissionNamespace();
  int gshandle = GGameState.BeginContext(&local);
#if USE_PRECOMPILATION
  bool ok = GGameState.EvaluateBool(_condition, _conditionExp, GameState::EvalContext::_reportUndefined, globals, RString("CfgRecipes.") + _name + RString(".condition"));
#else
  bool ok = GGameState.EvaluateBool(_condition, GameState::EvalContext::_reportUndefined, globals);
#endif
  GGameState.EndContext(gshandle);

  return ok;
}

// helper to create an inventory item
InventoryItem * CreateResult(Person *owner, RString name)
{
  Ref<InventoryItem> item = dyn_cast<InventoryItem>(GWorld->NewVehicleWithID(name));
  if (!item) return NULL;

  item->Init(owner ? owner->FutureVisualState().Transform() : M4Identity, true);

  // invoke "init" event handler
  item->OnEvent(EEInit);

  // add to the world
  GWorld->AddOutVehicle(item);
  item->SetMoveOutDone(NULL);

  // create the network object later (when final placement will be known)

  return item;
}

bool Recipe::Process(Person *owner, InventoryItem *item1, InventoryItem *item2) const
{
  // prepare variables for action expression if available
  GameVarSpace local(GGameState.GetContext(), false);
  local.VarSet("_owner", GameValueExt(owner), true);
  local.VarSet("_name", _name, true);

  bool destroyItem1 = false, destroyItem2 = false;
  if (!MatchesImpl(local, item1, item2, destroyItem1, destroyItem2)) return false;

  // source matched, create results
  RefArray<InventoryItem> create;
  for (int i=0; i<_results.Size(); i++)
  {
    InventoryItem *result = CreateResult(owner, _results[i]);
    if (result)
    {
      create.Add(result);
      local.VarSet(Format("_result%d", i + 1), GameValueExt(result), true);
    }
  }

  if (!_action.IsEmpty())
  {
    // execute action
    GameDataNamespace *globals = GWorld->GetMissionNamespace();
    int gshandle = GGameState.BeginContext(&local);
#if USE_PRECOMPILATION
    GGameState.Execute(_action, _actionExp, GameState::EvalContext::_reportUndefined, globals, RString("CfgRecipes.") + _name + RString(".action"));
#else
    GGameState.Execute(_action, GameState::EvalContext::_reportUndefined, globals);
#endif
    GGameState.EndContext(gshandle);
  }

  // handle results
  if (destroyItem1)
  {
    Entity *parent = item1->GetHierarchyParent();
    if (parent) parent->RemoveOwnedItem(item1);
    item1->ItemDestroy();
  }
  if (destroyItem2)
  {
    Entity *parent = item2->GetHierarchyParent();
    if (parent) parent->RemoveOwnedItem(item2);
    item2->ItemDestroy();
  }
  for (int i=0; i<create.Size(); i++)
  {
    InventoryItem *item = create[i];
    if (owner->AddItem(item))
    {
      // create network object
      if (GWorld->GetMode() == GModeNetware)
        GetNetworkManager().CreateVehicle(item, VLTOut, "", -1);
    }
    else
    {
      // not handled - drop it
      create[i]->ItemDrop(owner, owner->FutureVisualState().Position(), owner->FutureVisualState().Direction());
      if (GWorld->GetMode() == GModeNetware)
        GetNetworkManager().CreateVehicle(item, VLTSlowVehicle, "", -1);
    }
  }

  return true;
}

/// functor will replace references to recipe ingrediences
class FormatRecipeFunc
{
protected:
  GameVarSpace &_local;

public:
  FormatRecipeFunc(GameVarSpace &local) : _local(local) {}

  RString operator ()(const char *&ptr)
  {
    const char *p = ptr;
    while (*p && (isalnum(*p) || *p == '_')) p++;
    RString varName = RString("_") + RString(ptr, p - ptr);
    ptr = p;
    varName.Lower();
    GameValue value;
    if (!_local.VarGet(varName, value)) return RString();
    if (value.GetType() != GameObject) return RString();

    Object *obj = static_cast<GameDataObject *>(value.GetData())->GetObject();
    if (!obj) return RString();
    return obj->GetDisplayName();
  }
};

INode *Recipe::GetDescription(Person *owner, InventoryItem *item1, InventoryItem *item2) const
{
  // prepare variables for condition expression if available
  GameVarSpace local(GGameState.GetContext(), false);
  local.VarSet("_owner", GameValueExt(owner), true);
  bool dummy1, dummy2; // need not to know what will be destroyed
  MatchesImpl(local, item1, item2, dummy1, dummy2);

  FormatRecipeFunc func(local);
  return CreateTextStructured(NULL, _displayName.ParseFormat(func));
}

void RecipeBank::Load(ParamEntryPar cls)
{
  int n = cls.GetEntryCount();
  Realloc(n);
  for (int i=0; i<n; i++)
  {
    ParamEntryVal entry = cls.GetEntry(i);
    if (entry.IsClass()) Add(new Recipe(entry));
  }
  Compact();
}

bool RecipeBank::Collect(Person *owner, InventoryItem *item1, InventoryItem *item2, const std::function<bool(const Recipe *recipe)> &add) const
{
  for (int i=0; i<Size(); i++)
  {
    const Recipe *recipe = Get(i);
    if (recipe->Matches(owner, item1, item2))
    {
      if (add(recipe)) return true;
    }
  }
  return false;
}

const Recipe *RecipeBank::Find(RString name) const
{
  for (int i=0; i<Size(); i++)
  {
    const Recipe *recipe = Get(i);
    if (stricmp(recipe->_name, name) == 0) return recipe;
  }
  return nullptr;
}

RecipeBank Recipes;

