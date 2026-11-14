#include "AnimatedObject.h"
using namespace simul;
using namespace graph;
using namespace standardnodes;
AnimatedObject::AnimatedObject(const char *what_name):OrientedObject(what_name)
{
}

AnimatedObject::~AnimatedObject()
{
}

bool AnimatedObject::IsAnimEnabled(const char *anim_name) const
{
	std::string name(anim_name);
	std::map<std::string,bool>::const_iterator i=enabled_anims.find(name);
	if(i!=enabled_anims.end())
		return true;
	return false;
}

bool AnimatedObject::IsAnimLooped(const char *anim_name) const
{
	std::map<std::string,bool>::const_iterator i=enabled_anims.find(anim_name);
	if(i!=enabled_anims.end())
		return i->second;
	return false;
}

void AnimatedObject::EnableAnim(const char *anim_name,bool loop)
{
	std::string name(anim_name);
	if(enabled_anims.find(name)==enabled_anims.end())
	{
		enabled_anims[name]=loop;
	}
}								 				

void AnimatedObject::DisableAnim(const char *anim_name)
{
	std::string name(anim_name);
	std::map<std::string,bool>::iterator i=enabled_anims.find(name);
	if(i!=enabled_anims.end())
		enabled_anims.erase(i);
}