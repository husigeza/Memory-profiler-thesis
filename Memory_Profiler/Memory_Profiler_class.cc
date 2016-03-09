#include <iostream>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <map>
#include <vector>
#include <algorithm>
#include <fstream>
#include <time.h>

#include "Memory_Profiler_handler_template.h"
#include "Memory_Profiler_class.h"


using namespace std;

Memory_Profiler::Memory_Profiler(string fifo_path) {

	this->fifo_path = fifo_path;

	if (mkfifo(this->fifo_path.c_str(), 0666) == -1) {

		if (errno == EEXIST) {
			cout << "FIFO already exists" << endl;
		} else {
			cout << "Failed creating FIFO" << "errno: " << errno << endl;
			return;
		}
	}
	else {
	cout << "FIFO is created" << endl;
	}
	mem_prof_fifo = open(fifo_path.c_str(), O_RDONLY | O_NONBLOCK );

}

Memory_Profiler::~Memory_Profiler() {

	close(mem_prof_fifo);
	unlink(fifo_path.c_str());
	Processes.clear();
}

bool Memory_Profiler::Add_Process_to_list(const pid_t PID) {


	if (Processes.find(PID) == Processes.end()) {

		try{

			Processes.insert(make_pair(PID,template_handler<Process_handler> (*(new Process_handler(PID)))));
		}
		catch(const bool v){
			if(v == false){
				cout << "Process NOT added,"
						""
						" PID: " << dec << PID << endl;
				return false;
			}
		}

		if(Processes.find(PID) != Processes.end()){
			cout << "Process added, PID: " << dec << PID << endl;
			return true;
		}
	}

	//cout<< "Process is already in the list: " << PID << endl;
	return true;


}
void Memory_Profiler::Add_process_to_profiling(const pid_t PID) {

	//Shared memory is initialized here (if it has not been initialized before) because before starting the first profiling we want to
	//prevent the system to create unnecessary shared memories


	map<const pid_t, template_handler<Process_handler> >::iterator it = Processes.find(PID);

	if(it->second.object->Get_alive() == true){
		if(it->second.object->Get_profiled() == true){
			cout << "Process " << dec << PID << " is already under profiling!"<< endl;
			return;
		}
		if(it->second.object->Is_shared_memory_initialized() == false){
			cout << "Shared memory has not been initialized, initializing it... Process: " << dec << PID << endl;
			if(it->second.object->Init_shared_memory() == false){
				cout << "Shared memory init unsuccessful, not added to profiled state." << endl;
				return;
			}
			cout << "Shared memory init successful!" << endl;
		}
		else {
			cout << "Shared memory has been initialized for process: " << dec << PID << endl;
		}
		it->second.object->Set_profiled(true);
		it->second.object->Start_Stop_profiling();
		cout << "Process " << dec << PID << " added to profiled state" << endl;
	}
	else{
		cout << "Process " << PID << " is not alive, not added to profiled state.!" << endl;
	}
}

void Memory_Profiler::Add_all_process_to_profiling() {

	map<const pid_t, template_handler<Process_handler> >::iterator it;

	for (it = Processes.begin(); it != Processes.end(); it++) {

		Add_process_to_profiling(it->first);
	}
}

void Memory_Profiler::Set_process_alive_flag(const pid_t PID, bool value){

	Processes[PID].object->Set_alive(value);
}

bool Memory_Profiler::Get_process_alive_flag(const pid_t PID){

	return Processes[PID].object->Get_alive();
}

bool Memory_Profiler::Get_process_shared_memory_initilized_flag(const pid_t PID){

	return Processes[PID].object->Is_shared_memory_initialized();
}

void Memory_Profiler::Remove_process_from_profiling(const pid_t PID){

	if(Processes[PID].object->Get_profiled() == false){
		cout << "Process " << dec << PID << "is not under profiling!"<< endl;
		return;
	}

	if(Processes[PID].object->Remap_shared_memory()){
		Processes[PID].object->Start_Stop_profiling();
		Processes[PID].object->Set_profiled(false);
		cout << "Process " << dec << PID << " removed from profiled state!" << endl;
	}
	else{
		cout << "Process " << dec << PID << " NOT removed from profiled state!" << endl;
	}



}

void Memory_Profiler::Remove_all_process_from_profiling(){

	map<const pid_t, template_handler<Process_handler> >::iterator it;

	for (it = Processes.begin(); it != Processes.end(); it++) {

		Remove_process_from_profiling(it->first);
	}
}



void Memory_Profiler::Start_stop_profiling(const pid_t PID){

	Processes[PID].object->Start_Stop_profiling();
}


void Memory_Profiler::Start_stop_profiling_all_processes(){

	map<const pid_t, template_handler<Process_handler> >::iterator it;

	for (it = Processes.begin(); it != Processes.end(); it++) {

		if (it->second.object->Get_profiled() == true) {
			Start_stop_profiling(it->first);
		}
	}
}

bool Memory_Profiler::Remap_process_shared_memory(const pid_t PID){

	return Processes[PID].object->Remap_shared_memory();

}

void Memory_Profiler::Remap_all_process_shared_memory(){

	map<const pid_t, template_handler<Process_handler> >::iterator it;

	for (it = Processes.begin(); it != Processes.end(); it++) {
		//cout << "Remapping Process: " << it->first << endl;
			if(it->second.object->Remap_shared_memory() == false){
				cout << "Failed remapping process " << it->first << " shared memory " << endl;
			}
			else {
				//cout << "Remapping successful "<< endl;
			}
		}
 }


void Memory_Profiler::Read_FIFO() {

	vector<pid_t> alive_processes;
	pid_t pid;
	string buffer;
	size_t buff_size = 6;
	int res;
	map<const pid_t, template_handler<Process_handler> >::iterator it;

	if (mem_prof_fifo != -1) {
		while ((res = read(mem_prof_fifo, (char*)buffer.c_str(), buff_size/*sizeof(buffer)*/)) != 0) {

			if (res > 0) {
				pid = atol( buffer.c_str() );
				Add_Process_to_list(pid);
				alive_processes.push_back(pid);

			} else {
				//cout << "Failed reading the FIFO" << endl;
				return;
			}
		}
		// IF nobody has written the FIFO, res = 0, alive_processes = 0, all the processes are dead
		for (it = Processes.begin(); it != Processes.end(); it++) {
			if(find(alive_processes.begin(), alive_processes.end(), it->first) == alive_processes.end()) {
				Set_process_alive_flag(it->first,false);
				//cout << "Process " << dec << it->first <<" is no more alive!" << endl;
			}
			else {
				Set_process_alive_flag(it->first,true);
			}
		}
	} else {
		cout << "Failed opening the FIFO, errno: " << errno << endl;
	}
}


void Memory_Profiler::Create_new_analyzer(Analyzer& analyzer){

	Analyzers_vector.push_back(analyzer);
}

void Memory_Profiler::Remove_analyzer(unsigned int analyzer_index){

	if(analyzer_index >= Analyzers_vector.size()){
		cout << "Wrong analyzer index" << endl;
	}
	else{
		Analyzers_vector.erase(Analyzers_vector.begin() + analyzer_index);
	}
}

void Memory_Profiler::Create_new_size_filter_cli(unsigned long size_p, string operation_p){

	try{
		Create_new_filter (*(new Size_filter(size_p,operation_p)));
	}
	catch(const bool v){
		if( v == false){
			cout << "Filter not added: bad operation type." << endl;
			cout << "Possibilities: equal, less, bigger" << endl;
		}
	}
}

void Memory_Profiler::Create_new_filter(Filter& filter){

	Filters_vector.push_back(template_handler<Filter>(filter));
}

void Memory_Profiler::Create_new_pattern(string name){

	vector< template_handler<Pattern> >::iterator pattern = Find_pattern_by_name(name);

	if(pattern != Patterns_vector.end()){
		cout << "Pattern with that name already exists!" << endl;
	}
	else{
		Patterns_vector.push_back( template_handler<Pattern> (*(new Pattern(name))));
	}
}

void Memory_Profiler::Print_patterns() const{

	unsigned int i = 0;
	for(vector< template_handler<Pattern> >::const_iterator pattern = Patterns_vector.begin();pattern != Patterns_vector.end();pattern++){
		cout << endl;
		cout <<"Index: " << dec << i << endl;
		pattern->object->Print();
		cout << endl;
		cout << "Analyzers in the pattern: " << endl;
		pattern->object->Print_analyzers();
		cout << endl;
		cout << "Filters in the pattern: " << endl;
		pattern->object->Print_filters();
		cout << endl << "-----------------------------" << endl;
		i++;
	}
}

void Memory_Profiler::Print_filters() const{

	unsigned int i = 0;
	for(vector< template_handler<Filter> >::const_iterator filter = Filters_vector.begin();filter != Filters_vector.end();filter++){
		cout << endl;
		cout <<"Index: " << dec << i << endl;
		filter->object->Print();
		i++;
	}
}

void Memory_Profiler::Print_analyzers() const{

	unsigned int i = 0;
	for(vector< template_handler<Analyzer> >::const_iterator analyzer = Analyzers_vector.begin();analyzer != Analyzers_vector.end();analyzer++){
		cout << endl;
		cout <<"Index: " << dec << i << endl;
		analyzer->object->Print();
		i++;
	}
}

vector< template_handler<Pattern> >::iterator Memory_Profiler::Find_pattern_by_name(string Pattern_name){

	return find(Patterns_vector.begin(),Patterns_vector.end(),Pattern_name);

}

void Memory_Profiler::Add_analyzer_to_pattern(unsigned int analyzer_index,unsigned int pattern_index){

	if(analyzer_index >= Analyzers_vector.size()){
		cout << "Wrong Analyzer ID" << endl;
	}
	else if (pattern_index >= Patterns_vector.size()){
		cout << "Wrong Pattern ID" << endl;
	}
	else{
		Patterns_vector[pattern_index].object->Analyzer_register(Analyzers_vector[analyzer_index]);
		Analyzers_vector[analyzer_index].object->Pattern_register(Patterns_vector[pattern_index]);
	}
}

void Memory_Profiler::Remove_analyzer_from_pattern(unsigned int analyzer_index,unsigned int pattern_index){

	if (pattern_index >= Patterns_vector.size()){
			cout << "Wrong Pattern ID" << endl;
	}
	else if(analyzer_index >= Patterns_vector[pattern_index].object->Get_number_of_analyzers()){
		cout << "Wrong Analyzer ID" << endl;
	}
	else{
		Patterns_vector[pattern_index].object->Analyzer_deregister(analyzer_index);
	}
}

void Memory_Profiler::Add_analyzer_to_pattern_by_name(unsigned int analyzer_index,string pattern_name){

	vector<template_handler<Pattern> >::iterator pattern = Find_pattern_by_name(pattern_name);

	if (pattern == Patterns_vector.end()){
		cout << "Wrong pattern name!" << endl;
	}
	else if(analyzer_index >= Analyzers_vector.size()){
		cout << "Wrong Analyzer ID" << endl;
	}
	else{
		pattern->object->Analyzer_register(Analyzers_vector[analyzer_index]);
		Analyzers_vector[analyzer_index].object->Pattern_register(*pattern);
	}
}

void Memory_Profiler::Remove_analyzer_from_pattern_by_name(unsigned int analyzer_index,string pattern_name){

	vector< template_handler<Pattern> >::iterator pattern = Find_pattern_by_name(pattern_name);

	pattern->object->Analyzer_deregister(analyzer_index);

}

void Memory_Profiler::Remove_filter(unsigned int filter_index){

	if(filter_index >= Filters_vector.size()){
		cout << "Wrong filter index" << endl;
	}
	else{
		Filters_vector.erase(Filters_vector.begin() + filter_index);
	}
}
void Memory_Profiler::Remove_filter_from_pattern(unsigned int filter_index,unsigned int pattern_index){

	if (pattern_index >= Patterns_vector.size()){
			cout << "Wrong Pattern ID" << endl;
	}
	else if(filter_index >= Patterns_vector[pattern_index].object->Get_number_of_analyzers()){
		cout << "Wrong Filter ID" << endl;
	}
	else{
		Patterns_vector[pattern_index].object->Filter_deregister(filter_index);
	}
}

void Memory_Profiler::Remove_filter_from_pattern_by_name(unsigned int filter_index,string pattern_name){

	vector< template_handler<Pattern> >::iterator pattern = Find_pattern_by_name(pattern_name);

	pattern->object->Filter_deregister(filter_index);
}

void Memory_Profiler::Add_filter_to_pattern(unsigned int filter_index,unsigned int pattern_index){

	if(filter_index >= Filters_vector.size()){
		cout << "Wrong Filter ID" << endl;
	}
	else if (pattern_index >= Patterns_vector.size()){
		cout << "Wrong Pattern ID" << endl;
	}
	else{
		Patterns_vector[pattern_index].object->Filter_register(Filters_vector[filter_index]);
		Filters_vector[filter_index].object->Pattern_register(Patterns_vector[pattern_index]);
	}
}


void Memory_Profiler::Add_filter_to_pattern_by_name(unsigned int filter_index,string pattern_name){

	vector< template_handler<Pattern> >::iterator pattern = Find_pattern_by_name(pattern_name);

	if(filter_index >= Filters_vector.size()){
		cout << "Wrong Filter ID" << endl;
	}
	else if (pattern == Patterns_vector.end()){
		cout << "Wrong Pattern name" << endl;
	}
	else{
		pattern->object->Filter_register(Filters_vector[filter_index]);
		Filters_vector[filter_index].object->Pattern_register(*pattern);
	}
}

void Memory_Profiler::Run_pattern(unsigned int pattern_index, pid_t PID){

	if (pattern_index >= Patterns_vector.size()){
			cout << "Wrong pattern ID" << endl;
	}
	else{
		Patterns_vector[pattern_index].object->Run_analyzers(Processes[PID]);
	}
}

void Memory_Profiler::Run_pattern(string pattern_name, pid_t PID){

	vector< template_handler<Pattern> >::iterator pattern = Find_pattern_by_name(pattern_name);

	if (pattern == Patterns_vector.end()){
			cout << "Wrong pattern name" << endl;
	}
	else{
		pattern->object->Run_analyzers(Processes[PID]);
	}
}

void  Memory_Profiler::Run_pattern_all_process(unsigned int pattern_index){

	if (pattern_index >= Patterns_vector.size()){
		cout << "Wrong pattern ID" << endl;
	}
	else {
		for(map<pid_t const,template_handler<Process_handler> >::iterator process = Processes.begin();process != Processes.end();process++){
			Run_pattern(pattern_index,process->first);
		}
	}
}

void Memory_Profiler::Run_pattern_all_process(string name){

	vector< template_handler<Pattern> >::iterator pattern = Find_pattern_by_name(name);

	if (pattern == Patterns_vector.end()){
		cout << "Wrong Pattern name" << endl;
	}
	else{
		for(map<pid_t const,template_handler<Process_handler> >::iterator process = Processes.begin();process != Processes.end();process++){
			pattern->object->Run_analyzers(process->second);
		}
	}

}

