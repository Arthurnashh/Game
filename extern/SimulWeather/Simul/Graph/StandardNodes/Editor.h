#ifndef SIMUL_GRAPH_STD_EDITOR_H
#define SIMUL_GRAPH_STD_EDITOR_H
namespace simul
{
	namespace graph
	{
		namespace standardnodes
		{
			struct ActionCallback
			{
				virtual bool ShouldDo()
				{
					return false;
				}
				virtual void ReportEnabled(bool val)
				{
					val;
				}
			};

			class Editor
			{
			public:
				Editor(){}
				virtual ~Editor(){}
				virtual void EnableUndo(){}
				virtual void DisableUndo(){}
				virtual void StoreForUndo(){}
				virtual void Undo(){}
				virtual void Redo(){}
				virtual void Modify(){}
				virtual void ClearUndoList(){}
				virtual bool CanUndo(){return true;}
			};
		}
	}
}
#endif