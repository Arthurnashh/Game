#pragma once
#include <Es/Strings/rString.hpp>
#include <El/ParamFile/paramFileDecl.hpp>
#include <lib/vehicle.hpp>
#include <functional>
#include "inventory.hpp"
class InventoryItem;
class INode;
class GameVarSpace;

// Crafting recipes
struct Recipe
{
  struct InputSpec
  {
    RStringB _text;
    Ref<const EntityType> _type;
#if USE_PRECOMPILATION
    CompiledExpression _expression;
#endif
    ClassIsMovable(InputSpec);

    void Init(RStringB text);
    bool Matches(InventoryItem *item, RString context) const;
  };
  // unique class name
  RStringB _name;
  // user readable name
  RStringB _displayName;
  // involved items
  AutoArray<InputSpec> _material;
  AutoArray<InputSpec> _tools;
  AutoArray<RStringB> _results;
  // event handlers
  /// when the action can be added
  RStringB _condition;
  RStringB _action;
#if USE_PRECOMPILATION
  CompiledExpression _conditionExp;
  CompiledExpression _actionExp;
#endif

  Recipe (ParamEntryPar cls);

  bool Matches (Person *owner, InventoryItem *item1, InventoryItem *item2) const;
  bool Process (Person *owner, InventoryItem *item1, InventoryItem *item2) const;

  INode *GetDescription (Person *owner, InventoryItem *item1, InventoryItem *item2) const;

private:
  // implementation - check if recipes matches (shared part of Matches, Process)
  bool MatchesImpl (GameVarSpace &local, InventoryItem *item1, InventoryItem *item2, bool &destroyItem1, bool &destroyItem2) const;
};

struct RecipeBank : public AutoArray< SRef<const Recipe> >
{
  void Load (ParamEntryPar cls);
  bool Collect (Person *owner, InventoryItem *item1, InventoryItem *item2, const std::function<bool(const Recipe *recipe)> &add) const;
  const Recipe * Find (RString name) const;
};

extern RecipeBank Recipes;

