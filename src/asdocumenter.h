#ifndef ASDOCUMENTER_H
#define ASDOCUMENTER_H

//disable to not include strings in final executable.
#define AS_PRINTABLE 0

class asIScriptEngine;

#if AS_PRINTABLE > 0
#include <vector>
#include <map>
#include <cstdint>


class asDocumenter
{
public:
typedef std::map<int, const char *> mapping;
	static int insensitiveStrncmpAscii(const char * a, const char * b, int length);

	asDocumenter() = default;
	~asDocumenter() = default;

	void clear();

	inline const char * GetInterface (int typeId) const { return ReadTypeId(typeId); }
	inline const char * GetObjectType(int typeId) const { return ReadTypeId(typeId); }
	inline const char * GetTypedef   (int typeId) const { return ReadTypeId(typeId); }
	inline const char * GetFuncDef   (int typeId) const { return ReadTypeId(typeId); }
	inline const char * GetEnum      (int typeId) const { return ReadTypeId(typeId); }

	inline const char * GetObjectMethod   (int funcId) const { return ReadFunction(funcId); }
	inline const char * GetGlobalFunction (int funcId) const { return ReadFunction(funcId); }
	inline const char * GetObjectBehaviour(int funcId) const { return ReadFunction(funcId); }

	inline const char * GetEnumValue     (int typeId, int value)  const { return ReadOffset(typeId, value ); }
	inline const char * GetObjectProperty(int typeId, int offset) const { return ReadOffset(typeId, offset); }
	inline const char * GetGlobalProperty(int offset)                  const { return ReadOffset(0,      offset); }

	inline const char * GetFunctionOverride(int offset)                const { return ReadOverrideFunction(offset); }

	inline void RegisterInheritance (int typeId, int subTypeId) { m_inheritance.push_back({typeId, subTypeId}); }
	inline void RegisterInterface(int typeId, const char * description) { WriteTypeId(typeId, description); }

	inline void RegisterObjectType(int typeId, const char * description) { WriteTypeId(typeId, description); }
	inline void RegisterTypedef   (int typeId, const char * description) { WriteTypeId(typeId, description); }
	inline void RegisterFuncDef   (int typeId, const char * description) { WriteTypeId(typeId, description); }
	inline void RegisterEnum      (int typeId, const char * description) { WriteTypeId(typeId, description); }

	inline void RegisterObjectMethod   (int funcId, const char * description) { WriteFunction(funcId, description); }
	inline void RegisterGlobalFunction (int funcId, const char * description) { WriteFunction(funcId, description); }
	inline void RegisterObjectBehaviour(int funcId, const char * description) { WriteFunction(funcId, description); }

	inline void OverrideObjectMethod   (int funcId, const char * description) { WriteOverrideFunction(funcId, description); }
	inline void OverrideGlobalFunction (int funcId, const char * description) { WriteOverrideFunction(funcId, description); }

	inline void RegisterEnumValue     (int typeId, int value,       const char * description) { WriteOffset(typeId, value , description); }
	inline void RegisterObjectProperty(int typeId, int offset, const char * description) { WriteOffset(typeId, offset, description); }
	inline void RegisterGlobalProperty(int offset,                  const char * description) { WriteOffset(0,      offset, description); }

	inline void AddToModule(int typeId, const char * _module) { m_modules.push_back({typeId, _module}); }
	inline void AssociateGlobalFunctions(int typeId, int first_funcId, int lastFuncId) { m_globals.push_back({typeId, first_funcId, lastFuncId}); }
	void RegisterSubtype(int typeId, int subtypeId);

	inline void AddPage(const char * title, const char * desc) { m_miscPages.push_back({title, desc}); }

	std::vector<const char *> GetModules();
	std::vector<int> GetModuleContents(const char *);

	std::vector<int> GetSubTypes(int typeId);
	const char *		  GetModule(int typeId);
	std::vector<std::pair<int, int>> GetGlobals(int);
	bool				  IsGlobalAssociated(int);
	int64_t				  GetParentType(int);

	void AddSpecialNamespace(const char * it) { m_modules.push_back({0, it}); }
	bool IsSpecialNamespace(const char * c) const;

	bool SilenceFunction(int funcId) const;

private:
	const char * ReadTypeId(int typeId) const;
	void         WriteTypeId(int typeId, const char * description);

	const char * ReadFunction(int funcId) const;
	void         WriteFunction(int funcId, const char * description);

	const char * ReadOverrideFunction(int funcId) const;
	void         WriteOverrideFunction(int funcId, const char * description);

	const char * ReadOffset(int typeId, int offset) const;
	void         WriteOffset(int typeId, int offset, const char * description);

	std::vector<const char *>   m_funcDscr;
	std::vector<const char *>   m_funcOverride;
	mapping                     m_typeDscr;
	mapping                     m_offsetDscr;
	std::map<int, mapping> m_propertyDscr;

	std::vector<std::pair<int, const char *>>         m_modules;
	std::vector<std::pair<int, int>>             m_subTypes;
	std::vector<std::pair<int, int>>             m_inheritance;
	std::vector<std::tuple<int, int, int> > m_globals;

	std::vector<std::pair<const char *, const char *>>		m_miscPages;
//	mutable std::map<int, mapping>::iterator last_property{m_propertyDscr.end()};
};

#define docInhertiance(x, y)  doc->RegisterInheritance	(x, y )
#define docInterface(x, y)    doc->RegisterInterface	(x, y )
#define docObjectType(x, y)   doc->RegisterObjectType	(x, y )
#define docTypedef(x, y)      doc->RegisterTypedef		(x, y )
#define docFuncDef(x, y)      doc->RegisterFuncDef		(x, y )
#define docEnum(x, y)         doc->RegisterEnum			(x,	y )

#define docMethod(x, y)       doc->RegisterObjectMethod		(x, y )
#define docFunction(x, y)     doc->RegisterGlobalFunction	(x, y )
#define docBehavior(x, y)     doc->RegisterObjectBehaviour	(x, y )

#define docEnumValue(x, y, z) doc->RegisterEnumValue	 (x, y, z )
#define docProperty(x, y, z)  doc->RegisterObjectProperty(x, y, z )
#define docGlobal(x, y)       doc->RegisterGlobalProperty(x, y )

#define docAddToModule(x, y)	doc->AddToModule(x, y)
#define docRegisterSubtype(x, y)	doc->RegisterSubtype(x, y)
#define docAddGlobals(x, y, z)  doc->AssociateGlobalFunctions(x, y, z)

#define docAddNamespace(x)	doc->AddSpecialNamespace(x)

#define docOverrideFunc(x, y) doc->OverrideGlobalFunction(x, y)
#define docAddPage(x, y) doc->AddPage(x, y)

#else

class asDocumenter
{
};

#define docInheritance(x, y)  (void)doc; (void)x
#define docInterface(x, y)    (void)doc; (void)x
#define docObjectType(x, y)   (void)doc; (void)x
#define docTypedef(x, y)      (void)doc; (void)x
#define docFuncDef(x, y)      (void)doc; (void)x
#define docEnum(x, y)         (void)doc; (void)x

#define docMethod(x, y)       (void)doc; (void)x
#define docFunction(x, y)     (void)doc; (void)x
#define docBehavior(x, y)     (void)doc; (void)x

#define docEnumValue(x, y, z) (void)doc; (void)x
#define docProperty(x, y, z)  (void)doc; (void)x
#define docGlobal(x, y)       (void)doc; (void)x

#define docAddToModule(x, y)		(void)doc; (void)x
#define docRegisterInheritance(x, y)	(void)doc; (void)x; (void)y;
#define docRegisterSubtype(x, y)	(void)doc; (void)x
#define docAddGlobals(x, y, z)		(void)doc; (void)x; (void)y
#define docAddNamespace(x)		(void)doc; (void)x

#define docOverrideFunc(x, y) (void)doc; (void)x
#define docAddPage(x, y) (void)doc;

#endif

void PrintAllRegistered(const char *, asDocumenter *, asIScriptEngine *);

#endif // ASDOCUMENTER_H

