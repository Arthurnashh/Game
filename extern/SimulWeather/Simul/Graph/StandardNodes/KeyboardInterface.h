#ifndef KEYBOARDINTERFACE_H
#define KEYBOARDINTERFACE_H

namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
			//! an interface for a class that can take keypresses.
			class KeyboardInterface
			{
			public:
				//! Inform the object that this key has been depressed.
				virtual void HandleKeyDown(int k)=0;
				//! Inform the object that this key has been released.
				virtual void HandleKeyUp(int k)=0;
			};
			//! an interface for a class that can read the keyboard.
			class KeyboardInputProviderInterface
			{
			public:
				//! Return true if the key is depressed, false otherwise.
				virtual bool IsKeyDown(char k)=0;
			};
		}
	}
}
#endif
