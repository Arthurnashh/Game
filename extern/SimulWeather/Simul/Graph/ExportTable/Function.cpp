#include "Function.h"
#include "CallBinding.h"
#include "Callx64.h"
#include <stdarg.h>
using namespace simul::graph::exporttable;

ExportFunction::ExportFunction()
{
	proc=0;
	Virtual=false;
	VirtualImplementation=false;
	Static=false;
}


size_t ParamSize(const std::string &typenam)
{
	// A reference is a pointer:
	if(typenam.find_last_of("&")==typenam.length()-1)
		return sizeof(void*);
	if(typenam.find_last_of("*")==typenam.length()-1)
		return sizeof(void*);
	if(typenam=="char"||typenam=="unsigned char")
		return sizeof(char);
	if(typenam=="int"||typenam=="unsigned"||typenam=="unsigned int"||
		typenam=="float")
		return sizeof(int);
	return 8;
}

FunctionReturnValue ExportFunction::Invoke(void* this_pointer,...) const
{
	FunctionReturnValue frv;
	frv.float_ret=0.f;
	frv.ret_val=0;

	size_t parm_list_size=0;
	for(size_t i=0;i<param_types.size();i++)
	{	
		parm_list_size+=ParamSize(param_types[i]);
	}
#ifdef WIN32
	char *params=new char[parm_list_size];
	char *this_param=params;
	size_t arg_num=0;
	va_list args;
	va_start(args,this_pointer);
	for(size_t i=0;i<param_types.size();i++)
	{
		if(arg_num<param_types.size())
		{
			if(param_types[arg_num].find_last_of("&")==param_types[arg_num].length()-1
				||param_types[arg_num].find_last_of("*")==param_types[arg_num].length()-1)
			{
				 void *vp_val=va_arg(args,void *);
				 void **vp_param=(void **)this_param;
				 *vp_param=vp_val;
				 vp_param++;
				 this_param=(char*)vp_param;
			}
			else if(param_types[arg_num]=="bool")
			{
				 int int_val=va_arg(args,int);
				 int *int_param=(int*)this_param;
				 *int_param=int_val;
				 int_param++;
				 this_param=(char*)int_param;
			}
			else if(param_types[arg_num]=="char")
			{
				 int int_val=va_arg(args,int);
				 int *int_param=(int*)this_param;
				 *int_param=int_val;
				 int_param++;
				 this_param=(char*)int_param;
			}
			else if(param_types[arg_num]=="unsigned char")
			{
				 unsigned char uchar_val=va_arg(args,unsigned char);
				 unsigned char *uchar_param=(unsigned char*)this_param;
				 *uchar_param=uchar_val;
				 uchar_param++;
				 this_param=(char*)uchar_param;
			}
			else if(param_types[arg_num]=="int")
			{
				 int int_val=va_arg(args,int);
				 int *int_param=(int*)this_param;
				 *int_param=int_val;
				 int_param++;
				 this_param=(char*)int_param;
			}
			else if(param_types[arg_num]=="unsigned"||param_types[arg_num]=="unsigned int")
			{
				 unsigned unsigned_int_val=va_arg(args,unsigned);
				 unsigned *unsigned_int_param=(unsigned*)this_param;
				 *unsigned_int_param=unsigned_int_val;
				 unsigned_int_param++;
				 this_param=(char*)unsigned_int_param;
			}
			else if(param_types[arg_num]=="float")
			{
				 float float_val=(float)va_arg(args,double);
				 float *float_param=(float*)this_param;
				 *float_param=float_val;
				 float_param++;
				 this_param=(char*)float_param;
			}
		}
		arg_num++;
	}
	va_end(args);

	if(calling_convention=="__stdcall")
		frv=Call_stdcall(params,parm_list_size,proc);
	else if(calling_convention=="__cdecl")
		frv=Call_cdecl(params,parm_list_size,proc);
	else if(calling_convention=="__thiscall")
	{
		assert(this_pointer);
		frv=Call_thiscall(this_pointer,params,parm_list_size,proc);
	}
	if(return_type=="float")
	{
		float floatval=0;
		_asm
		{
			fstp floatval
		}
		frv=floatval;
	}
	return frv;
#endif
#ifdef WIN64
	size_t arg_num=0;
	size_t num_int_args=0;
	size_t num_float_args=0;
	__int64 int_args[4]={0,0,0,0};
	float FLT_ARGS[4]={0,0,0,0};
	bool member_fn=false;
	unsigned inputs=0;
	if(calling_convention=="win64_thiscall")
	{
		member_fn=true;
		int_args[0]=(__int64)this_pointer;
		arg_num++;
		num_int_args++;
	}
	va_list args;
	va_start(args,this_pointer);

	__int64 this_arg=1;
	while(this_arg!=0xcccccccccccccccc&&(arg_num-member_fn?1:0)<param_types.size())
	{
		this_arg=va_arg(args,__int64);
		if(arg_num<param_types.size()+member_fn?1:0)
		{
			const std::string &param_type=param_types[arg_num-member_fn?1:0];
			if(arg_num<4)
			{
				if(param_type=="float")
				{
					double &d=*(reinterpret_cast<double*>(&this_arg));
					float f=(float)d;
					FLT_ARGS[arg_num]=f;
					num_float_args++;
					inputs|=1<<(arg_num);
				}
				else
				{
					size_t sz=ParamSize(param_type);
					if(sz==1)
					{
						unsigned char c=*(reinterpret_cast<unsigned char*>(&this_arg));
						unsigned char *uc_ptr=(unsigned char *)(&int_args[arg_num]);
						*uc_ptr=c;
					}
					if(sz==4)
					{
						unsigned int c=*(reinterpret_cast<unsigned int*>(&this_arg));
						int_args[arg_num]=c;
						unsigned int *ui_ptr=(unsigned int *)(&int_args[arg_num]);
						*ui_ptr=c;
					}
					if(sz==5)
					{
						unsigned __int64 c=*(reinterpret_cast<unsigned __int64*>(&this_arg));
						int_args[arg_num]=c;
					}
					num_int_args++;
				}
			}
		}
		arg_num++;
	}
	va_end(args);
	static float floatval[12];

	switch(inputs)
	{
	case CALLX64III:
		frv=(int_or_ptr)Callx64iii(proc,int_args[0],int_args[1],int_args[2]);
		break;
	case CALLX64IIF:
		frv=(int_or_ptr)Callx64iif(proc,int_args[0],int_args[1],FLT_ARGS[2]);
		break;
	case CALLX64IFI:
		frv=(int_or_ptr)Callx64ifi(proc,int_args[0],FLT_ARGS[1],int_args[2]);
		break;
	case CALLX64FII:
		frv=(int_or_ptr)Callx64fii(proc,FLT_ARGS[0],int_args[1],int_args[2]);
		break;
	case CALLX64IFF:
		frv=(int_or_ptr)Callx64iff(proc,int_args[0],FLT_ARGS[1],FLT_ARGS[2]);
		break;
	case CALLX64FIF:
		frv=(int_or_ptr)Callx64fif(proc,FLT_ARGS[0],int_args[1],FLT_ARGS[2]);
		break;
	case CALLX64FFI:
		frv=(int_or_ptr)Callx64ffi(proc,FLT_ARGS[0],FLT_ARGS[1],int_args[2]);
		break;
	case CALLX64FFF:
		frv=(int_or_ptr)Callx64fff(proc,FLT_ARGS[0],FLT_ARGS[1],FLT_ARGS[2]);
		break;
	}

	//frv=(int_or_ptr)Call_win64_call(int_args,3,proc,FLT_ARGS,3);
	if(return_type=="float")
	{
		ReturnFloatx64(floatval);
		frv=floatval[0];
	}
	return frv;
#endif
}
