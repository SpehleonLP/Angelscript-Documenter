#include "asdocumenter.h"
#include <algorithm>
#include <cstring>
#include <cassert>

#if AS_PRINTABLE > 0

void asDocumenter::clear()
{
	std::vector<const char *>().swap(m_funcDscr);
	m_typeDscr.clear();
	m_offsetDscr.clear();
	m_propertyDscr.clear();
}

const char * asDocumenter::ReadTypeId(intptr_t typeId) const
{
	auto itr = m_typeDscr.find(typeId);

	if(itr == m_typeDscr.end())
		return "";

	return itr->second;
}
void         asDocumenter::WriteTypeId(intptr_t typeId, const char * description)
{
	m_typeDscr[typeId] = description;
}

const char * asDocumenter::ReadFunction(intptr_t funcId) const
{
	if((uint64_t)funcId < m_funcDscr.size())
		return m_funcDscr[funcId];

	return "";
}

void         asDocumenter::WriteFunction(intptr_t funcId, const char * description)
{
	m_funcDscr.resize(std::max<uintptr_t>(m_funcDscr.size(), funcId+1), "");
	m_funcDscr[funcId] = description;
}

const char * asDocumenter::ReadOverrideFunction(intptr_t funcId) const
{
	if((uint64_t)funcId < m_funcOverride.size())
		return m_funcOverride[funcId]?  m_funcOverride[funcId] : "";

	return "";
}

void         asDocumenter::WriteOverrideFunction(intptr_t funcId, const char * description)
{
	m_funcOverride.resize(std::max<uintptr_t>(m_funcOverride.size(), funcId+1), "");
	m_funcOverride[funcId] = description;
}

bool asDocumenter::SilenceFunction(intptr_t funcId) const
{
	if((uint64_t)funcId < m_funcOverride.size())
		return m_funcOverride[funcId]? false : true;

	return false;
}


const char * asDocumenter::ReadOffset(intptr_t typeId, intptr_t offset) const
{
	auto itr0 = const_cast<asDocumenter*>(this)->m_propertyDscr.find(typeId);

	if(itr0 != m_propertyDscr.end())
	{
		auto itr = itr0->second.find(offset);

		if(itr != itr0->second.end())
		{
			assert(itr->second != (void*)41);
			return itr->second;
		}
	}

	return "";
}

void         asDocumenter::WriteOffset(intptr_t typeId, intptr_t offset, const char * description)
{
	auto itr0 = const_cast<asDocumenter*>(this)->m_propertyDscr.find(typeId);

	if(itr0 == m_propertyDscr.end())
		itr0 = m_propertyDscr.emplace(std::make_pair(typeId, mapping())).first;

	itr0->second[offset] = description;
}

void asDocumenter::RegisterSubtype(intptr_t typeId, intptr_t subtypeId)
{
	if(GetParentType(typeId))
			throw -1;

	m_subTypes.push_back({typeId, subtypeId});
}

std::vector<const char *> asDocumenter::GetModules()
{
	std::vector<const char *> r;

	for(auto & p : m_modules)
	{
		if(p.second != nullptr)
		{
			for(auto c : r)
			{
				if(strcmp(c, p.second) == 0)
				{
					goto end;
				}
			}

			r.push_back(p.second);

		end:
			(void)0L;
		}
	}

	return r;
}

std::vector<intptr_t> asDocumenter::GetModuleContents(const char * m)
{
	std::vector<intptr_t> r;

	for(auto & p : m_modules)
	{
		if(p.first > 0 && strcmp(p.second, m) == 0)
			r.push_back(p.first);
	}

	return r;
}


std::vector<intptr_t> asDocumenter::GetSubTypes(intptr_t typeId)
{
	std::vector<intptr_t> r;

	for(auto & p : m_subTypes)
	{
		if(p.first == typeId)
			r.push_back(p.second);
	}

	return r;
}

const char *		  asDocumenter::GetModule(intptr_t typeId)
{
	for(auto & p : m_modules)
	{
		if(p.first == typeId)
			return p.second;
	}

	return "";
}

std::vector<std::pair<intptr_t, intptr_t>> asDocumenter::GetGlobals(intptr_t k)
{
	std::vector<std::pair<intptr_t, intptr_t>>  r;

	for(auto & p : m_globals)
	{
		if(std::get<0>(p) == k)
		{
			r.push_back({std::get<1>(p), std::get<2>(p)});
		}
	}

	return r;
}

bool				  asDocumenter::IsGlobalAssociated(intptr_t k)
{
	for(auto & p : m_globals)
	{
		if(std::get<1>(p) <= k && k <= std::get<2>(p))
		{
			return true;
		}
	}

	return false;
}

int64_t asDocumenter::GetParentType(intptr_t typeId)
{
	for(auto & p : m_subTypes)
	{
		if(typeId == p.second)
		{
			return p.first;
		}
	}

	return 0;
}

bool asDocumenter::IsSpecialNamespace(const char * c) const
{
	if(*c == 0)
		return false;

	auto length = strlen(c);

	for(auto p : m_modules)
	{
		if(insensitiveStrncmpAscii(p.second, c, length) == 0)
			return true;
	}

	return false;
}

int asDocumenter::insensitiveStrncmpAscii(const char * a, const char * b, int length)
{
	auto p = a;
	for(; *a != '\0' && *b != '\0' && tolower(*a) == tolower(*b) && a-p < length; ++a, ++b) {}
	return (a-p) < length? (tolower(*a) - tolower(*b)) : 0;
}

#endif


