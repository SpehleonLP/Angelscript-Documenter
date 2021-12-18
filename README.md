# Angelscript-Documenter

Functions for generating documentation for angelscript registered classes 

### Example Usage
	#include <angelscript.h>
	#include <asdocumenter.h>


	void asRegisterNativeClasses(asIScriptEngine *engine)
	{
		int r{};

		asDocumenter idoc;
	//the macros are looking for a pointer called "doc" so create an alias	
		auto doc = &idoc;
		
	//register  some stuff so we have a starting point	
		RegisterScriptDictionary(engine);
		RegisterStdString(engine);
		
		int dictionary_id = engine->GetTypeInfoByName("dictionary")->GetTypeId();
		int string_id = engine->GetTypeInfoByName("string")->GetTypeId();
		
	//make the dictionary value and dictionary print on the same page
		docRegisterSubtype(dictionary_id,  engine->GetTypeInfoByName("dictionaryValue")->GetTypeId());
		
	//add a category in the printed wiki
		docAddNamespace("Support");
	//put some stuff in it	
		docAddToNamespace(dictionary_id, "Support");
		docAddToNamespace(string_id, "Support");
		
	//document a global
		r = engine->RegisterGlobalFunction("string normalize(string)", asFUNCTION(asNormalize), asCALL_CDECL);
		docFunction(r, "Normalize unicode string, (compatibility decomposition followed by cannonical composition)");
				
	//print out the results	
		PrintAllRegistered("my_path_dir", doc, engine);
	}
