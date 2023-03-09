
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

static std::string GetFunctionName(asIScriptFunction * info, bool isGlobal)
{
	std::string name = info->GetName();

	if(isGlobal)
	{
		auto nameSpace = info->GetNamespace();
		if(nameSpace && *nameSpace != '\0')
			name = std::string(nameSpace) + "::" + name;
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

static std::string GetTypeName(asDocumenter * doc, asIScriptEngine * engine, int type, asDWORD flags)
{
	std::stringstream ss;

	if(flags & asTM_CONST)
		ss << "const ";

	auto name = GetTypeName(engine, type);
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

		ss << name;

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

void PrintDescription(std::ostream & file, std::string const& str)
{
	if(str.empty())
		return;

	for(size_t i = 0, next = str.find_first_of('\n', i); i < str.size(); next = str.find_first_of('\n', i))
	{
		std::string substring;
		if(next == std::string::npos)
			substring = str.substr(i);
		else
			substring = str.substr(i, next - i);

		i = next;
		while(str[i] == '\n')
			++i;

		if(substring.size() < 2)
			continue;

		file << "/// " << substring << "\n";
	}
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

		auto desc =  doc->GetGlobalFunction(func->GetId());

		PrintDescription(file, desc);
		file << '\t' << GetFunctionName(doc, engine, func, false, className) << ";\n";
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
		file << "\n/// Factories\n";
		did_print = true;
	}

	for(asUINT i = 0; i < N; ++i)
	{
		auto func = info->GetFactoryByIndex(i);

		if(doc->SilenceFunction(func->GetId()))
			continue;

		auto desc =  doc->GetObjectBehaviour(func->GetId());

		PrintDescription(file, desc);
		file << '\t' << GetFunctionName(doc, engine, func, false, typeName.c_str()) << ";\n";
	}

	PrintGlobalFunctions(file, doc, engine, info, "/// Factories\n", did_print, false, [=](asIScriptFunction * func)
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
				file << "\n/// Constructors\n";
				did_print = true;
			}

			auto desc =  doc->GetObjectBehaviour(func->GetId());

			PrintDescription(file, desc);
			file << '\t' << GetFunctionName(doc, engine, func, false, typeName.c_str()) << ";\n";
		}
	}

	PrintGlobalFunctions(file, doc, engine, info, "\n/// Constructors\n", did_print, false, [=](asIScriptFunction * func)
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

	for(asUINT i = 0; i < N; ++i)
	{
		auto func = info->GetMethodByIndex(i);

		if(doc->SilenceFunction(func->GetId()))
			continue;

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
			file << "\n/// Methods\n";
		}

		auto desc =  doc->GetObjectMethod(func->GetId());

		PrintDescription(file, desc);
		file << '\t' << GetFunctionName(doc, engine, func, false) << ";\n";
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

	file << '\n';

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

		const char * description_str = nullptr;

		if(properties[i].setter != 0)
			description_str = doc->GetObjectMethod(properties[i].setter);
		if(properties[i].getter != 0 && description_str == nullptr)
			description_str = doc->GetObjectMethod(properties[i].getter);
		if(description_str == nullptr)
			description_str = doc->GetObjectProperty(info->GetTypeId(),  properties[i].offset);

		PrintDescription(file, description_str);
		file << "\t" << GetPropertyName(doc, engine, properties[i].name, nullptr, properties[i].type, (properties[i].setter == 0), properties[i].index_type) << ";\n";
	}

	return true;
}

static bool PrintEnumeration(std::ofstream & file, asDocumenter * doc, asITypeInfo * info)
{
	const asUINT N = info->GetEnumValueCount();
	const int typeId = info->GetTypeId();

	if(!N) return false;

	file << "\nenum " << info->GetName() << " {\n";

	for(asUINT i = 0; i < N; ++i)
	{
		int outValue;
		const char * name = info->GetEnumValueByIndex(i, &outValue);
		auto descr = doc->GetEnumValue(typeId, outValue) ;

		PrintDescription(file, descr);
		file << '\t' << name << " = 0x" << std::hex << outValue << ",\n";
	}

	file << "}\n";

	return true;
}

static void PrintTypeInfo(std::ofstream & file, asDocumenter * doc, asIScriptEngine * engine, asITypeInfo * info)
{
	auto description_str = doc->GetObjectType(info->GetTypeId());

	if(*description_str)
	{
		PrintDescription(file, description_str);
	}

	auto globals = doc->GetGlobals(info->GetTypeId());

	if(globals.size())
	{
		PrintGlobalFunctions(file, doc, engine, info, "\n/// Global Functions\n", false, false, [=](asIScriptFunction * func)
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

	if(PrintEnumeration(file, doc, info) == true)
		return;

	file << "class " << info->GetName() << " {\n";

	PrintFactories(file, doc, engine, info);
	PrintConstructors(file, doc, engine, info);

	std::vector<kProperty> properties = GetProperties(info);
	PrintMethods(properties, file, doc, engine, info);

	PrintProperties(std::move(properties), file, doc, engine, info);

	file << "}\n";

	auto subtypes = doc->GetSubTypes(info->GetTypeId());

	for(auto type : subtypes)
	{
		auto info = engine->GetTypeInfoById(type);

		if(info)
		{
			PrintTypeInfo(file, doc, engine, info);
		}
	}
}

static bool PrintNamespace(std::ofstream & file, asDocumenter * doc,  asIScriptEngine * engine, const char * NameSpace)
{
	int length = strlen(NameSpace);

	asUINT N;

	if(length)
		file << "\nnamespace " << NameSpace << " {\n";

	if((N =  engine->GetObjectTypeCount()))
	{
		std::vector<std::pair<const char *, asITypeInfo*>> obj_types;

		for(uint32_t i = 0; i < N; ++i)
		{
			auto type_info = engine->GetObjectTypeByIndex(i);
			auto _module = doc->GetModule(type_info->GetTypeId());

			if(_module && *_module == 0 && asDocumenter::insensitiveStrncmpAscii(NameSpace, type_info->GetNamespace(), length) == 0)
			{
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
			//	file << "\n/// " <<  p.second->GetName() << "\n";
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
					printed = true;
					file << "\n/// Functions\n";
				}

				if(doc->SilenceFunction(func->GetId()))
					continue;

				auto desc = doc->GetGlobalFunction(func->GetId());

				PrintDescription(file, desc);
				file << GetFunctionName(doc, engine, engine->GetGlobalFunctionByIndex(i), true) << ";\n";
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
				printed = true;
				file << "\n/// Properties\n";
			}

			auto prop_desc = doc->GetGlobalProperty((intptr_t)out_pointer);

			if(prop_desc < (void*)0x100)
			{
				prop_desc = doc->GetGlobalProperty((intptr_t)out_pointer);
			}

			PrintDescription(file, prop_desc);

			file << GetPropertyName(doc, engine, name, nameSpace, typeId, isConst) << ";\n";
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
				PrintTypeInfo(file, doc, engine, typeInfo);
			}
		}
	}

	if(length)
		file << "}\n";

	return file.is_open();
}

static void PrintGlobals(std::ofstream & file, asDocumenter * doc,  asIScriptEngine * engine)
{
	auto N = engine->GetGlobalPropertyCount();

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
				file << "\n/// Properties\n";
			}

			PrintDescription(file, doc->GetGlobalProperty((intptr_t)out_pointer));
			file << "\t" << GetPropertyName(doc, engine, name, nameSpace, typeId, isConst) << ";\n";
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
					file << "\n/// Functions\n";
				}

				if(doc->SilenceFunction(func->GetId()))
					continue;

				PrintDescription(file, doc->GetGlobalFunction(func->GetId()));
				file << GetFunctionName(doc, engine, engine->GetGlobalFunctionByIndex(i), true) << ";\n";
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
				PrintTypeInfo(file, doc, engine, typeInfo);
			}
		}
	}

}

void WriteModule(std::vector<std::pair<const char *, asITypeInfo*>> && obj_types, std::ofstream & file, asDocumenter * doc, asIScriptEngine * engine)
{
	std::sort(obj_types.begin(), obj_types.end(), [](std::pair<const char*, asITypeInfo*> const& A, std::pair<const char*, asITypeInfo*> const& B)
	{
		return strcmp(A.first, B.first) < 0;
	});


	for(auto p : obj_types)
	{
		PrintTypeInfo(file, doc, engine, p.second);
	}
}


void PrintAllRegistered(const char * filepath, asDocumenter * doc, asIScriptEngine * engine)
{
	std::ofstream file(std::string(filepath) + "symbol_table.as");

	if (!file.is_open())
	{
		throw std::system_error(errno,  std::system_category(), std::string(filepath) + "_sidebar.md");
	}

	file.exceptions ( std::ifstream::failbit | std::ifstream::badbit );

	file.imbue(std::locale("C"));

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

		if(!PrintNamespace(file, doc, engine, c))
		{
			file << "/// " << c << "\n";
		}

		WriteModule(std::move(obj_types), file, doc, engine);
	}

	std::vector<std::pair<const char *, asITypeInfo*>> obj_types;

	asUINT N = engine->GetObjectTypeCount();
	for(asUINT i = 0; i < N; ++i)
	{
		auto type_info = engine->GetObjectTypeByIndex(i);
		auto _module = doc->GetModule(type_info->GetTypeId());

		if(_module && *_module == 0 && !doc->IsSpecialNamespace(type_info->GetNamespace()))
		{
			obj_types.emplace_back(type_info->GetName(), type_info);
		}
	}

	file << "/// No _module.\n";

	WriteModule(std::move(obj_types), file, doc, engine);

//	file << "# [Globals](Globals)\n";

	PrintGlobals(file, doc, engine);

	file.close();
};



#endif

