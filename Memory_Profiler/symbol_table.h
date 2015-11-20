/*
 * symbol_table.h
 *
 *  Created on: Nov 20, 2015
 *      Author: egezhus
 */

#ifndef SYMBOL_TABLE_H_
#define SYMBOL_TABLE_H_

#include <string>
#include <stdint.h>

using namespace std;

class symbol_table_entry_class {

public:
	string name;
	uint64_t address;

	symbol_table_entry_class() : name{""},address{0} {};
	symbol_table_entry_class(string name_p, uint64_t address_p): name{name_p},address{address_p} {};

	symbol_table_entry_class(const symbol_table_entry_class &obj)noexcept;
	symbol_table_entry_class& operator=(const symbol_table_entry_class &obj)noexcept;

	symbol_table_entry_class(symbol_table_entry_class &&obj)noexcept;
	symbol_table_entry_class& operator=(symbol_table_entry_class &&obj)noexcept;

	bool operator==(const string& symbol_name) const {
		    if(this->name.compare(symbol_name) == 0)return true;
		    else return false;
	}
	bool operator!=(const string& symbol_name) const {return(!(this->name == symbol_name));}
	bool operator<(const symbol_table_entry_class& entry) const {return (address < entry.address);}
	bool operator>(const symbol_table_entry_class& entry) const {return (!(address < entry.address));}
	bool operator<=(const symbol_table_entry_class& entry) const {return (address < entry.address);}
	bool operator>=(const symbol_table_entry_class& entry) const {return (!(address > entry.address));}


};

bool operator == (const uint64_t &address, const symbol_table_entry_class& entry);
bool operator != (const uint64_t &address, const symbol_table_entry_class& entry);
bool operator <  (const uint64_t &address, const symbol_table_entry_class& entry);
bool operator >  (const uint64_t &address, const symbol_table_entry_class& entry);
bool operator <= (const uint64_t &address, const symbol_table_entry_class& entry);
bool operator >= (const uint64_t &address, const symbol_table_entry_class& entry);

bool operator == (const symbol_table_entry_class& entry, const uint64_t &address);
bool operator != (const symbol_table_entry_class& entry, const uint64_t &address);
bool operator <  (const symbol_table_entry_class& entry, const uint64_t &address);
bool operator >  (const symbol_table_entry_class& entry, const uint64_t &address);
bool operator <= (const symbol_table_entry_class& entry, const uint64_t &address);
bool operator >= (const symbol_table_entry_class& entry, const uint64_t &address);

#endif /* SYMBOL_TABLE_H_ */
