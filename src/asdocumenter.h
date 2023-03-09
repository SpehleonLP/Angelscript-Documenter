#ifndef ASDOCUMENTER_H
#define ASDOCUMENTER_H

//disable to not include strings in final executable.
#define AS_PRINTABLE 0

class asIScriptEngine;

#if AS_PRINTABLE
#include <vector>
#include <map>


class asDocumenter
{
public:
typedef std::map<intptr_t, const char *> mapping;
	static int insensitiveStrncmpAscii(const char * a, const char * b, int length);

	asDocumenter() = default;
	~asDocumenter() = default;

	void clear();

	inline const char * GetInterface (intptr_t typeId) const { return ReadTypeId(typeId); }
	inline const char * GetObjectType(intptr_t typeId) const { return ReadTypeId(typeId); }
	inline const char * GetTypedef   (intptr_t typeId) const { return ReadTypeId(typeId); }
	inline const char * GetFuncDef   (intptr_t typeId) const { return ReadTypeId(typeId); }
	inline const char * GetEnum      (intptr_t typeId) const { return ReadTypeId(typeId); }

	inline const char * GetObjectMethod   (intptr_t funcId) const { return ReadFunction(funcId); }
	inline const char * GetGlobalFunction (intptr_t funcId) const { return ReadFunction(funcId); }
	inline const char * GetObjectBehaviour(intptr_t funcId) const { return ReadFunction(funcId); }

	inline const char * GetEnumValue     (intptr_t typeId, intptr_t value)  const { return ReadOffset(typeId, value ); }
	inline const char * GetObjectProperty(intptr_t typeId, intptr_t offset) const { return ReadOffset(typeId, offset); }
	inline const char * GetGlobalProperty(intptr_t offset)                  const { return ReadOffset(0,      offset); }

	inline const char * GetFunctionOverride(intptr_t offset)                const { return ReadOverrideFunction(offset); }

	inline void RegisterInterface (intptr_t typeId, const char * description) { WriteTypeId(typeId, description); }
	inline void RegisterObjectType(intptr_t typeId, const char * description) { WriteTypeId(typeId, description); }
	inline void RegisterTypedef   (intptr_t typeId, const char * description) { WriteTypeId(typeId, description); }
	inline void RegisterFuncDef   (intptr_t typeId, const char * description) { WriteTypeId(typeId, description); }
	inline void RegisterEnum      (intptr_t typeId, const char * description) { WriteTypeId(typeId, description); }

	inline void RegisterObjectMethod   (intptr_t funcId, const char * description) { WriteFunction(funcId, description); }
	inline void RegisterGlobalFunction (intptr_t funcId, const char * description) { WriteFunction(funcId, description); }
	inline void RegisterObjectBehaviour(intptr_t funcId, const char * description) { WriteFunction(funcId, description); }

	inline void OverrideObjectMethod   (intptr_t funcId, const char * description) { WriteOverrideFunction(funcId, description); }
	inline void OverrideGlobalFunction (intptr_t funcId, const char * description) { WriteOverrideFunction(funcId, description); }

	inline void RegisterEnumValue     (intptr_t typeId, int value,       const char * description) { WriteOffset(typeId, value , description); }
	inline void RegisterObjectProperty(intptr_t typeId, intptr_t offset, const char * description) { WriteOffset(typeId, offset, description); }
	inline void RegisterGlobalProperty(intptr_t offset,                  const char * description) { WriteOffset(0,      offset, description); }

	inline void AddToModule(intptr_t typeId, const char * _module) { m_modules.push_back({typeId, _module}); }
	inline void AssociateGlobalFunctions(intptr_t typeId, intptr_t first_funcId, intptr_t lastFuncId) { m_globals.push_back({typeId, first_funcId, lastFuncId}); }
	void RegisterSubtype(intptr_t typeId, intptr_t subtypeId);

	inline void AddPage(const char * title, const char * desc) { m_miscPages.push_back({title, desc}); }

	std::vector<const char *> GetModules();
	std::vector<intptr_t> GetModuleContents(const char *);

	std::vector<intptr_t> GetSubTypes(intptr_t typeId);
	const char *		  GetModule(intptr_t typeId);
	std::vector<std::pair<intptr_t, intptr_t>> GetGlobals(intptr_t);
	bool				  IsGlobalAssociated(intptr_t);
	int64_t				  GetParentType(intptr_t);

	void AddSpecialNamespace(const char * it) { m_modules.push_back({0, it}); }
	bool IsSpecialNamespace(const char * c) const;

	bool SilenceFunction(intptr_t funcId) const;

private:
	const char * ReadTypeId(intptr_t typeId) const;
	void         WriteTypeId(intptr_t typeId, const char * description);

	const char * ReadFunction(intptr_t funcId) const;
	void         WriteFunction(intptr_t funcId, const char * description);

	const char * ReadOverrideFunction(intptr_t funcId) const;
	void         WriteOverrideFunction(intptr_t funcId, const char * description);

	const char * ReadOffset(intptr_t typeId, intptr_t offset) const;
	void         WriteOffset(intptr_t typeId, intptr_t offset, const char * description);

	std::vector<const char *>   m_funcDscr;
	std::vector<const char *>   m_funcOverride;
	mapping                     m_typeDscr;	q
	mapping                     m_offsetDscr;
	std::map<intptr_t, mapping> m_propertyDscr;

	std::vector<std::pair<intptr_t, const char *>>         m_modules;
	std::vector<std::pair<intptr_t, intptr_t>>             m_subTypes;
	std::vector<std::tuple<intptr_t, intptr_t, intptr_t> > m_globals;

	std::vector<std::pair<const char *, const char *>>		m_miscPages;
//	mutable std::map<intptr_t, mapping>::iterator last_property{m_propertyDscr.end()};
};

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

void PrintAllRegistered(const char * filepath, asDocumenter * doc, asIScriptEngine * engine);

#else

class asDocumenter
{
};

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
#define docRegisterSubtype(x, y)	(void)doc; (void)x
#define docAddGlobals(x, y, z)		(void)doc; (void)x; (void)y
#define docAddNamespace(x)		(void)doc; (void)x

#define docOverrideFunc(x, y) (void)doc; (void)x
#define docAddPage(x, y) (void)doc;


inline void PrintAllRegistered(const char *, asDocumenter *, asIScriptEngine *) {}

#endif

#endif // ASDOCUMENTER_H

