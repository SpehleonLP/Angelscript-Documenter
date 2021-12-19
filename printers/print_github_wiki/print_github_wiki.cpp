
#include "../../src/asdocumenter.h"

#if AS_PRINTABLE == 1

#include <angelscript.h>
#include <../source/as_objecttype.h>
#include <system_error>
#include <cstring>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>

struct kProperty
{
	const char * name{0L};
	int  type{};
	int  index_type{};
	int  offset{};
	intptr_t setter{0};
	intptr_t getter{0};
};

enum class Overload
{
	None,

	UNARY_BEGIN,

	opConv = UNARY_BEGIN,
	opImplConv,
	opCast, opImplCast,

	opNeg,
	opCom,
	opPreInc,
	opPreDec,

	opPostInc,
	opPostDec,

	opEquals,
	opCmp,

	UNARY_END,

	BINARY_BEGIN = UNARY_END,

	opAssign = BINARY_BEGIN,
	opAddAssign,
	opSubAssign,
	opMulAssign,
	opDivAssign,
	opModAssign,
	opPowAssign,
	opAndAssign,
	opOrAssign,
	opXorAssign,
	opShlAssign,
	opShrAssign,
	opUShrAssign,
	opHndlAssign,

	opAdd, 	opAdd_r,
	opSub, 	opSub_r,
	opMul, 	opMul_r,
	opDiv, 	opDiv_r,
	opMod, 	opMod_r,
	opPow, 	opPow_r,
	opAnd, 	opAnd_r,
	opOr, 	opOr_r,
	opXor, 	opXor_r,
	opShl, 	opShl_r,
	opShr, 	opShr_r,
	opUShr,	opUShr_r,

	BINARY_END,

	opIndex = BINARY_END,
	opCall,
};


#define ASSIGN "Assign"
#define TEST_OVERLOAD(x) if(strcmp(name, #x) == 0) return Overload::x

static Overload  IsOperatorOverload(const char * name)
{
	if(name[0] != 'o' || name[1] != 'p')
		return Overload::None;

	TEST_OVERLOAD(opNeg);
	TEST_OVERLOAD(opCom);
	TEST_OVERLOAD(opPreInc);
	TEST_OVERLOAD(opPreDec);

	TEST_OVERLOAD(opPostInc);
	TEST_OVERLOAD(opPostDec);

	TEST_OVERLOAD(opEquals);
	TEST_OVERLOAD(opCmp);

	TEST_OVERLOAD(opAssign);
	TEST_OVERLOAD(opAddAssign);
	TEST_OVERLOAD(opSubAssign);
	TEST_OVERLOAD(opMulAssign);
	TEST_OVERLOAD(opDivAssign);
	TEST_OVERLOAD(opModAssign);
	TEST_OVERLOAD(opPowAssign);
	TEST_OVERLOAD(opAndAssign);
	TEST_OVERLOAD(opOrAssign);
	TEST_OVERLOAD(opXorAssign);
	TEST_OVERLOAD(opShlAssign);
	TEST_OVERLOAD(opShrAssign);
	TEST_OVERLOAD(opUShrAssign);
	TEST_OVERLOAD(opHndlAssign);

	TEST_OVERLOAD(opAdd); 	TEST_OVERLOAD(opAdd_r);
	TEST_OVERLOAD(opSub); 	TEST_OVERLOAD(opSub_r);
	TEST_OVERLOAD(opMul); 	TEST_OVERLOAD(opMul_r);
	TEST_OVERLOAD(opDiv); 	TEST_OVERLOAD(opDiv_r);
	TEST_OVERLOAD(opMod); 	TEST_OVERLOAD(opMod_r);
	TEST_OVERLOAD(opPow); 	TEST_OVERLOAD(opPow_r);
	TEST_OVERLOAD(opAnd); 	TEST_OVERLOAD(opAnd_r);
	TEST_OVERLOAD(opOr); 	TEST_OVERLOAD(opOr_r);
	TEST_OVERLOAD(opXor); 	TEST_OVERLOAD(opXor_r);
	TEST_OVERLOAD(opShl); 	TEST_OVERLOAD(opShl_r);
	TEST_OVERLOAD(opShr); 	TEST_OVERLOAD(opShr_r);
	TEST_OVERLOAD(opUShr);	TEST_OVERLOAD(opUShr_r);

	TEST_OVERLOAD(opIndex);

	TEST_OVERLOAD(opCall);

	TEST_OVERLOAD(opConv); TEST_OVERLOAD(opImplConv);
	TEST_OVERLOAD(opCast); TEST_OVERLOAD(opImplCast);

	return Overload::None;
}

static std::string GetTypeName(asITypeInfo * info, bool isFileName = false)
{
	if(info == nullptr)
		return "?";

	std::string name = info->GetName();

	auto nameSpace = info->GetNamespace();
	if(nameSpace && *nameSpace != '\0')
		name = std::string(nameSpace) + (isFileName? "__" : "::") + name;

	return name;
}

static std::string GetFileName(asITypeInfo * info)
{
	if(info == nullptr)
		return "?";

	std::string name = info->GetName();

	auto nameSpace = info->GetNamespace();
	if(nameSpace && *nameSpace != '\0')
		name = std::string(nameSpace) + "__" + name;

	return name;
}

static std::string GetFunctionName(asIScriptFunction * info, bool isGlobal)
{
	std::string name = info->GetName();

	if(isGlobal)
	{
		auto nameSpace = info->GetNamespace();
		if(nameSpace && *nameSpace != '\0')
			name = std::string(nameSpace) + "::" + name;
	}

	asUINT i;
	for(i = 0; i < name.size(); ++i)
	{
		if(name[i] == '_')
		{
			name.insert(name.begin()+i, '\\');
			++i;
		}
	}

	return name;
}

static std::string EscapeUnderscores(std::string name)
{
	asUINT i;
	for(i = 0; i < name.size(); ++i)
	{
		if(name[i] == ':')
		{
			name[i] = '_';
		}
	}

	return name;
}

std::string GetTypeName(asIScriptEngine * engine, int type, bool isFileName = false)
{
	switch(type)
	{
	case asTYPEID_VOID:		return "void";
	case asTYPEID_BOOL: 	return "bool";
	case asTYPEID_INT8: 	return "int8";
	case asTYPEID_INT16: 	return "int16";
	case asTYPEID_INT32: 	return "int";
	case asTYPEID_INT64: 	return "int64";
	case asTYPEID_UINT8: 	return "uint8";
	case asTYPEID_UINT16: 	return "uint16";
	case asTYPEID_UINT32: 	return "uint";
	case asTYPEID_UINT64: 	return "uint64";
	case asTYPEID_FLOAT: 	return "float";
	case asTYPEID_DOUBLE: 	return "double";
	default:
		break;
	}

	auto info = engine->GetTypeInfoById(type);
	return GetTypeName(info, isFileName);
}

static std::string GetLink(asDocumenter * doc, asIScriptEngine * engine, int type)
{
	int parent = doc->GetParentType(type);

	if(!parent)
	{
		return GetTypeName(engine, type, true);
	}

	int p2 = parent;
	do
	{
		parent = p2;
		p2 = doc->GetParentType(parent);
	} while(p2);

	return GetTypeName(engine, parent, true) + "#" + GetTypeName(engine, type, true);
}

static std::string GetTypeName(asDocumenter * doc, asIScriptEngine * engine, int type, asDWORD flags)
{
	std::stringstream ss;

	if(flags & asTM_CONST)
		ss << "const ";

	auto name = GetTypeName(engine, type);

	auto description_str = doc->GetObjectType(type);

	auto info = engine->GetTypeInfoById(type);


	if(type < 12)
	{
		ss << name;
	}
	else if(strcmp(info->GetName(), "array") == 0)
	{
		if(type & asTYPEID_HANDLETOCONST)
		{
			ss << "const ";
		}

		int subtype = info->GetSubTypeId();

		if(subtype)
		{
			ss << GetTypeName(doc, engine, subtype, 0) << "[]";
		}

		if(type & (asTYPEID_HANDLETOCONST | asTYPEID_OBJHANDLE))
		{
			ss << '@';
		}

		return ss.str();
	}
	else
	{
		if(type & asTYPEID_HANDLETOCONST)
		{
			ss << "const ";
		}

		if(name != "T")
		{
			ss << "[" << name << "]" << "(" << GetLink(doc, engine, type);

			if(description_str)
				ss << " \"" << description_str << "\"";

			ss << ")";
		}
		else
		{
			ss << 'T';
		}

		if(type & asTYPEID_TEMPLATE)
		{
			ss << "<T>";
		}

		int subtype = info->GetSubTypeId();

		if(subtype > 0)
		{
			ss << "<" << GetTypeName(doc, engine, subtype, 0) << ">";
		}

		if(type & (asTYPEID_HANDLETOCONST | asTYPEID_OBJHANDLE))
		{
			ss << '@';
		}
	}

	if((flags & asTM_INOUTREF) == asTM_INOUTREF)
	{
		ss << "&inout";
	}
	else if(flags & asTM_OUTREF)
	{
		ss << "&out";
	}

	return ss.str();
}

static std::string GetOperatorOverload(asDocumenter * doc, asIScriptEngine * engine, asIScriptFunction * func, Overload op)
{
	const asUINT n = func->GetParamCount();

	if(Overload::UNARY_BEGIN <= op && op < Overload::UNARY_END)
	{
		if(n != 0) return "";
	}

	if(Overload::BINARY_BEGIN <= op && op < Overload::BINARY_END)
	{
		if(n != 1) return "";
	}

	asDWORD flags{};
	int type = func->GetReturnTypeId(&flags);
	std::string r_class = GetTypeName(doc, engine, type, flags);
	std::string r_class_nf = flags? GetTypeName(doc, engine, type, 0) : r_class;

	flags = 0;
	if(func->IsReadOnly()) flags |= asTM_CONST;
	std::string m_class = GetTypeName(doc, engine, func->GetObjectType()->GetTypeId(), flags);

	std::string p_class;

	if(n > 0)
	{
		func->GetParam(0, &type, &flags, nullptr, nullptr);
		p_class = GetTypeName(doc, engine, type, flags);
	}

	std::stringstream ss;

	switch(op)
	{
	case Overload::opConv:
		ss << r_class << " = *" << r_class_nf << "*(" << m_class << ")";
		return ss.str();
	case Overload::opImplConv:
		ss << r_class << " = " << m_class;
		return ss.str();
	case Overload::opCast:
		ss << r_class << " @= *cast< " << r_class << ">*(" << m_class << ")";
		return ss.str();
	case Overload::opImplCast:
		ss << r_class << " @= " << m_class;
		return ss.str();

	case Overload::opNeg:
		ss << r_class << " = *-* " << m_class;
		return ss.str();
	case Overload::opCom:
		ss << r_class << " = *~* " << m_class;
		return ss.str();
	case Overload::opPreInc:
		ss << r_class << " = *++* " << m_class;
		return ss.str();
	case Overload::opPreDec:
		ss << r_class << " = *--* " << m_class;
		return ss.str();

	case Overload::opPostInc:
		ss << r_class << " = " << m_class << " *++*";
		return ss.str();
	case Overload::opPostDec:
		ss << r_class << " = " << m_class << " *--*";
		return ss.str();

	case Overload::opEquals:
		ss << r_class << " = " << m_class <<  "*==* " << p_class << "\n\n";
		ss << r_class << " = " << m_class <<  "*!=* " << p_class;
		return ss.str();

	case Overload::opCmp:
		ss << r_class << " = " << m_class <<  "*<* " << p_class << "\n\n";
		ss << r_class << " = " << m_class <<  "*<=* " << p_class << "\n\n";
		ss << r_class << " = " << m_class <<  "*>* " << p_class << "\n\n";
		ss << r_class << " = " << m_class <<  "*>=* " << p_class << "\n\n";
		ss << r_class << " = " << m_class <<  "*is* " << p_class << "\n\n";
		ss << r_class << " = " << m_class <<  "*!is* " << p_class;
		return ss.str();

	case Overload::opAssign:
		ss << r_class << " = " << m_class << " *=* " << p_class;
		return ss.str();
	case Overload::opAddAssign:
		ss << r_class << " = " << m_class << " *+=* " << p_class;
		return ss.str();
	case Overload::opSubAssign:
		ss << r_class << " = " << m_class << " *-=* " << p_class;
		return ss.str();
	case Overload::opMulAssign:
		ss << r_class << " = " << m_class << " *\\*=* " << p_class;
		return ss.str();
	case Overload::opDivAssign:
		ss << r_class << " = " << m_class << " */=* " << p_class;
		return ss.str();
	case Overload::opModAssign:
		ss << r_class << " = " << m_class << " *%=* " << p_class;
		return ss.str();
	case Overload::opPowAssign:
		ss << r_class << " = " << m_class << " *\\*\\*=* " << p_class;
		return ss.str();
	case Overload::opAndAssign:
		ss << r_class << " = " << m_class << " *&=* " << p_class;
		return ss.str();
	case Overload::opOrAssign:
		ss << r_class << " = " << m_class << " *|=* " << p_class;
		return ss.str();
	case Overload::opXorAssign:
		ss << r_class << " = " << m_class << " *^=* " << p_class;
		return ss.str();
	case Overload::opShlAssign:
		ss << r_class << " = " << m_class << " *<<=* " << p_class;
		return ss.str();
	case Overload::opShrAssign:
		ss << r_class << " = " << m_class << " *>>=* " << p_class;
		return ss.str();
	case Overload::opUShrAssign:
		ss << r_class << " = " << m_class << " *>>>=* " << p_class;
		return ss.str();
	case Overload::opHndlAssign:
		ss << r_class << " = " << m_class << " *@=* " << p_class;
		return ss.str();

	case Overload::opAdd:
		ss << r_class << " = " << m_class << " *+* " << p_class;
		return ss.str();
	case Overload::opAdd_r:
		ss << r_class << " = " << p_class << " *+* " << m_class;
		return ss.str();
	case Overload::opSub:
		ss << r_class << " = " << m_class << " *-* " << p_class;
		return ss.str();
	case Overload::opSub_r:
		ss << r_class << " = " << p_class << " *-* " << m_class;
		return ss.str();
	case Overload::opMul:
		ss << r_class << " = " << m_class << " *\\** " << p_class;
		return ss.str();
	case Overload::opMul_r:
		ss << r_class << " = " << p_class << " *\\** " << m_class;
		return ss.str();
	case Overload::opDiv:
		ss << r_class << " = " << m_class << " */* " << p_class;
		return ss.str();
	case Overload::opDiv_r:
		ss << r_class << " = " << p_class << " */* " << m_class;
		return ss.str();
	case Overload::opMod:
		ss << r_class << " = " << m_class << " *%* " << p_class;
		return ss.str();
	case Overload::opMod_r:
		ss << r_class << " = " << p_class << " *%* " << m_class;
		return ss.str();
	case Overload::opPow:
		ss << r_class << " = " << m_class << " *\\*\\** " << p_class;
		return ss.str();
	case Overload::opPow_r:
		ss << r_class << " = " << p_class << " *\\*\\** " << m_class;
		return ss.str();
	case Overload::opAnd:
		ss << r_class << " = " << m_class << " *&* " << p_class;
		return ss.str();
	case Overload::opAnd_r:
		ss << r_class << " = " << p_class << " *&* " << m_class;
		return ss.str();
	case Overload::opOr:
		ss << r_class << " = " << m_class << " *|* " << p_class;
		return ss.str();
	case Overload::opOr_r:
		ss << r_class << " = " << p_class << " *|* " << m_class;
		return ss.str();
	case Overload::opXor:
		ss << r_class << " = " << m_class << " *^* " << p_class;
		return ss.str();
	case Overload::opXor_r:
		ss << r_class << " = " << p_class << " *^* " << m_class;
		return ss.str();
	case Overload::opShl:
		ss << r_class << " = " << m_class << " *<<* " << p_class;
		return ss.str();
	case Overload::opShl_r:
		ss << r_class << " = " << p_class << " *<<* " << m_class;
		return ss.str();
	case Overload::opShr:
		ss << r_class << " = " << m_class << " *>>* " << p_class;
		return ss.str();
	case Overload::opShr_r:
		ss << r_class << " = " << p_class << " *>>* " << m_class;
		return ss.str();
	case Overload::opUShr:
		ss << r_class << " = " << m_class << " *>>>* " << p_class;
		return ss.str();
	case Overload::opUShr_r:
		ss << r_class << " = " << p_class << " *>>>* " << m_class;
		return ss.str();

	case Overload::opIndex:
	case Overload::opCall:
		ss << r_class << " = " << m_class << (op == Overload::opIndex? "*[" : "*(");

		for(asUINT i = 0; i < n; ++i)
		{
			const char * name;
			const char * defaultArg;
			func->GetParam(i, &type, &flags, &name, &defaultArg);

			if(type == asTYPEID_INT32 && flags & asTM_INOUTREF)
				continue;

			ss << GetTypeName(doc, engine, type, flags);

			if(name       != nullptr && *name      ) ss << " " << name;
			if(defaultArg != nullptr && *defaultArg) ss << " = " << defaultArg;

			if(i+1 < n)
				ss << ", ";
		}

		ss <<  (op == Overload::opIndex? "]*" : ")*");
		return ss.str();
	default:
	return "";
	}
}

int GetVaridicLength(asIScriptFunction * func, int i, asUINT n)
{
	int type;
	asDWORD flags;
	const char * name;
	const char * defaultArg;
	func->GetParam(i, &type, &flags, &name, &defaultArg);

	if(defaultArg != nullptr && type == -1 && flags & asTM_INOUTREF && strcmp(defaultArg, "null") == 0)
	{
		int varidic_length = 1;

		for(asUINT j = i+1; j < n; ++j, ++varidic_length)
		{
			const char * name;
			const char * defaultArg;
			func->GetParam(i, &type, &flags, &name, &defaultArg);

			if(!(defaultArg != nullptr && type == -1 && flags & asTM_INOUTREF && strcmp(defaultArg, "null") == 0))
			{
				break;
			}
		}

		return varidic_length;
	}

	return 0;
}

static std::string GetFunctionName(asDocumenter * doc, asIScriptEngine * engine, asIScriptFunction * func, bool is_global, const char * class_name = nullptr)
{
	auto override = doc->GetFunctionOverride(func->GetId());

	if(override && override[0]) return override;

	auto op = IsOperatorOverload(func->GetName());

	if(!class_name && !is_global && func->GetObjectType() && op != Overload::None)
	{
		std::string r = GetOperatorOverload(doc, engine, func, op);
		if(r.size()) return r;
	}

	std::stringstream ss;

	std::string func_name;

	//fputc('[', fp);

	if(class_name)
		func_name = class_name;
	else
		func_name = GetFunctionName(func, is_global);

	asDWORD flags{};
	int type = func->GetReturnTypeId(&flags);

	if(class_name == nullptr)
	{
		ss << GetTypeName(doc, engine, type, flags) << " ";
	}

	ss << func_name << "(";

	const asUINT n = func->GetParamCount();

	for(asUINT i = 0; i < n; ++i)
	{
		const char * name;
		const char * defaultArg;
		func->GetParam(i, &type, &flags, &name, &defaultArg);

		int len = GetVaridicLength(func, i, n);

		if(len > 8)
		{
			ss << "...";
			i += n;
		}
		else
		{
			ss << GetTypeName(doc, engine, type, flags);

			if(name       != nullptr && *name      ) ss << " " << name;
			if(defaultArg != nullptr && *defaultArg) ss << " = " << defaultArg;
		}

		if(i+1 < n)
			ss << ", ";
	}

	ss << ')';

	if(func->IsReadOnly())
	{
		ss << " const";
	}

	return ss.str();
}

static std::string GetPropertyName(asDocumenter * doc, asIScriptEngine * engine, const char * name, const char * nameSpace, int typeId, bool isConst, int index_type = 0)
{
	(void)index_type;

	std::string prop_name = name;

	if(nameSpace)
	{
		prop_name = std::string(nameSpace) + "::" + prop_name;
	}

	return GetTypeName(doc, engine, typeId, isConst? asTM_CONST : 0) + " " + prop_name;
}

static bool HasSetterMethod(asITypeInfo * type_info, int type, const char * name)
{
	asUINT N = type_info->GetMethodCount();

	for(asUINT i = 0; i < N; ++i)
	{
		auto func = type_info->GetMethodByIndex(i);

		if(func->GetReturnTypeId() != asTYPEID_VOID)
			continue;

		const char * func_name = func->GetName();

		if(strncmp("set_", name, 4) != 0 || strcmp(name, func_name+4) != 0)
			continue;

		if(func->GetParamCount() != 1)
			continue;

		int id;
		func->GetParam(0, &id);

		if(id == type)
			return true;
	}

	return false;
}

template<typename T>
static bool PrintGlobalFunctions(std::ofstream & file,  asDocumenter * doc, asIScriptEngine * engine, asITypeInfo * info, const char * title, bool did_print, bool use_classname, T const& shouldPrint)
{
	asUINT N = engine->GetGlobalFunctionCount();

	const char * className = use_classname? info->GetName() : nullptr;

	for(asUINT i = 0; i < N; ++i)
	{
		asIScriptFunction * func = engine->GetGlobalFunctionByIndex(i);

		if(doc->SilenceFunction(func->GetId()))
			continue;

		if(!shouldPrint(func))
			continue;

		if(did_print == false)
		{
			file << title;
			did_print = true;
		}

		file << "**" << GetFunctionName(doc, engine, func, false, className)  << "**\n\n";
		file << doc->GetGlobalFunction(func->GetId()) << "\n\n";
	}

	return did_print;
}

static bool PrintFactories(std::ofstream & file, asDocumenter * doc, asIScriptEngine * engine, asITypeInfo * info)
{
	const std::string typeName = GetTypeName(info);
	const asUINT N = info->GetFactoryCount();
	bool did_print = false;

	if(N)
	{
		file << "### Factories\n";
		did_print = true;
	}

	for(asUINT i = 0; i < N; ++i)
	{
		auto func = info->GetFactoryByIndex(i);

		if(doc->SilenceFunction(func->GetId()))
			continue;

		file << "**" << GetFunctionName(doc, engine, func, false, typeName.c_str())  << "**\n\n";
		file << doc->GetObjectBehaviour(func->GetId()) << "\n\n";
	}

	PrintGlobalFunctions(file, doc, engine, info, "### Factories\n", did_print, false, [=](asIScriptFunction * func)
	{
		int type = func->GetReturnTypeId();

		return (type & (asTYPEID_HANDLETOCONST | asTYPEID_OBJHANDLE)) != false
			&& (type & asTYPEID_MASK_SEQNBR) == (info->GetTypeId() & asTYPEID_MASK_SEQNBR);
	});

	return did_print;
}

static bool PrintConstructors(std::ofstream & file, asDocumenter * doc, asIScriptEngine * engine, asITypeInfo * info)
{
	const std::string typeName = GetTypeName(info);
	const asUINT N = info->GetBehaviourCount();
	bool did_print = false;

	for(asUINT i = 0; i < N; ++i)
	{
		asEBehaviours bhvr;
		auto func = info->GetBehaviourByIndex(i, &bhvr);

		if(doc->SilenceFunction(func->GetId()))
			continue;

		if(bhvr == asBEHAVE_CONSTRUCT)
		{
			if(did_print == false)
			{
				file << "### Constructors\n";
				did_print = true;
			}

			file << "**" << GetFunctionName(doc, engine, func, false, typeName.c_str())  << "**\n\n";
			file << doc->GetObjectBehaviour(func->GetId()) << "\n\n";
		}
	}

	PrintGlobalFunctions(file, doc, engine, info, "### Constructors\n", did_print, false, [=](asIScriptFunction * func)
	{
		return (func->GetReturnTypeId() & asTYPEID_MASK_SEQNBR) == info->GetTypeId();
	});

	return did_print;
}

std::vector<kProperty> GetProperties(asITypeInfo * info)
{
	auto c_obj = static_cast<asCObjectType*>(info);

	const asUINT N = info->GetPropertyCount();
	std::vector<kProperty> properties;

	for(asUINT i = 0; i < N; ++i)
	{
		const char * name;
		int typeId;
		bool isConst = c_obj->properties[i]->type.IsReadOnly();
		bool out_private;
		bool out_protected;
		int  out_offset;

		info->GetProperty(i, &name, &typeId, &out_private, &out_protected, &out_offset);
		info->GetPropertyDeclaration(i);

		properties.push_back({name, typeId, 0, out_offset, isConst == false, false});
	}

	return properties;
}

static bool PrintMethods(std::vector<kProperty> & properties, std::ofstream & file, asDocumenter * doc, asIScriptEngine * engine, asITypeInfo * info)
{
	const asUINT N = info->GetMethodCount();
	bool did_print;

	std::vector<asIScriptFunction*> overloads;

	for(asUINT i = 0; i < N; ++i)
	{
		auto func = info->GetMethodByIndex(i);

		if(doc->SilenceFunction(func->GetId()))
			continue;

		if(IsOperatorOverload(func->GetName()) != Overload::None)
		{
			overloads.push_back(func);
			continue;
		}

		if(func->IsReadOnly() && strncmp("get_", func->GetName(), 4) == 0
		&& func->GetParamCount() <= 1)
		{
			int id = 0;
			if(func->GetParamCount()) func->GetParam(0, &id);
			properties.push_back({func->GetName()+4, func->GetReturnTypeId(), id, false, true});
			continue;
		}

		if(func->IsReadOnly() == false && strncmp("set_", func->GetName(), 4) == 0
		&& func->GetParamCount() <= 2)
		{
			int id = 0;
			if(func->GetParamCount() == 2) func->GetParam(0, &id);
			int type_id = 0;
			func->GetParam(func->GetParamCount()-1, &type_id);
			properties.push_back({func->GetName()+4, type_id, id, true, false});
			continue;
		}

		if(did_print == false)
		{
			did_print = true;
			file << "### Methods\n\n";
		}

		file << "**" << GetFunctionName(doc, engine, func, false) << "**\n\n";
		file << doc->GetObjectMethod(func->GetId()) << "\n\n";
	}

	if(overloads.size())
	{
		did_print = true;
		file << "### Operators\n\n";

		for(auto func : overloads)
		{
			if(doc->SilenceFunction(func->GetId()))
				continue;

			file << "**" << GetFunctionName(doc, engine, func, false) << "**\n\n";
			file << doc->GetObjectMethod(func->GetId()) << "\n\n";
		}
	}

	return did_print;
}

static bool PrintProperties(std::vector<kProperty> && properties, std::ofstream & file, asDocumenter * doc, asIScriptEngine * engine, asITypeInfo * info)
{
	if(properties.empty())
		return false;
/*
	std::sort(properties.begin(), properties.end(), [](kProperty const& A, kProperty const&B)
	{
		return strcmp(A.name, B.name) < 0;
	});*/

	file << "### Properties\n\n";



	for(uint32_t i = 0; i < properties.size(); ++i)
	{
		if(properties[i].name == nullptr)
			continue;

		for(uint32_t j = i+1; j < properties.size(); ++j)
		{
			 if(properties[j].name && strcmp(properties[i].name, properties[j].name) == 0)
			 {
				properties[j].name = nullptr;
				properties[i].setter = std::max(properties[i].setter, properties[j].setter);
				properties[i].getter = std::max(properties[i].getter, properties[j].getter);
				break;
			 }
		}

		file << "**" << GetPropertyName(doc, engine, properties[i].name, nullptr, properties[i].type, (properties[i].setter == 0), properties[i].index_type) << "**\n\n";

		const char * description_str = nullptr;

		if(properties[i].setter != 0)
			description_str = doc->GetObjectMethod(properties[i].setter);
		if(properties[i].getter != 0 && description_str == nullptr)
			description_str = doc->GetObjectMethod(properties[i].getter);
		if(description_str == nullptr)
			description_str = doc->GetObjectProperty(info->GetTypeId(),  properties[i].offset);

		file << description_str << "\n\n";
	}

	return true;
}

static bool PrintEnumeration(std::ofstream & file, asDocumenter * doc, asITypeInfo * info)
{
	const asUINT N = info->GetEnumValueCount();
	const int typeId = info->GetTypeId();

	if(!N) return false;

	file << "| Name | Value | Description |\n| --- | --- | --- |\n";

	for(asUINT i = 0; i < N; ++i)
	{
		int outValue;
		const char * name = info->GetEnumValueByIndex(i, &outValue);

		file << "| " << name << " | " << outValue << " | " << doc->GetEnumValue(typeId, outValue) << " |\n";
	}

	file << "\n";

	return true;
}

static void PrintTypeInfo(std::ofstream & file, asDocumenter * doc, asIScriptEngine * engine, asITypeInfo * info)
{
	auto description_str = doc->GetObjectType(info->GetTypeId());

	if(*description_str)
	{
		file << "### Overview\n";
		file << description_str << "\n";
	}

	auto globals = doc->GetGlobals(info->GetTypeId());

	if(globals.size())
	{
		PrintGlobalFunctions(file, doc, engine, info, "### Global Functions\n", false, false, [=](asIScriptFunction * func)
		{
			int type = func->GetReturnTypeId();

			bool is_factory = (type & (asTYPEID_HANDLETOCONST | asTYPEID_OBJHANDLE)) != false
				&& (type & asTYPEID_MASK_SEQNBR) == (info->GetTypeId() & asTYPEID_MASK_SEQNBR);

			if(is_factory)
				return false;

			int funcId = func->GetId();

			for(auto & p : globals)
			{
				if(p.first <= funcId && funcId <= p.second)
					return true;
			}

			return false;
		});
	}

	PrintFactories(file, doc, engine, info);
	PrintConstructors(file, doc, engine, info);

	std::vector<kProperty> properties = GetProperties(info);
	PrintMethods(properties, file, doc, engine, info);

	PrintProperties(std::move(properties), file, doc, engine, info);
	PrintEnumeration(file, doc, info);

	auto subtypes = doc->GetSubTypes(info->GetTypeId());

	for(auto type : subtypes)
	{
		auto info = engine->GetTypeInfoById(type);

		if(info)
		{
			file << "# " <<  info->GetName() << "\n";
			PrintTypeInfo(file, doc, engine, info);
		}
	}
}


static void PrintTypeInfo(std::ofstream & _sidebar, const char * filepath,  asDocumenter * doc, asIScriptEngine * engine, asITypeInfo * info)
{
	if(doc->GetParentType(info->GetTypeId()))
		return;

	std::string section_name = GetTypeName(info);
	std::string file_name    = GetTypeName(info, true);

	std::ofstream file(filepath + file_name + ".md");

	if (!file.is_open())
	{
		throw std::system_error(errno,  std::system_category(), filepath + file_name + ".md");
	}

	_sidebar << "[" << section_name << "](" << file_name << ")\n\n\n";

	file.exceptions ( std::ifstream::failbit | std::ifstream::badbit );

	PrintTypeInfo(file, doc, engine, info);

	file.close();
}

void OpenFile(std::ofstream & r, std::ofstream & _sidebar, const char * filepath, const char * nameSpace)
{
	if(r.is_open())	return;

	std::string file_name    = EscapeUnderscores(nameSpace);

	std::ofstream file(filepath + file_name + ".md");

	if (!file.is_open())
	{
		throw std::system_error(errno,  std::system_category(), filepath + file_name + ".md");
	}

	_sidebar << "### [" << nameSpace << "](" << file_name << ")\n\n\n";

	file.exceptions ( std::ifstream::failbit | std::ifstream::badbit );

	r = std::move(file);
}

static bool PrintNamespace(std::ofstream & _sidebar, const char * filepath, asDocumenter * doc,  asIScriptEngine * engine, const char * NameSpace)
{
	int length = strlen(NameSpace);

	std::ofstream file;
	asUINT N;

	if((N =  engine->GetObjectTypeCount()))
	{
		std::vector<std::pair<const char *, asITypeInfo*>> obj_types;

		for(uint32_t i = 0; i < N; ++i)
		{
			auto type_info = engine->GetObjectTypeByIndex(i);
			auto module = doc->GetModule(type_info->GetTypeId());

			if(module && *module == 0 && asDocumenter::insensitiveStrncmpAscii(NameSpace, type_info->GetNamespace(), length) == 0)
			{
				OpenFile(file, _sidebar, filepath, NameSpace);

				obj_types.emplace_back(type_info->GetName(), type_info);
			}
		}

		if(obj_types.size())
		{
			std::sort(obj_types.begin(), obj_types.end(), [](std::pair<const char*, asITypeInfo*> const& A, std::pair<const char*, asITypeInfo*> const& B)
			{
				return strcmp(A.first, B.first) < 0;
			});


			for(auto p : obj_types)
			{
				file << "## " <<  p.second->GetName() << "\n";
				PrintTypeInfo(file, doc, engine, p.second);
			}
		}
	}



	if((N = engine->GetGlobalFunctionCount()))
	{
		bool printed = false;

		for(asUINT i = 0; i < N; ++i)
		{
			auto func = engine->GetGlobalFunctionByIndex(i);

			if(asDocumenter::insensitiveStrncmpAscii(NameSpace, func->GetNamespace(), length) == 0
			&& !doc->IsGlobalAssociated(func->GetId()))
			{
				if(!printed)
				{
					OpenFile(file, _sidebar, filepath, NameSpace);

					printed = true;
					file << "## Functions\n\n";
				}

				if(doc->SilenceFunction(func->GetId()))
					continue;

				file << "**" << GetFunctionName(doc, engine, engine->GetGlobalFunctionByIndex(i), true) << "**\n\n";
				file <<  doc->GetGlobalFunction(func->GetId()) << "\n\n";
			}
		}
	}

	if((N = engine->GetGlobalPropertyCount()))
	{
		bool printed = false;

		for(asUINT i = 0; i < N; ++i)
		{
			const char * name;
			const char * nameSpace;
			int typeId;
			bool isConst;
			void * out_pointer;

			engine->GetGlobalPropertyByIndex(i, &name, &nameSpace, &typeId, &isConst, nullptr, &out_pointer);

			if(asDocumenter::insensitiveStrncmpAscii(NameSpace, nameSpace, length) != 0)
				continue;

			if(!printed)
			{
				OpenFile(file, _sidebar, filepath, NameSpace);

				printed = true;
				file << "## Properties\n\n";
			}

			auto prop_desc = doc->GetGlobalProperty((intptr_t)out_pointer);

			if(prop_desc < (void*)0x100)
			{
				prop_desc = doc->GetGlobalProperty((intptr_t)out_pointer);
			}

			file << "**" << GetPropertyName(doc, engine, name, nameSpace, typeId, isConst) << "**\n\n";
			file <<  prop_desc << "\n\n";
		}
	}

//print enums
	if((N = engine->GetEnumCount()))
	{
		for(asUINT i = 0; i < N; ++i)
		{
			auto typeInfo = engine->GetEnumByIndex(i);

			if(!doc->GetParentType(typeInfo->GetTypeId())
			&& asDocumenter::insensitiveStrncmpAscii(NameSpace, typeInfo->GetNamespace(), length) == 0)
			{
				OpenFile(file, _sidebar, filepath, NameSpace);

				file << "# " <<  typeInfo->GetName() << "\n";
				PrintTypeInfo(file, doc, engine, typeInfo);
			}
		}
	}

	return file.is_open();
}

static void PrintGlobals(std::ofstream & _sidebar, const char * filepath, asDocumenter * doc,  asIScriptEngine * engine)
{
	std::ofstream file(std::string(filepath) + "Globals.md");

	if (!file.is_open())
	{
		throw std::system_error(errno,  std::system_category(), std::string(filepath) + "Globals.md");
	}

	file.exceptions ( std::ifstream::failbit | std::ifstream::badbit );

	asUINT N;

	_sidebar << "[Globals](globals)\n\n";

	N = engine->GetGlobalPropertyCount();


	if(N)
	{
		bool printed = false;


		for(asUINT i = 0; i < N; ++i)
		{
			const char * name;
			const char * nameSpace;
			int typeId;
			bool isConst;
			void * out_pointer;

			engine->GetGlobalPropertyByIndex(i, &name, &nameSpace, &typeId, &isConst, nullptr, &out_pointer);

			if(doc->IsSpecialNamespace(nameSpace))
				continue;

			if(!printed)
			{
				printed = true;
				file << "### Properties\n\n";
			}

			file << "**" << GetPropertyName(doc, engine, name, nameSpace, typeId, isConst) << "**\n\n";
			file <<  doc->GetGlobalProperty((intptr_t)out_pointer) << "\n\n";
		}
	}

	N = engine->GetGlobalFunctionCount();

	if(N)
	{
		bool printed = false;

		for(asUINT i = 0; i < N; ++i)
		{
			auto func = engine->GetGlobalFunctionByIndex(i);

			if(!doc->IsGlobalAssociated(func->GetId())
			&& !doc->IsSpecialNamespace(func->GetNamespace()))
			{
				if(!printed)
				{
					printed = true;
					file << "### Functions\n\n";
				}

				if(doc->SilenceFunction(func->GetId()))
					continue;

				file << "**" << GetFunctionName(doc, engine, engine->GetGlobalFunctionByIndex(i), true) << "**\n\n";
				file <<  doc->GetGlobalFunction(func->GetId()) << "\n\n";
			}
		}
	}


//print enums
	N = engine->GetEnumCount();

	if(N)
	{
		for(asUINT i = 0; i < N; ++i)
		{
			auto typeInfo = engine->GetEnumByIndex(i);

			if(!doc->GetParentType(typeInfo->GetTypeId())
			&& doc->IsSpecialNamespace(typeInfo->GetNamespace()))
			{
				PrintTypeInfo(file, filepath, doc, engine, typeInfo);
			}
		}
	}

}

void WriteModule(std::vector<std::pair<const char *, asITypeInfo*>> && obj_types, std::ofstream & file, const char * filepath, asDocumenter * doc, asIScriptEngine * engine)
{
	std::sort(obj_types.begin(), obj_types.end(), [](std::pair<const char*, asITypeInfo*> const& A, std::pair<const char*, asITypeInfo*> const& B)
	{
		return strcmp(A.first, B.first) < 0;
	});


	for(auto p : obj_types)
	{
		PrintTypeInfo(file, filepath, doc, engine, p.second);
	}
}


void PrintAllRegistered(const char * filepath, asDocumenter * doc, asIScriptEngine * engine)
{
	std::ofstream file(std::string(filepath) + "_sidebar.md");

	if (!file.is_open())
	{
		throw std::system_error(errno,  std::system_category(), std::string(filepath) + "_sidebar.md");
	}

	file.exceptions ( std::ifstream::failbit | std::ifstream::badbit );

	auto modules = doc->GetModules();

	for(auto c : modules)
	{
		auto types = doc->GetModuleContents(c);

		std::vector<std::pair<const char *, asITypeInfo*>> obj_types;

		for(auto type : types)
		{
			auto type_info = engine->GetTypeInfoById(type);
			if(type_info)
				obj_types.emplace_back(type_info->GetName(), type_info);
		}

		if(!PrintNamespace(file, filepath, doc, engine, c))
		{
			file << "### " << c << "\n";
		}

		WriteModule(std::move(obj_types), file, filepath, doc, engine);
	}

	std::vector<std::pair<const char *, asITypeInfo*>> obj_types;

	asUINT N = engine->GetObjectTypeCount();
	for(asUINT i = 0; i < N; ++i)
	{
		auto type_info = engine->GetObjectTypeByIndex(i);
		auto module = doc->GetModule(type_info->GetTypeId());

		if(module && *module == 0 && !doc->IsSpecialNamespace(type_info->GetNamespace()))
		{
			obj_types.emplace_back(type_info->GetName(), type_info);
		}
	}

	file << "### No module.\n";

	WriteModule(std::move(obj_types), file, filepath, doc, engine);

//	file << "# [Globals](Globals)\n";

	PrintGlobals(file, filepath, doc, engine);

	file.close();
};



#endif

