/* ------------------------------------------------
Copyright 2014 AT&T Intellectual Property
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 ------------------------------------------- */

#include "iface_q.h"
#include<errno.h>
#include <algorithm>
#include <cctype>


using namespace std;

string int_to_string(int i){
    string ret;
    char tmpstr[100];
    sprintf(tmpstr,"%d",i);
    ret=tmpstr;
    return(ret);
}


//		Interface for the res parser
extern int ResParserparse(void);
extern FILE *ResParserin;
extern int ResParserdebug;
void ResParser_setfileinput(FILE *f);
void ResParser_setstringinput(char *s);
extern int flex_res_lineno;

extern xml_parser* parser;

///////////////////////////////////////
///		iface_t methods

	void iface_t::add_property(const char *name, const char *att){
		string nm(name);
		transform(nm.begin(), nm.end(), nm.begin(), (int(*)(int)) tolower);
		vals[nm].push_back(att);
	}

	void iface_t::add_property(const char *name, const char **atts){
		string val = "";
		string nm = name;
		transform(nm.begin(), nm.end(), nm.begin(), (int(*)(int)) tolower);
		if(atts[0]) val = atts[1];

		vals[nm].push_back(val);
	}

	void iface_t::add_property(const char *name, vector<string> &val_vec){
		string val = "";
		string nm = name;
		transform(nm.begin(), nm.end(), nm.begin(), (int(*)(int)) tolower);
		if(val_vec.size()) val = val_vec[0];

		vals[nm].push_back(val);
	}

	int iface_t::finalize(string &errs){
		string tag ="ERROR in interface starting at line "+int_to_string(lineno)+", ";
		string e;
		if(vals.count("name") == 0){
			e += "Name not specified. ";
		}else{
			if(vals["name"].size() > 1){
				e+="More than one name specified. ";
			}
		}

		if(e != ""){
			errs += tag + e + "\n";
			return(1);
		}
		return(0);
	}

	string iface_t::to_string(){
		int i;
		string ret = "Interface "+vals["name"][0]+":\n";
		map<string, vector<string> >::iterator svmi;
		for(svmi=vals.begin();svmi!=vals.end();++svmi){
			ret += "\t"+(*svmi).first + " : ";
			for(i=0;i<((*svmi).second).size();++i){
				if(i>0) ret+=", ";
				ret += (*svmi).second[i];
			}
			ret += "\n";
		}

		return ret;
	}

	string iface_t::get_name(){
		if(vals.count("name") == 0) return (string)"";
		return (vals["name"])[0];
	}

	string iface_t::get_host(){
		if(vals.count("host") == 0) return (string)"";
		return (vals["host"])[0];
	}


	bool iface_t::eval_Contains(string prop, string val){
		// convert to lowercase
		transform(prop.begin(), prop.end(), prop.begin(), (int(*)(int)) tolower);

		if(vals.count(prop) == 0) return false;
		int i;
		for(i=0;i<vals[prop].size();++i){
			if((vals[prop])[i] == val) return true;
		}
		return false;
	}

	bool iface_t::eval_Equals(string prop, string val){
		// convert to lowercase
		transform(prop.begin(), prop.end(), prop.begin(), (int(*)(int)) tolower);
		if(vals.count(prop) == 0) return false;
		if(vals[prop].size() != 1) return false;
		int i;
		for(i=0;i<vals[prop].size();++i){
			if((vals[prop])[i] == val) return true;
		}
		return false;
	}

	bool iface_t::eval_Exists(string prop){
		// convert to lowercase
		transform(prop.begin(), prop.end(), prop.begin(), (int(*)(int)) tolower);
		if(vals.count(prop) == 0) return false;
		return true;
	}


///////////////////////////////////////
///		gs_host_t methods

	void gs_host_t::add_property(const char *name, const char *att){
		string nm(name);
		transform(nm.begin(), nm.end(), nm.begin(), (int(*)(int)) tolower);
		vals[nm].push_back(att);
	}

	void gs_host_t::add_property(const char *name, vector<string> &val_vec){
		string val = "";
		string nm = name;
		transform(nm.begin(), nm.end(), nm.begin(), (int(*)(int)) tolower);
		if(val_vec.size()) val = val_vec[0];

		vals[nm].push_back(val);
	}

	int gs_host_t::finalize(string &errs){
		string tag ="ERROR in host starting at line "+int_to_string(lineno)+", ";
		string e;
		if(vals.count("name") == 0){
			e += "Name not specified. ";
		}else{
			if(vals["name"].size() > 1){
				e+="More than one name specified. ";
			}
		}

		if(e != ""){
			errs += tag + e + "\n";
			return(1);
		}
		return(0);
	}





///////////////////////////////////////////
///		Resource parser methods

	int resource_parser::finalize_iface(string &errs){
		int ret = curr_iface->finalize(errs);
		if(ret) {
			delete curr_iface;
			failure = true;
		}else{
			ifaces.push_back(curr_iface);
			curr_host->add_interface(curr_iface);
		}
		return ret;
	}


	int resource_parser::finalize_host(string &errs){
		int ret = curr_host->finalize(errs);
		if(ret) {
			delete curr_host;
			failure = true;
		}else{
			hosts.push_back(curr_host);
			// push the host name to every interface
			curr_host->propagate_name();
		}
		return ret;
	}

	string resource_parser::to_string(){
		string ret;
		int i;
		for(i=0;i<ifaces.size();i++)
			ret += ifaces[i]->to_string();
		return ret;
	}


	vector<pair<string,string> > resource_parser::find_ifaces(predicate_t *pr){
		int i;
		vector<pair<string,string> > ret;
		for(i=0;i<ifaces.size();++i){
			if(this->eval_pred(i,pr)){
				pair<string,string> p(ifaces[i]->get_host(), ifaces[i]->get_name());
				ret.push_back(p);
			}
		}
		return ret;
	}


	vector<int> resource_parser::get_ifaces_by_Name(std::string host_name, std::string if_name){
		int i;
		vector<int> ret;
		for(i=0;i<ifaces.size();++i){
			if(ifaces[i]->get_host() == host_name && ifaces[i]->get_name() == if_name)
				ret.push_back(i);

		}
		return ret;
	}


	bool resource_parser::eval_pred(int i, predicate_t *pr){
    vector<scalarexp_t *> op_list;
	string prop, val;

	switch(pr->get_operator_type()){
	case PRED_IN:
		fprintf(stderr,"INTERNAL ERROR in reparse_date::eval_pred, line %d, character %d, IN not supported. %d\n",
			pr->get_lineno(), pr->get_charno(), pr->get_operator_type() );
		exit(1);
	case PRED_COMPARE:
		fprintf(stderr,"INTERNAL ERROR in reparse_date::eval_pred, line %d, character %d, IN not supported. %d\n",
			pr->get_lineno(), pr->get_charno(), pr->get_operator_type() );
		exit(1);
	case PRED_UNARY_OP:
		if(pr->get_op() == "NOT")
			return(! eval_pred(i, pr->get_left_pr()) );
		fprintf(stderr,"INTERNAL ERROR in reparse_date::eval_pred, line %d, character %d, unknown unary pred operator %s.\n",
			pr->get_lineno(), pr->get_charno(), pr->get_op().c_str() );
		exit(1);
	case PRED_BINARY_OP:
		if(pr->get_op() == "AND"){
			if(! eval_pred(i, pr->get_left_pr()) ) return false;
			return  eval_pred(i, pr->get_right_pr()) ;
		}
		if(pr->get_op() == "OR"){
			if( eval_pred(i, pr->get_left_pr()) ) return true;
			return  eval_pred(i, pr->get_right_pr()) ;
		}
		fprintf(stderr,"INTERNAL ERROR in reparse_date::eval_pred, line %d, character %d, unknown binary pred operator %s.\n",
			pr->get_lineno(), pr->get_charno(), pr->get_op().c_str() );
		exit(1);
	case PRED_FUNC:
		op_list = pr->get_op_list();
		if(op_list[0]->get_operator_type() != SE_PARAM){
			fprintf(stderr,"INTERNAL ERROR : ifq parameters are supposed to be of type SE_PARAM, found %s at position 0\n",op_list[0]->get_op().c_str());
			exit(1);
		}else{
			prop=op_list[0]->get_op();
		}
		if(op_list.size() > 1){
			if(op_list[1]->get_operator_type() != SE_PARAM){
				fprintf(stderr,"INTERNAL ERROR : ifq parameters are supposed to be of type SE_PARAM, found %s at position 1\n",op_list[0]->get_op().c_str());
				exit(1);
			}else{
				val=op_list[1]->get_op();
			}
		}

		if(pr->get_op() == "Contains"){
			if(op_list.size() != 2){
				fprintf(stderr,"INTERNAL ERROR : predicate Contains expects 2 parameters, received %lu\n",op_list.size());
				exit(1);
			}
			return ifaces[i]->eval_Contains(prop,val);
		}

		if(pr->get_op() == "Equals"){
			if(op_list.size() != 2){
				fprintf(stderr,"INTERNAL ERROR : predicate Equals expects 2 parameters, received %lu\n",op_list.size());
				exit(1);
			}
			return ifaces[i]->eval_Equals(prop,val);
		}

		if(pr->get_op() == "Exists"){
			if(op_list.size() != 1){
				fprintf(stderr,"INTERNAL ERROR : predicate Exists expects 1 parameter, received %lu\n",op_list.size());
				exit(1);
			}
			return ifaces[i]->eval_Exists(prop);
		}

		fprintf(stderr,"INTERNAL ERROR : Unknown predicate %s in reparse_date::eval_pred.\n",pr->get_op().c_str());
		exit(1);

	default:
		fprintf(stderr,"INTERNAL ERROR in reparse_date::eval_pred, line %d, character %d, unknown predicate operator type %d\n",
			pr->get_lineno(), pr->get_charno(), pr->get_operator_type() );
		exit(1);
	}

	return(false);

	}


void resource_parser::startElement(const char *name, vector<string> &attr_vec, vector<string> &val_vec){
  int i;

  level.push_back(name);

  if(level.size() == 1 && level[0] == "Resources")
	in_Resources = true;

  if(in_Resources && level.size() == 2 && level[1] == "Host"){
//	new_iface(XML_GetCurrentLineNumber(parser));

	new_host(flex_res_lineno);
	for (int i = 0; i < attr_vec.size(); ++i)
		curr_host->add_property(attr_vec[i].c_str(), val_vec[i].c_str());
    in_Host = true;
  }

  if(in_Host && level.size() == 3) {
	  if ( level[2] == "Interface"){
		//	new_iface(XML_GetCurrentLineNumber(parser));
		new_iface(flex_res_lineno);
		for (int i = 0; i < attr_vec.size(); ++i)
			add_property(attr_vec[i].c_str(), val_vec[i].c_str());
		in_Iface = true;
	  } else {
		curr_host->add_property(name, val_vec);
	  }
	}

  if(in_Iface && level.size() == 4){
	add_property(name, val_vec);
  }


//  for(i=0;i<level.size();++i) printf("%s, ",level[i].c_str());
//  printf("\n");
}

///////////////////////////////////////////
///		XML parser callbacks for parsing interface resources

void resource_parser::endElement(const char *name) {

  if(in_Iface && level.size() == 3){
	string err_msg;
	int err = finalize_iface(err_msg);
	if(err) fprintf(stderr,"%s\n",err_msg.c_str());
  } else if (in_Host && level.size() == 2) {
	string err_msg;
	int err = finalize_host(err_msg);
	if(err) fprintf(stderr,"%s\n",err_msg.c_str());
  }

  level.pop_back();
}


//		Interface to ifq definition lexer and parser ...

extern int IfqParserparse(void);
extern FILE *IfqParserin;
extern int IfqParserdebug;
void IfqParser_setfileinput(FILE *f);
void IfqParser_setstringinput(char *s);


fta_parse_t *ifq_parse_result;

int ifq_t::load_ifaces(string fname, string &err){
  char buf[1000], errbuf[1000];
//  XML_Parser parser = XML_ParserCreate(NULL);

    // set the proper parser
  res_parser = new resource_parser();
  parser = res_parser;

  int done;
  int depth = 0;
  err="";

// IfqParserdebug = 1;

  // set xml parser to be

  FILE *resfile = fopen(fname.c_str(),"r");
  if(resfile == NULL){
	err="Error, can't open "+fname+", error is ";
	err+=strerror(errno); err+="\n";
	return(1);
  }

  ResParser_setfileinput(resfile);
  if(ResParserparse()){
	err = "ERROR, interface query parse failed.\n";
	return(1);
  }

/*
  XML_SetUserData(parser, rpd);
  XML_SetElementHandler(parser, startElement, endElement);
  do {
    size_t len = fread(buf, 1, sizeof(buf), resfile);
    done = len < sizeof(buf);
    if (!XML_Parse(parser, buf, len, done)) {
      sprintf(errbuf,
	      "%s at line %d\n",
	      XML_ErrorString(XML_GetErrorCode(parser)),
	      XML_GetCurrentLineNumber(parser));
		err=errbuf;
      return 1;
    }
  } while (!done);
  XML_ParserFree(parser);

  if(failed()){
	err = "Interface resource parse failed, exiting.\n";
  }
*/

  fclose(resfile);

  return(0);

}

int ifq_t::load_ifqs(string fname, string &err){
  char err_buf[1000];
  err="";

  // set the proper parser
  res_parser = new resource_parser();
  parser = res_parser;

  FILE *qfile = fopen(fname.c_str(),"r");
  if(qfile == NULL){
	sprintf(err_buf, "ERROR, can't open %s, error is %s\n",fname.c_str(),strerror(errno));
	err=err_buf;
	return(1);
  }

  ifq_parse_result = new fta_parse_t();
  IfqParser_setfileinput(qfile);
  if(IfqParserparse()){
	err = "ERROR, interface query parse failed.\n";
	return(1);
  }

  vector<table_exp_t *> ifqlist = ifq_parse_result->parse_tree_list->qlist;
  int i;
  bool dup = false;
  for(i=0;i<ifqlist.size();++i){
	string nm = ifqlist[i]->nmap["name"];
	if(ifq_map.count(nm)){
		err += "ERROR, Duplicate interface query "+nm+"\n";
		dup = true;
	}
	ifq_map[nm] = ifqlist[i]->wh;
  }


  if(dup) return(1);
  return(0);
}


vector<pair<string,string> > ifq_t::eval(string qname, int &err_no){
	vector<pair<string,string> > retval;
	err_no = 0;

	if(ifq_map.count(qname) == 0){
		err_no = 1;
		return retval;
	}
	if(res_parser->failed()){
		err_no = 2;
		return retval;
	}

	return res_parser->find_ifaces(ifq_map[qname]);

}

vector<string> ifq_t::get_iface_vals(string host_name, string if_name, string property, int &err_no, string &err_str){
	vector<string> retval;
	err_no = 0;

	if(res_parser->failed()){
		err_no = 1; err_str = "interface resource parse failed.";
		return retval;
	}

	vector<int> ifi = res_parser->get_ifaces_by_Name(host_name, if_name);
	if(ifi.size() == 0){
		err_no = 1; err_str="interface not found.";
		return retval;
	}
	if(ifi.size()>1){
		err_no = 1; err_str="multiple interfaces found.";
		return retval;
	}

	return res_parser->get_property(ifi[0], property);

}


vector<string> ifq_t::get_hosts() {
	vector<string> ret;
	for(int i=0;i<res_parser->hosts.size();++i){
		ret.push_back(res_parser->hosts[i]->get_name());
	}

	return ret;
}
