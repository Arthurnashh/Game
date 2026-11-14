#include "../wpch.hpp"
#include "powereditem.hpp"
#include <lib/person.hpp>
#include <lib/Network/network.hpp>
#include <lib/AI/ai.hpp>

PoweredItem::PoweredItem(const EntityAIType *type, bool fullCreate)
  : base(type, fullCreate)
  , _isOn(false)
{
  SetTargetCategory("loot");
}

bool PoweredItem::ForEachAttachment(const std::function<bool(const InventoryItem *item)> &func) const
{
  for (int i=0; i<NAttachments(); i++)
  {
    const InventoryItem *item = GetAttachment(i);
    if ( item && (func(item) || item->ForEachAttachment(func)) ) return true;
  }
  return false;
}

bool PoweredItem::ForEachAttachment(const std::function<bool(InventoryItem *item)> &func)
{
  for (int i=0; i<NAttachments(); i++)
  {
    InventoryItem *item = GetAttachment(i);
    if ( item && (func(item) || item->ForEachAttachment(func)) ) return true;
  }
  return false;
}

float PoweredItem::DrawingAreaRatio() const
{
  if(IsAttached())
    return 1.0f;

  Entity* parent = NULL;
  for(Entity* ent = GetHierarchyParent(); ent; ent = parent)
  {
    parent = GetHierarchyParent();
    if(parent == ent || parent == NULL)
    {
      if(dynamic_cast<Person*>(ent))
        return 1.0f;
      break;
    }
  }

  return 0.01f;
};

bool PoweredItem::ForEachItem(const std::function<bool(InventoryItem *)> &func)
{
  // weapons in containers
  ResourceSupply *supply = GetSupply();
  if (supply && supply->GetItemsCargo().ForEachItem(func)) return true;

  for (int j=0; j<NAttachments(); j++)
  {
    InventoryItem *attachment = GetAttachment(j);
    if ( attachment && func(attachment) ) return true;
  }
  return false;
}

bool PoweredItem::ForEachItem(const std::function<bool(const InventoryItem *)> &func) const
{
  // weapons in containers
  const ResourceSupply *supply = GetSupply();
  if (supply && supply->GetItemsCargo().ForEachItem(func)) return true;

  for (int j=0; j<NAttachments(); j++)
  {
    const InventoryItem *attachment = GetAttachment(j);
    if ( attachment && func(attachment) ) return true;
  }
  return false;
}

void PoweredItemType::Load(ParamEntryPar par, const char *shape)
{
  base::Load(par, shape);
  ParamEntryPtr entry = par.FindEntry("energyResources");
  if (entry && entry->IsArray())
  {
    int n = entry->GetSize();
    m_energySupply.Realloc(n);
    for (int i=0; i<n; i++)
    {
      if ((*entry)[i].IsArrayValue())
      {
        const IParamArrayValue& energy = (*entry)[i];
        if ( energy.GetItemCount() != 2 || !energy.GetItem(0)->IsTextValue() || !energy.GetItem(1)->IsFloatValue() )
          continue;
        m_energySupply.AddFast(PoweredItemType::EnergySupply(energy.GetItem(0)->GetValue(), energy.GetItem(1)->GetFloat()));
      }
    }
  }
  if (const ConstParamEntryPtr entry = par.FindEntry("internalResources"))
  {
    const int n = entry->GetSize();
    m_internalSupply.Realloc(n);
    for (int i = 0; i < n; ++i)
    {
      if ((*entry)[i].IsArrayValue())
      {
        const IParamArrayValue& energy = (*entry)[i];
        if ( energy.GetItemCount() != 2 || !energy.GetItem(0)->IsTextValue() || !energy.GetItem(1)->IsFloatValue() )
          continue;
        m_internalSupply.AddFast(PoweredItemType::EnergySupply(energy.GetItem(0)->GetValue(), energy.GetItem(1)->GetFloat()));
      }
    }
  }
}

void PoweredItem::SwitchOn(bool onOff)
{
  if (_isOn!=onOff)
  {
    _isOn = onOff;
   if (GetNetworkManager().IsServerOwner())
   {
      if (_isOn)
      {
        if (IsEventHandler(EEPowerOn))
          OnEvent(EEPowerOn);
      }
      else
      {
        if (IsEventHandler(EEPowerOff))
          OnEvent(EEPowerOff);
      }
      //SetPilotLight(onOff); // like headTorch and other devices lights
    }
     else
     {
       UpdateLights();
     }
  }
}

bool PoweredItem::IsPowered() const
{
  if (IsDamageDestroyed()) return false; // destroyed items cannot operate
  if (Type()->m_energySupply.Size())
  {
    for (int i=0; i<NAttachments(); i++)
    {
      Ref<InventoryItem> attachment = const_cast<InventoryItem *>(GetAttachment(i));
      if (!attachment) continue;
      GameVarSpace * vars = attachment->GetVars();
      if (!vars) continue;
      for (int j=0; j<Type()->m_energySupply.Size(); j++)
      {
        GameValue val;
        if (vars->VarGet(Type()->m_energySupply[j].m_varName, val))
        { // there is some attachment with supported energy variable
          if (static_cast<float>(val) > 0)
            return true;
        }
      }
    }
  }

  if (Type()->m_internalSupply.Size())
  {
    if (const GameVarSpace * const vars = GetVars())
    {
      const PoweredItemType::EnergySupply & power_cfg = Type()->m_internalSupply[0];
      GameValue val;
      if (vars->VarGet(power_cfg.m_varName, val))
        if (static_cast<float>(val) > 0.0f)
          return true;
    }
  }
  return false;
}

void PoweredItem::Simulate(float deltaT, SimulationImportance prec)
{
  if (GetNetworkManager().IsServerOwner())
  {
    if (_isOn)
    { // energy consumed only when switched ON
      float energizeTime = deltaT;  //remaining time needed to be covered by energy
      if (Type()->m_energySupply.Size())
      {
        for (int i=0; i<NAttachments(); i++)
        {
          Ref<InventoryItem> attachment = GetAttachment(i);
          if (!attachment) continue;
          GameVarSpace *vars = attachment->GetVars();
          if (!vars) continue;
          for (int j=0; j<Type()->m_energySupply.Size(); j++)
          {
            GameValue val;
            if (vars->VarGet(Type()->m_energySupply[j].m_varName, val))
            { // there is some attachment with supported energy variable
              float energy = static_cast<float>(val);
              if (energy)
              { // asume there is enough energy, if not, next simulation step would detect it
                energy = energy - Type()->m_energySupply[j].m_input * energizeTime;
                if (energy < 0.0f) energy = 0.0f;
                energizeTime = 0.0f; //finished
                //DIAG_MESSAGE(250, Format("Power: %.1f", energy));
                vars->VarSet(Type()->m_energySupply[j].m_varName, energy); // energy consumed from the variable
                break;
              }
            }
          }
          if (energizeTime == 0.0f) break;
        }
      }

      if (Type()->m_internalSupply.Size())
      {
        float energy = 0.0f;
        const PoweredItemType::EnergySupply & power_cfg = Type()->m_internalSupply[0];
        if (GameVarSpace * const vars = GetVars())
        {
          GameValue val;
          if (vars->VarGet(power_cfg.m_varName, val))
            energy = val;

          if (energy > 0.0f)
          {
            energy = energy - power_cfg.m_input * energizeTime;
            if (energy < 0.0f)
              energy = 0.0f;
            energizeTime = 0.0f;
            _internalEnergy = energy;
            vars->VarSet(power_cfg.m_varName, energy); // energy consumed from the variable
          }
        }
      }

      if (energizeTime != 0.0f)
      { //not enough energy, switchOFF
        SwitchOn(false);
      }
    }
  }

  ForEachItem([deltaT,prec](InventoryItem *item) -> bool { item->Simulate(deltaT, prec); return false; } );

  // user animations (both for local and server entities)
  if (!IsAnimatorRequired()) SimulateAnimations(deltaT,prec); 
}

LSError PoweredItem::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar));
  CHECK(ar.Serialize("isOn", _isOn, false));
  return LSOK;
}

