#ifndef FUNC_LIST_H 
#define FUNC_LIST_H 

#include <vector>
#include <string>


typedef struct _func_signature_t {
	std::string function_name;
	std::vector<std::string> params;
	std::vector<std::string> ret_vals;
	std::vector<std::string> leading_inst_pc;
	std::vector<std::string> leading_inst_ret_val_pc;
} func_signature_t;

class func_list_t {
	std::vector <func_signature_t>  f_list;
	public:
	bool find(std::string f_name) {
		for(int i=0; i<f_list.size(); i++) 
			if(f_list[i].function_name.compare(f_name) == 0)
				return true;
		return false;
	}
	void add(func_signature_t func_sig) {
		f_list.push_back(func_sig);
	}
	int size() {
		return f_list.size();
	}
	std::string function_name(int i) {
		if(i<f_list.size())
			return f_list[i].function_name;
		else 
			return "";
	}
	std::vector<std::string> get_params(int i) {
		return f_list[i].params;
	}
	std::vector<std::string> get_ret_vals(int i) {
		return f_list[i].ret_vals;
	}
	void set_leading_inst_pc(int i, int j, std::string pc) {
		f_list[i].leading_inst_pc[j] = pc;
	}
	void set_leading_inst_ret_val_pc(int i, int j, std::string pc) {
		f_list[i].leading_inst_ret_val_pc[j] = pc;
	}
	std::string get_leading_inst_pc (std::string func_name, std::string local_reg) {
		int index = -1;
		for(int i=0; i<f_list.size(); i++) 
			if(f_list[i].function_name.compare(func_name) == 0) {
				index = i; 
				break ; 
			}
		if(index != -1) {
			for(int i=0; i<f_list[index].params.size(); i++) {
				if(f_list[index].params[i].compare(local_reg) == 0)
					return f_list[index].leading_inst_pc[i];
			}

		}
		return "";
	}
	std::string get_leading_inst_ret_val_pc (std::string func_name, std::string local_reg) {
		int index = -1;
		for(int i=0; i<f_list.size(); i++) 
			if(f_list[i].function_name.compare(func_name) == 0) {
				index = i; 
				break ; 
			}
		if(index != -1) {
			for(int i=0; i<f_list[index].ret_vals.size(); i++) {
				if(f_list[index].ret_vals[i].compare(local_reg) == 0)
					return f_list[index].leading_inst_ret_val_pc[i];
			}

		}
		return "";
	}
};

#endif //FUNC_LIST_H 
