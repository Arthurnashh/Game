#pragma once

namespace TreeEngine
{


///Plant control data object - new generation TreeEngine.
/**
*  This interface is used to cooperate with tree database.
*
* Tree Database is not part of TreeEngine. Application must define own database 
*/
class IPlantControlDB
{
public:
  ///Notifies the database, that tree engine is processing new component
  /**
  * @param compName full specified name of component (relative to global namespace)
  */    
  virtual void SetActiveComponentName(const RString &compName)=0;

  ///Retrieves data from the database
  /**
  * Function retrieves script data. Database can cache the parsed data to increase performance
  * @param fieldName name of field relative to current component. If field contains a path
  * parameter is searched through hierarchy
  * @return function returns script data. If data doesn't exist in database, function
  * returns empty GameData (null).
  */
  virtual const GameValue &GetDBItemScript(const RString &fieldName)=0;
  ///Retrieves data from the database
  /**
  * Function retrieves script data. Data are received as string, and reader should perform conversion by own.
  * @param fieldName name of field relative to current component. If field contains a path
  * parameter is searched through hierarchy
  * @return function returns pointer to RString object. If field doesn't exist, function
  * returns NULL.
  */
  virtual const char *GetDBItemString(const RString &relName)=0;
  ///Translates name to component name
  /**
   * Gets full component name. 
   * @param relName Relative component name. Component is searched in current package.
   * if component is not found, function processes parent package, etc.
   * If name of package is specified, function returns name of startup component in package
   * @return found component name, or empty string if component was not found.
   */
  virtual const RStringI &GetFullComponentName(const RStringI &partName)=0;
  ///Changes current namespace
  /**
  * Namespace is prefix before field name. Implementation will insert namespace 
  * before field separated by dot. It doesn't affect active component name. All
  * fields will be searched relative to current component.
  * @param ns new namespace. Set to empty "" to return default namespace
  * @note Current namespace is reset every time the function SetActiveComponent is called.
  */
  virtual void SetNamespace(const RString &ns)=0;
  ///Profile calling
  /**
  * Called for every component, before component calculations are started. This function
  * is called after SetActiveComponentName is processed
  */
  virtual void Profile_ComponentStart() {}
  ///Profile calling
  /**
  *  Called for every component, after component calculations are done. 
  */
  virtual void Profile_ComponentStop() {}

  ///Returns id of first component for enumeration
  /**
  * @retval 0 no components available
  * @retval other id of first component
  */
  virtual int GetFirstComponentId()=0;

  ///Retrieves global settings
  /**
   In most of cases, it receives settings from !global namespace
   */
  virtual const char *GetGlobalDBItemString(const RString &item)=0;
  ///Retrieves global settings
  /**
   In most of cases, it receives settings from !global namespace
   */
  virtual GameValue GetGlobalDBItemScript(const RString &item)=0;

  struct ComponentDesc
  {
    RString type;
    RString name;
    RString script;
    int defIndex;
    ComponentDesc(const RString &type, const RString &name, const RString &script, int index):
    type(type),name(name),script(script),defIndex(index) {}
    ComponentDesc() {}
  };

  ///Returns current component at id and increases id
  /**
  * To enumerate components, call GetFirstComponentId to obtain id. Then repeat until
  * id is zero. Each GetNextComponent call returns current component name and changes
  * variable to contain id of next component.
  * @param id enumeration variable that contains id of component to retrieve. After
  * function is processed, variable contains id of next component or zero, if there
  * are no more components.
  * @return Component description
  */
  virtual ComponentDesc GetNextComponent(int &id)=0;

  ///Retrieves first texture id
  /**
  * @retval 0 no texture available
  * @retval other id of first texture
  */
  virtual int GetFirstTextureId()=0;

  struct TextureDesc
  {
    int textureIndex;
    RString textureName;

    enum AddrMode
    {
      Clamp,
      Wrap,
      Mirror
    };

    AddrMode addr_u:4;
    AddrMode addr_v:4;
    bool linearAlpha;

    TextureDesc():textureIndex(0) {}
    TextureDesc(int textureIndex, const RString &textureName, AddrMode au, AddrMode av,bool linearAlpha):textureIndex(textureIndex),
      textureName(textureName),addr_u(au),addr_v(av),linearAlpha(linearAlpha) {}
  };

  ///Returns current texture at id and increases id
  /**
  * To enumerate textures, call GetFirstTextureId to obtain id. Then repeat until
  * id is zero. Each GetNextTexture call returns current texture and changes
  * variable to contain id of next texture.
  * @param id enumeration variable that contains id of texture to retrieve. After
  * function is processed, variable contains id of next textyre or zero, if there
  * are no more textures. 
  * @note Id of texture and texture index aren't the same values. The id can be an
  * internal database index.
  * @return Texture description
  */
  virtual TextureDesc GetNextTexture(int &id)=0;


  virtual RString GetStartComponent()=0;
};

}