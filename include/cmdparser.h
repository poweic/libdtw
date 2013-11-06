#ifndef _CMD_PARSER_H
#define _CMD_PARSER_H

#include <iostream>
#include <string>
#include <map>
#include <cstdlib>

using namespace std;

class CmdParser {
  public:
    CmdParser(int argc, char** argv): _argc(argc), _argv(argv), _usage(""), _options("") {
      _usage = "Usage: " + string(_argv[0]) + " ";
    }

#ifdef __GXX_EXPERIMENTAL_CXX0X__
    typedef initializer_list<string> strlist;

    CmdParser& add(string option, strlist description_list, bool isMandatory = true, string defaultArg = "") {
      return _registerOption<strlist>(option, description_list, isMandatory, defaultArg);
    }
#endif

    CmdParser& add(string option, string description, bool isMandatory = true, string defaultArg = "") {
      return _registerOption<string>(option, description, isMandatory, defaultArg);

    }

    CmdParser& addGroup(string description) {
      if (description[description.size() - 1] == ':')
	description = description.substr(0, description.size() - 1);

      _options += "\n" + description + "\n";
      return *this;
    }

    string find(string option) const {
      map<string, Arg>::const_iterator itr = _arguments.find(option);

      if (itr == _arguments.end())
	return string();

      return itr->second.parameter;
    }

    void showUsageAndExit() const {
      cout << endl;
      cout << _usage << endl;
      cout << _options << endl;
      exit(-1);
    }

    bool isOptionLegal() {
      if ( this->_lookingForHelp() )
	this->showUsageAndExit();

      for (map<string, Arg>::iterator i=_arguments.begin(); i != _arguments.end(); ++i) {
	const string& opt = i->first;
	Arg& arg = i->second;
	arg.parameter = this->_find(opt);

	if (arg.parameter == "") {
	  if (!arg.optional)
	    return this->_illegalOption(opt);

	  arg.useDefault();
	}
      }

      return true;
    }

  private:

    template <typename T>
    CmdParser& _registerOption(string option, T description, bool isMandatory, string defaultArg = "") {

      bool optional = !isMandatory;
      Arg arg(option, description, optional, defaultArg);

      _arguments[option] = arg;
      this->_appendUsage(arg);
      _options += arg.getDescription();

      return *this;
    }

    struct Arg {
      Arg() {}

      Arg(string opt, string des, bool o, string darg) {
	_init(opt, o, darg);

	description =
	  this->_optionStr()
	  + des + "\n"
	  + this->_defaultArgStr();
      }

      void _init(string opt, bool o, string darg) {
	option = opt;
	optional = o;
	default_arg = darg;
      }

#ifdef __GXX_EXPERIMENTAL_CXX0X__
      Arg(string opt, strlist des_list, bool o, string darg) {
	_init(opt, o, darg);

	description =
	  this->_optionStr()
	  + this->_getDescription(des_list)
	  + this->_defaultArgStr();
      }

      string _getDescription(const strlist& des_list) {
	string des;
	int counter = 0;
	for (auto& d: des_list) {
	  if (counter++ != 0)
	    des += this->getPadding();
	  des += d + "\n";
	}

	return des;
      }
#endif

      string _optionStr() {
	string opt = "  " + option;
	opt.resize(PADDING_RIGHT, ' ');
	opt += "\t";
	return opt;
      }

      string getPadding() {
	string padding;
	padding.resize(PADDING_RIGHT, ' ');
	padding += "\t";
	return padding;
      }

      string _defaultArgStr() {
	if (optional && default_arg != "")
	  return this->getPadding() + "(default = " + default_arg + ")\n";

	return "";
      }

      string getDescription() {
	return description;
      }

      void useDefault() {
	this->parameter = this->default_arg;
      }

      static const size_t PADDING_RIGHT = 24;

      string option;
      string description;
      bool optional;
      string default_arg;
      string parameter;
    };

    string _find(const string& option) {
      for(int i=1; i<_argc; ++i) {
	string arg(_argv[i]);

	if (arg == option && i+1<_argc)
	  return _argv[i+1];

	size_t pos = arg.find_first_of('=');
	if (pos == string::npos)
	  continue;

	string left = arg.substr(0, pos);
	string right = arg.substr(pos + 1);
	if (left == option)
	  return right;
      }

      return string();
    }

    bool _illegalOption(const string& opt) const {
      cout << "Missing argument after " + opt << endl;
      return false;
    }

    bool _lookingForHelp() const {
      string help("--help");
      for (int i=1; i<_argc; ++i) {
	if (help.compare(_argv[i]) == 0)
	  return true;
      }
      return false;
    }

    void _appendUsage(const Arg& arg) {
      if (arg.optional)
	_usage += " [" + arg.option + " ]";
      else
	_usage += " <" + arg.option + " >";
    }

    int _argc;
    char** _argv;

    string _usage;
    string _options;
    map<string, Arg> _arguments;
};

#endif // _CMD_PARSER_H
