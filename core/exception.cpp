#include "exception.h"



void Exception::save_error(std::string info, std::string file, int line) {
	std::cout << info << std::endl;
}
void Exception::save_error(std::string info) {
	std::cout << info << std::endl;
}
void Exception::save_info(std::string info) {
	std::cout << info << std::endl;
}

