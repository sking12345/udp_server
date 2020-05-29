#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_
#include <iostream>
#include <string>

class Exception {
  public:
	void static save_error(std::string info, std::string file, int line);
	void static save_error(std::string info);
	void static  save_info(std::string info);

};

#endif