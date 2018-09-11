// config.cpp

#include "config.h"
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <limits>
#include <iomanip>
#include <regex>
#include <cctype>
using namespace std;

// HELPER FUNCTIONS

// Trim
//  helper function to trim whitespace from a string.
string Trim(string s)
{
    string::size_type start = s.find_first_not_of(" \t");
    if (start == string::npos)
        return string();
    string::size_type end = s.find_last_not_of(" \t");
    return s.substr(start, end - start + 1);
}

// ConvertFromString
//  helper template functors to convert strings into any needed types.
//  to convert string to string, no transformation is done;
//  to convert string to bool, "false" is false and all other values are true;
//  to convert string to vector, comma-delimited tokens are passed to appropriate handler for value type;
//  to convert string to LuaFunction, LuaFunction constructor is used with string as compact form;
//  for other conversions, uses istringstream.
template <typename Type>
struct ConvertFromString {
    Type operator()(string s)
    {
        Type return_value;
        istringstream iss(s);
        iss >> return_value;
        return return_value;
    }
};

template <>
struct ConvertFromString<string> {
    string operator()(string s)
    {
        return s;
    }
};

template <>
struct ConvertFromString<bool> {
    bool operator()(string s)
    {
        return s != "false";
    }
};

template <typename Subtype>
struct ConvertFromString<vector<Subtype>> {
    vector<Subtype> operator()(string s)
    {
        ConvertFromString<Subtype> subconv;
        vector<Subtype> value_vector;
        string::size_type start = 0, end = 0;
        while (end != string::npos)
        {
            end = s.find(',', start);
            value_vector.push_back(subconv(Trim(s.substr(start, end - start))));
            start = end + 1;
        }
        return value_vector;
    }
};

#ifndef CONFIG_NO_LUAFUNC
template <typename Subtype>
struct ConvertFromString<LuaFunction<Subtype>> {
    LuaFunction<Subtype> operator()(string s)
    {
        return LuaFunction<Subtype>(s);
    }
};
#endif

// ConvertToString
//  helper template functors to convert parameter types to strings for
//  writing to an output stream, ensuring that necessary precision is kept
//  for numerical types.
template <typename Type>
struct ConvertToString {
    string operator()(Type const& v)
    {
        ostringstream oss;
        oss << setprecision(numeric_limits<Type>::digits10 + 1) << v;
        return oss.str();
    }
};

template<>
struct ConvertToString<string> {
    string operator()(string const& v)
    {
        return v;
    }
};

template<>
struct ConvertToString<bool> {
    string operator()(bool const& v)
    {
        return v ? "true" : "false";
    }
};

template<typename Subtype>
struct ConvertToString<vector<Subtype>> {
    string operator()(vector<Subtype> const& v)
    {
        ConvertToString<Subtype> subconv;
        string total;
        for (unsigned int i = 0; i < v.size(); ++i)
        {
            total += subconv(v[i]);
            if (i + 1 != v.size()) total += ',';
        }
        return total;
    }
};

#ifndef CONFIG_NO_LUAFUNC
template<typename Subtype>
struct ConvertToString<LuaFunction<Subtype>> {
    string operator()(LuaFunction<Subtype> const& v)
    {
        return v.CompactForm();
    }
};
#endif

// SetParameterFromMap2
//  used by SetParameterFromMap below.
//  throws an error, as should not be used on a non-vector.
template <typename Type>
bool SetParameterFromMap2(Type& parameter,
                          string parameter_name,
                          Parameters::NameValueMap& nvm,
                          int index)
{
    (void) parameter;
    (void) nvm;
    (void) index;
    throw runtime_error(parameter_name + " is not a vector, but tried to subset it.");
}

// SetParameterFromMap2
//  used by SetParameterFromMap below.
//  sets the element [index] of the supplied vector according to the name-value map supplied.
template <typename Subtype>
bool SetParameterFromMap2(vector<Subtype>& parameter,
                          string parameter_name,
                          Parameters::NameValueMap& nvm,
                          int index)
{
    auto map_entry = nvm.find(parameter_name);
    if (map_entry != nvm.end())
    {
        ConvertFromString<Subtype> conv;
        parameter[index] = conv(map_entry->second);
        return true;
    }
    return false;
}

// SetParameterFromMap
//  assigns a value to the supplied parameter.
//  if there is an entry in the name-value map with the correct parameter name,
//  that entry is used; otherwise the parameter is unchanged. Returns true if
//  parameter was set.
template <typename Type>
bool SetParameterFromMap(Type& parameter,
                         string parameter_name,
                         Parameters::NameValueMap& nvm)
{
    for (auto& p : nvm)
    {
        auto dot = p.first.find('.');
        if (dot == string::npos)
        {
            if (p.first == parameter_name)
            {
                ConvertFromString<Type> conv;
                parameter = conv(p.second);
                return true;
            }
        }
        else
        {
            if (p.first.substr(0, dot) == parameter_name)
            {
                int index = stoi(p.first.substr(dot + 1));
                Parameters::NameValueMap sub_nvm;
                sub_nvm[p.first.substr(0, dot)] = p.second;
                return SetParameterFromMap2(parameter, parameter_name, sub_nvm, index);
            }
        }
    }
    return false;

    // auto map_entry = nvm.find(parameter_name);
    // if (map_entry != nvm.end())
    // {
    //     ConvertFromString<Type> conv;
    //     parameter = conv(map_entry->second);
    //     return true;
    // }
    // return false;
}

// CheckDeprecated
//  throw an exception if a deprecated parameter has been set in the name-value map.
void CheckDeprecated(string deprecated_name,
                     Parameters::NameValueMap& nvm,
                     string stage)
{
    if (nvm.find(deprecated_name) != nvm.end())
        cout << "Config: the parameter " << deprecated_name << " assigned from " << stage << " is deprecated.\n";
}

// PARAMETERS METHODS

// Parameters constructor
//  sets parameters to default values.
Parameters::Parameters()
{
    // INCLUSION PASS 2: Collect all parameter names.
    #define PARAMETER(Type, Name, Default...) _param_names.insert(#Name);
    #define DEPRECATED(Name)
    #define REQUIRE(Condition)
    #include "config_def.h"
    #undef PARAMETER
    #undef DEPRECATED
    #undef REQUIRE

    Reset();
}

// Reset
//  resets the parameters, restoring them to their defaults.
void Parameters::Reset()
{
    _nvm_sweeps = vector<NameValueMap>(1);
    _nvm_sweep_names = vector<string>(1, "Main");
    _nvm_override.clear();
    _sweep = 0;
    _good = true;
    _assignment_virgin = true;
    _template_mode = false;
    SetAllToDefault();
}

// Read (istream)
//  load parameters from an input stream, ignoring //-style comments
//  and interpreting statements of the form pname = value as assignments.
//  Statements of the form [SweepName] name new parameter sweeps.
void Parameters::Read(istream& in)
{
    InterpretLines(in);
    AssignFromMap(_nvm_sweeps[_sweep], "input stream");
}

// Read (string)
//  load parameters from a named file, using the input stream method above.
void Parameters::Read(string filename)
{
    ifstream fin(filename);
    if (!fin.good())
        throw runtime_error("Could not load parameter file " + filename);
    InterpretLines(fin);
    AssignFromMap(_nvm_sweeps[_sweep], "parameter file " + filename);
}

// Read (argc, argv)
//  load parameters from the command line; parameter names are specified by
//  prefixing with '-'. Values are optionally taken from the following token;
//  if not provided, the parameter is set to true. However, negative numbers
//  can be specified as values using '-' as normal. If argv[1] is not prefixed
//  with '-', it is taken as a filename from which to load parameters before
//  overriding from the command line. If a config file is specified in that
//  manner, the next argument can be a sweep range in the format N or N-M,
//  where N and M are integers (M = N by default) specifying a range of sweeps
//  to limit the run by.
void Parameters::Read(int argc, char* argv[])
{
    // First try load parameters from parameter file, if applicable.
    int first = 1;
    if (argc > first && argv[first][0] != '-')
    {
        string filename = argv[first];
        ifstream fin(filename);
        if (!fin.good())
            throw runtime_error("Could not load parameter file " + filename);
        InterpretLines(fin);
        first = 2;

        // Attempt to read sweep range in the form e.g. "12" or "12-15"
        if (argc > first && argv[first][0] != '-')
        {
            ConvertFromString<int> conv;
            int sweep_start, sweep_end;
            string range = argv[first];
            
            auto dash = range.find('-');
            if (dash != string::npos)
            {
                sweep_start = conv(range.substr(0, dash));
                sweep_end = conv(range.substr(dash + 1));
            }
            else
            {
                sweep_start = sweep_end = conv(range);
            }
            if (sweep_start < 1 || sweep_end < 1 || sweep_start > NSweeps() || sweep_end > NSweeps() || sweep_start > sweep_end)
            {
                throw runtime_error("Invalid sweep range " + range);
            }

            // combine any skipped sweeps into the first non-skipped sweep
            for (int i = sweep_start - 2; i >= 0; --i)
            {
                _nvm_sweeps[sweep_start - 1].insert(_nvm_sweeps[i].begin(), _nvm_sweeps[i].end());
            }

            _nvm_sweeps = vector<NameValueMap>(_nvm_sweeps.begin() + sweep_start - 1, _nvm_sweeps.begin() + sweep_end);
            _nvm_sweep_names = vector<string>(_nvm_sweep_names.begin() + sweep_start - 1, _nvm_sweep_names.begin() + sweep_end);

            first = 3;
        }
    }

    // Now load overriding parameters from command line.
    // Anything starting with a '-' and followed with at least one a-z, A-Z or _ is interpreted as the name of a parameter.
    // Anything else is interpreted as the value of the preceding parameter.
    string parameter_name;
    for (int a = first; a < argc; ++a)
    {
        string token = argv[a];
        if (token.length() > 1 && token[0] == '-' && (isalpha(token[1]) || token[1] == '_'))
        {
            parameter_name = token.substr(1);
            _nvm_override[parameter_name] = "true";
        }
        else
        {
            if (parameter_name.empty())
            {
                throw runtime_error("Unexpected token in command line: " + token);
            }
            _nvm_override[parameter_name] = token;
            parameter_name.clear();
        }
    }

    AssignFromMap(_nvm_sweeps[_sweep], "sweep " + _nvm_sweep_names[_sweep]);
    AssignFromMap(_nvm_override, "command line");
}

// Read line
//  interpret a single command.
void Parameters::ReadLine(string line)
{
    InterpretLine(line);
    AssignFromMap(_nvm_sweeps[_sweep], "single line");
}

// Write
//  write parameters to the specified output stream, in a format which is
//  human- and Parameters- readable.
// NOTE: Here I prefix local identifiers with an underscore
// and fully qualify global identifiers to avoid name clashes.
void Parameters::Write(ostream& _out, string _pre) const
{
    const int _col1_width = 18;
    const int _col2_width = 24;

    // INCLUSION PASS 3: Write to output stream.
    #define PARAMETER(Type, Name, Default...) \
      { _out << _pre; \
        ConvertToString<Type> _conv; \
        _out.width(_col1_width); _out << std::left << #Name << " = "; \
        _out.width(_col2_width); _out << std::left << _conv(Name); \
        Type _def ( Default ); \
        _out << " // " << #Type << ", default = " << _conv(_def); \
        _out << std::endl; }
    #define DEPRECATED(Name)
    #define REQUIRE(Condition)
    #include "config_def.h"
    #undef PARAMETER
    #undef DEPRECATED
    #undef REQUIRE
}

// Set
//  Low-level set: set a parameter by using the provided name-value pair.
void Parameters::Set(string name, string value)
{
    NameValueMap nvm;
    nvm[name] = value;
    AssignFromMap(nvm, "direct call to Set");
}

// Sweep
//  Get the number of the current sweep.
int Parameters::Sweep() const
{
    return _sweep;
}

// SweepName
//  Get the name of the current sweep.
string Parameters::SweepName() const
{
    return _nvm_sweep_names[_sweep];
}

// NSweeps
//  Get the total number of sweeps.
int Parameters::NSweeps() const
{
    return (int)_nvm_sweeps.size();
}

// NextSweep
//  Advances to the next sweep.
void Parameters::NextSweep()
{
    ++_sweep;
    if (_sweep == NSweeps()) // silently stay on last sweep (useful for loops)
    {
        _good = false;
        _sweep = NSweeps() - 1;
    }
    else
    {
        AssignFromMap(_nvm_sweeps[_sweep], _nvm_sweep_names[_sweep]);
        AssignFromMap(_nvm_override, "command line");
    }
}

// GoToSweep
//  Sets parameters to those of a given sweep.
//  This is less efficient than jumping forward a certain number of times using NextSweep
//  if that is an option.
void Parameters::GoToSweep(int s)
{
    if (s > NSweeps())
    {
        throw runtime_error("Requested sweep in GoToSweep larger than maximum.");
    }

    SetAllToDefault();
    for (int S = 0; S < s; ++S)
    {
        NextSweep();
    }
}

// Good
//  Return true if we have not yet done all sweeps.
bool Parameters::Good() const
{
    return _good;
}

// InterpretLines
//  call InterpretLine on sequential lines of in.
void Parameters::InterpretLines(istream& in)
{
    while (in.good())
    {
        string line;
        getline(in, line);
        InterpretLine(line);
    }
}

// InterpretLine
//  first removes //-style comments from the line.
//  then interprets the following style of lines:
//      [SweepName]
//      SweepTemplate <A,B,C>
//      [SweepTemplateInstantiation]: SweepTemplate<0, 1, 2>
//      parameter = value
void Parameters::InterpretLine(string line)
{
    // Remove comments and leading/trailing whitespace from the line.
    string::size_type comment_start = line.find("//");
    if (comment_start != string::npos)
        line.resize(comment_start);
    line = Trim(line);
    if (line.empty())
        return;

    // Match interpretable lines.
    smatch matches;

    // Sweep heading
    if (regex_match(line, matches, regex("\\[\\s*(\\S+)\\s*\\]")))
    {
        // If we haven't seen an assignment yet, set the current sweep name to the supplied name.
        if (_assignment_virgin)
        {
            _nvm_sweep_names.back() = matches[1];
        }
        // Otherwise, make a new sweep.
        else
        {
            _nvm_sweeps.push_back(NameValueMap());
            _nvm_sweep_names.push_back(matches[1]);
        }

        _template_mode = false;
    }

    // Template declaration
    else if (regex_match(line, matches, regex("(\\w+)\\s*<(\\s*\\w+\\s*(\\s*,\\s*\\w+)*\\s*)>")))
    {
        // Make a new template.
        _templates.push_back(NameValueMap());
        _template_names.push_back(matches[1]);
        ConvertFromString<vector<string>> conv;
        _template_params.push_back(conv(matches[2]));

        _template_mode = true;
    }

    // Template instantiation
    else if (regex_match(line, matches, regex("\\[\\s*(\\S+)\\s*\\]\\s*:\\s*(\\w+)\\s*<(.*)>")))
    {
        // Make sure this is a valid template
        auto templ = std::find(_template_names.begin(), _template_names.end(), matches[2]);
        if (templ != _template_names.end())
        {
            auto which = templ - _template_names.begin();
            
            // Get the substitutions
            ConvertFromString<vector<string>> conv;
            auto subst = conv(matches[3]);
            if (subst.size() != _template_params[which].size())
            {
                cout << "Config: incorrect number of template parameters in line [" << line << "].\n";
            }
            else
            {
                // If we haven't seen an assignment yet, set the current sweep name to the supplied name.
                if (_assignment_virgin)
                {
                    _nvm_sweep_names.back() = matches[1];
                }
                // Otherwise, make a new sweep.
                else
                {
                    _nvm_sweeps.push_back(NameValueMap());
                    _nvm_sweep_names.push_back(matches[1]);
                }

                // Fill the sweep as though each assignment in the invoked template were now run, with appropriate substitutions.
                // Include the special substitution <$Name> -> name of this instantiation.
                for (auto& entry : _templates[which])
                {
                    string value = entry.second;
                    if (value.find('<') != string::npos)    // Don't bother trying any argument replacement if there are no <s.
                    {
                        for (unsigned int p = 0; p < _template_params[which].size(); ++p)
                            value = regex_replace(value, regex("<\\s*" + Trim(_template_params[which][p]) + "\\s*>"), Trim(subst[p]));
                        value = regex_replace(value, regex("<\\s*\\$Name\\s*>"), string(matches[1]));
                    }
                    _nvm_sweeps.back()[entry.first] = value;
                }

                _assignment_virgin = false;
            }
        }

        _template_mode = false;
    }

    // Assignment
    else if (regex_match(line, matches, regex("(\\w+)\\s*=\\s*(.*)")))
    {
        // Record the value of this parameter, either into the current sweep or the current template.
        if (_template_mode)
        {
            _templates.back()[matches[1]] = matches[2];
        }
        else
        {
            _nvm_sweeps.back()[matches[1]] = matches[2];
            _assignment_virgin = false;
        }
    }

    // Uninterpretable line
    else
    {
        cout << "Config: could not interpret line [" << line << "].\n";
    }
}

// SetAllToDefault
//  sets all defined parameters to the corresponding default value.
void Parameters::SetAllToDefault()
{
    // INCLUSION PASS 4: Set default values for each defined parameter.
    #define PARAMETER(Type, Name, Default...) { Type _def ( Default ); Name = _def; }
    #define DEPRECATED(Name)
    #define REQUIRE(Condition) \
        if (!(Condition)) \
            std::cout << "Config: required condition { " << #Condition << " } not met for default value of parameter.\n";
    #include "config_def.h"
    #undef PARAMETER
    #undef DEPRECATED
    #undef REQUIRE
}

// AssignFromMap
//  attempts to assign a value to all defined parameters from the provided
//  name-value map.
void Parameters::AssignFromMap(NameValueMap& _nvm, string _stage)
{
    int _active_line = -1;

    (void)_stage;
    (void)_active_line;

    // Check to see if any undefined parameters are being set.
    for (auto& _p : _nvm)
    {
        if (_param_names.find(_p.first.substr(0, _p.first.find('.'))) == _param_names.end())
            std::cout << "Config: unrecognized parameter " << _p.first << " set from " << _stage << ".\n";
    }

    // INCLUSION PASS 5: Call SetParameterFromMap on each defined parameter.
    #define PARAMETER(Type, Name, Default...) \
        if (SetParameterFromMap<Type>(Name, #Name, _nvm)) \
            _active_line = __LINE__;
    #define DEPRECATED(Name) \
        CheckDeprecated(#Name, _nvm, _stage);
    #define REQUIRE(Condition) \
        if (_active_line == __LINE__ && !(Condition)) \
            std::cout << "Config: required condition { " << #Condition << " } not met for parameter assigned from " << _stage << ".\n";
    #include "config_def.h"
    #undef PARAMETER
    #undef DEPRECATED
    #undef REQUIRE
}
