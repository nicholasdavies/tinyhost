// config.h
// Module for providing strongly-typed program options with default values,
// which can be set through configuration files and/or the command line.

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <set>
using std::string;
using std::vector;

#define CONFIG_NO_LUAFUNC
#ifndef CONFIG_NO_LUAFUNC
#include "LuaFunc/luafunc.h"
using LuaFunc::LuaFunction;
#endif

class Parameters
{
public:
    Parameters();

    void Reset();

    void Read(std::istream& in);
    void Read(string filename);
    void Read(int argc, char* argv[]);
    void ReadLine(string line);

    void Write(std::ostream& out, std::string pre = "") const;

    void Set(std::string name, std::string value);

    int Sweep() const;
    string SweepName() const;
    int NSweeps() const;
    void NextSweep();
    void GoToSweep(int s);
    bool Good() const;

    // INCLUSION PASS 1: Declare all parameters as public members of this class.
#ifndef CONFIG_NO_LUAFUNC
    template<typename T> using Func = LuaFunction<T>;
#endif
    #define PARAMETER(Type, Name, Default...) Type Name;
    #define DEPRECATED(Name)
    #define REQUIRE(Condition)
    #include "config_def.h"
    #undef PARAMETER
    #undef DEPRECATED
    #undef REQUIRE

    typedef std::map<string, string> NameValueMap;

private:
    void InterpretLines(std::istream& in);
    void InterpretLine(string line);
    void SetAllToDefault();
    void AssignFromMap(NameValueMap& nvm, string stage);

    std::set<string> _param_names;

    vector<NameValueMap> _nvm_sweeps;
    vector<string> _nvm_sweep_names;

    vector<NameValueMap> _templates;
    vector<string> _template_names;
    vector<vector<string>> _template_params;

    NameValueMap _nvm_override;

    int _sweep;
    bool _good;
    bool _assignment_virgin;
    bool _template_mode;
};

#endif  // CONFIG_H