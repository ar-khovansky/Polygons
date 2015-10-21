#pragma once

#include <stdexcept>



class call_error : public std::logic_error
{
public:
	call_error(std::string const &msg) : logic_error(msg) {}
	call_error(char const *msg) : logic_error(msg) {}
};


class state_error : public call_error
{
public:
	state_error(std::string const &msg) : call_error(msg) {}
	state_error(char const *msg) : call_error(msg) {}
};
