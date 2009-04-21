#ifndef incl_PythonScriptObject_Script_h
#define incl_PythonScriptObject_Script_h



namespace PythonScript
{

	class PythonScriptObject : public Foundation::ScriptObject
	{
	public:
		PythonScriptObject(void);
		virtual ~PythonScriptObject(void);
	  
		
		virtual ScriptObject* CallMethod(std::string& methodname, std::string& syntax, char* argv[]);
		virtual ScriptObject* CallMethod(std::string& methodname, const std::string& syntax, const ScriptObject* args);
		virtual ScriptObject* GetObject(const std::string& objectname);

		virtual char* ConvertToChar();

		PyObject* pythonRef; // reference to class or python script
		PyObject* pythonObj; // object instance
	
	private:
		static std::map<std::string, void(*)(char*)> methods;
		//static std::map<std::string, void*> methods;
		//void* methodptr;
		

	};
}

#endif