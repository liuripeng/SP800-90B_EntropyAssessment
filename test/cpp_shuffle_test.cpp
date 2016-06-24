#include <stdio.h>
#include <stdlib.h>
#include <iostream>		// std::cout
#include <string>		// std::string
#include <time.h>		// time
#include <algorithm>	// std::sort
#include <map>			// std::map
#include <vector>		// std::vector
#include <set>			// std::set
#include <string.h>		// strlen
#include <iomanip>		// setw / setfill
#include <math.h>		// pow

#include "bzlib.h" // sudo apt-get install libbz2-dev

#define SIZE 1000000
#define PERMS 10

using namespace std;

typedef unsigned char byte;

// Global variables

// Actual data read from file
byte data[SIZE];

// Baseline statistics
double mean = 0.0;
double median = 0.0;
bool is_binary = false;

// Counters for the pass/fail of each statistic
map<int, int*> C;

// Original test results (t) and permuted test results (t')
map<string, long double> t, tp;

// The tests used
const int num_tests = 19;
const string test_names[] = {"excursion","numDirectionalRuns","lenDirectionalRuns","numIncreasesDecreases","numRunsMedian","lenRunsMedian","avgCollision","maxCollision","periodicity(1)","periodicity(2)","periodicity(8)","periodicity(16)","periodicity(32)","covariance(1)","covariance(2)","covariance(8)","covariance(16)","covariance(32)","compression"};

// Read in binary file to test
void read_file(const char* file_path){
	FILE* file = NULL;

	#ifdef VERBOSE
	printf("Opening file %s\n", file_path);
	#endif

	file = fopen(file_path, "rb");
	fread(data, 1, SIZE, file);
	fclose(file);

	#ifdef VERBOSE
	printf("Data read\n");
	#endif
}

// Fisher-Yates Fast (in place) shuffle algorithm
void shuffle(byte arr[]){
	srand(time(NULL));
	long int r;

	for(long int i = SIZE-1; i > 0; i--){
		r = rand() % (i+1);
		swap(arr[r], arr[i]);
	}
}

// Quick sum array
long int sum(byte arr[]){
	long int sum = 0;
	for(long int i = 0; i < SIZE; i++){
		sum += arr[i];
	}

	return sum;
}

// Quick sum vector
long int sum(vector<int> v){
	long int sum = 0;
	for(long int i = 0; i < v.size(); i++){
		sum += v[i];
	}

	return sum;
}

// Calculate baseline statistics
// Finds mean, median, and whether or not the data is binary
void calc_stats(){

	// Calculate mean
	mean = sum(data) / (long double) SIZE;

	// Sort in a vector for median/min/max
	vector<byte> v(data, data+SIZE);
	sort(v.begin(), v.end());

	byte min = v[0];
	byte max = v[SIZE-1];

	if(min == 0 && max == 1){
		is_binary = true;
		median = 0.5;
	}else{
		long int half = SIZE / 2;
		median = (v[half] + v[half-1]) / 2.0;
	}
}

// 5.1 Conversion I
// Takes a binary sequence and partitions it into 8-bit blocks
// Blocks have the number of 1's counted and totaled	
vector<int> conversion1(){
	vector<int> ret(SIZE/8, 0);

	for(long int i = 0; i < SIZE; i+=8){
		for(int j = 0; j < 8; j++){
			ret[i/8] += data[i+j];
		}
	}

	return ret;
}

// 5.1 Conversion II
// Takes a binary sequence and partitions it into 8-bit blocks
// Blocks are then converted to decimal
vector<int> conversion2(){
	vector<int> ret(SIZE/8, 0);

	for(int i = 0; i < SIZE; i+=8){
		for(int j = 0; j < 8; j++){
			ret[i/8] += data[i+j] * pow(2, (7-j));
		}
	}

	return ret;
}

// 5.1.1 Excursion Test
// Measures how far the running sum of values deviates from the 
// average value at each point in the set
double excursion(){
	double d_i = 0;
	double max = 0;
	double running_sum = 0;

	for(long int i = 0; i < SIZE; i++){
		running_sum += data[i];
		d_i = running_sum - ((i+1) * mean);

		if(d_i > max){
			max = d_i;
		}else if(-1 * d_i > max){
			max = -1 * d_i;
		}
	}

	return max;
}

// Helper for 5.1.2, 5.1.3, and 5.1.4
// Builds a vector of the runs of consecutive values
// Pushes +1 to the vector if the value is greater than (or equal to) the previous
// Pushes -1 to the vector if the value is less than the previous
vector<byte> alt_sequence1(){
	vector<byte> ret(SIZE-1, 0);

	for(long int i = 0; i < SIZE-1; i++){
		ret[i] = ((data[i] > data[i+1]) ? -1 : 1);
	}

	return ret;
}

// Helper for 5.1.5 and 5.1.6
// Builds a vector of the runs of values compared to the median
// Pushes +1 to the vector if the value is greater than (or equal to) the median
// Pushes -1 to the vector if the value is less than the median
vector<byte> alt_sequence2(){
	vector<byte> ret(SIZE, 0);

	for(long int i = 0; i < SIZE; i++){
		ret[i] = ((data[i] < median) ? -1 : 1);
	}

	return ret;
}

// 5.1.2 Number of Directional Runs
// Determines the number of runs in the sequence. 
// A run is when multiple consecutive values are all >= the prior
// or all < the prior
long int num_directional_runs(vector<byte> alt_seq){
	long int num_runs = 0;
	for(long int i = 1; i < SIZE-1; i++){
		if(alt_seq[i] != alt_seq[i-1]){
			num_runs++;
		}
	}

	return num_runs;
}

// 5.1.3 Length of Directional Runs
// Determines the length of the longest run
long int len_directional_runs(vector<byte> alt_seq){
	long int max_run = 0;
	long int run = 1;

	for(long int i = 1; i < SIZE-1; i++){
		if(alt_seq[i] == alt_seq[i-1]){
			run++;
		}else{
			if(run > max_run){
				max_run = run;
			}
			run = 1;
		}
	}

	// Handle last run
	if(run > max_run){
		max_run = run;
	}

	return max_run;
}

// 5.1.4 Number of Increases and Decreases
// Determines the maximum number of increases or decreases between
// consecutive values
long int num_increases_decreases(vector<byte> alt_seq){
	long int pos = 0;
	for(long int i = 0; i < SIZE-1; i++){
		if(alt_seq[i] == 1) pos++;
	}

	return max(pos, SIZE-pos);
}

// 5.1.5 Number of Runs Based on the Median
// Determines the number of runs that are constructed with respect
// to the median of the dataset
// This is similar to a normal run, but instead of being compared
// to the previous value, each value is compared to the median
long int num_runs_median(vector<byte> alt_seq){
	long int num_runs = 1;

	for(long int i = 1; i < SIZE; i++){
		if(alt_seq[i] != alt_seq[i-1]){
			num_runs++;
		}
	}

	return num_runs;
}

// 5.1.6 Length of Runs Based on the Median
// Determines the length of the longest run that is constructed
// with respect to the median
long int len_runs_median(vector<byte> alt_seq){
	long int max_run = 0;
	long int run = 1;

	for(long int i = 1; i < SIZE; i++){
		if(alt_seq[i] == alt_seq[i-1]){
			run++;
		}else{
			if(run > max_run){
				max_run = run;
			}
			run = 1;
		}
	}

	// Handle the last run
	if(run > max_run){
		max_run = run;
	}

	return max_run;
}

// Helper function to prepare for 5.1.7 and 5.1.8
// Should take about 2-3 seconds for a 1mil size list
// Maybe speed up by randomizing the check_size progression
// based on the average amount until the next collision (about 16-19)
// Just advance maybe 4-5 per iteration and back up if collisions are found based on how many collision are found.
// POC: https://repl.it/C4eI
vector<int> find_collisions(){
	vector<int> ret;
	set<int> dups;

	long int i = 0;
	long int check_size;

	// Begin from each element
	while(i < SIZE){

		check_size = 0;

		// Progressively increase the number of elements checked
		while(check_size < (SIZE - i)){

			// Toss elements into a set
			dups.insert(data[i+check_size]);

			// If sizes don't match up then a collision exists
			if(dups.size() != check_size+1){

				// Record info on collision and end inner loop
				// Advance outer loop past the collision end
				ret.push_back(check_size+1);
				i += check_size;
				check_size = SIZE;
				dups.clear();
			}

			check_size++;
		}

		i++;
	}

	return ret;
}

// 5.1.7 Average Collision Test
// Counts the number of successive samples until a duplicate is found
double avg_collision(vector<int> col_seq){

	return sum(col_seq)/float(col_seq.size());
}

// 5.1.8 Maximum Collision Test
// Determines the maximum number of samples without a duplicate
long int max_collision(vector<int> col_seq){
	long int max = -1;
	for(long int i = 0; i < col_seq.size(); i++){
		if(max < col_seq[i]) max = col_seq[i];
	}

	return max;
}

// 5.1.9 Periodicity Test
// Determines the number of periodic structures
// Based on lag parameter p
long int periodicity(int p){
	long int T = 0;

	for(long int i = 0; i < SIZE-p; i++){
		if(data[i] == data[i+p]){
			T++;
		}
	}

	return T;
}

// 5.1.10 Covariance Test
// Measures the strength of lagged correlation
// Based on lag parameter p
long int covariance(int p){
	long int T = 0;

	for(long int i = 0; i < SIZE-p; i++){
		T += data[i] * data[i+p];
	}

	return T;
}

// 5.1.11 Compression Test
// Compresses the data using bzip2 and determines the length
// of the resulting compressed data
unsigned int compression(){

	// Build string of bytes
	string msg = "";
	for(long int i = 0; i < SIZE; i++){
		msg += to_string((int)data[i]);		// Dependent on C++11
		msg += " ";
	}

	char* source = (char*)msg.c_str();
	unsigned int dest_len = 2*SIZE;
	char* dest = new char[dest_len];

	int rc = BZ2_bzBuffToBuffCompress(dest, &dest_len, source, strlen(source), 5, 0, 0);

	delete[](dest);

	if(rc == BZ_OK){
		return dest_len;
	}else{
		return 0;
	}
}

// Helper that runs all the mentioned tests
void run_tests(map<string, long double> &stats){
	
	// Conversions for binary data
	vector<int> cs1, cs2;
	if(is_binary){
		cs1 = conversion1();
		cs2 = conversion2();
	}

	// Excursion test
	stats["excursion"] = excursion();

	// Directional tests
	// Prepare alternating sequence vector
	vector<byte> alt_seq;
	if(is_binary){
		alt_seq = alt_sequence1();
	}else{
		alt_seq = alt_sequence1();
	}

	// Run directional tests
	stats["numDirectionalRuns"] = num_directional_runs(alt_seq);
	stats["lenDirectionalRuns"] = len_directional_runs(alt_seq);
	stats["numIncreasesDecreases"] = num_increases_decreases(alt_seq);

	// Consecutive runs tests
	// Prepare alternating sequence vector
	if(is_binary){
		alt_seq = alt_sequence2();
	}else{
		alt_seq = alt_sequence2();
	}

	// Run consecutive runs tests
	stats["numRunsMedian"] = num_runs_median(alt_seq);
	stats["lenRunsMedian"] = len_runs_median(alt_seq);

	// Collision tests
	// Prepare collision sequence vector
	vector<int> col_seq;
	if(is_binary){
		col_seq = find_collisions();
	}else{
		col_seq = find_collisions();
	}

	// Run collision tests
	stats["avgCollision"] = avg_collision(col_seq);
	stats["maxCollision"] = max_collision(col_seq);

	// Periodicity tests
	if(is_binary){
		stats["periodicity(1)"] = periodicity(1);
		stats["periodicity(2)"] = periodicity(2);
		stats["periodicity(8)"] = periodicity(8);
		stats["periodicity(16)"] = periodicity(16);
		stats["periodicity(32)"] = periodicity(32);		
	}else{
		stats["periodicity(1)"] = periodicity(1);
		stats["periodicity(2)"] = periodicity(2);
		stats["periodicity(8)"] = periodicity(8);
		stats["periodicity(16)"] = periodicity(16);
		stats["periodicity(32)"] = periodicity(32);
	}

	// Covariance tests
	if(is_binary){
		stats["covariance(1)"] = covariance(1);
		stats["covariance(2)"] = covariance(2);
		stats["covariance(8)"] = covariance(8);
		stats["covariance(16)"] = covariance(16);
		stats["covariance(32)"] = covariance(32);
	}else{
		stats["covariance(1)"] = covariance(1);
		stats["covariance(2)"] = covariance(2);
		stats["covariance(8)"] = covariance(8);
		stats["covariance(16)"] = covariance(16);
		stats["covariance(32)"] = covariance(32);
	}

	// Compression test
	stats["compression"] = compression();
}

int main(){

	// Read in file
	const char* file_path = "../bin/beacon_rand.bin";
	read_file(file_path);

	cout << sizeof(data) << endl;

	// Calculate baseline statistics
	cout << "Calculating baseline statistics..." << endl;
	calc_stats();

#ifdef VERBOSE
	cout << "Mean: " << mean << endl;
	cout << "Median: " << median << endl;
	cout << "Binary: " << (is_binary ? "true" : "false") << endl;
#endif

	// Build map of results
	for(int i = 0; i < num_tests; i++){
		C[i] = new int[2];
		C[i][0] = 0;
		C[i][1] = 0;

		t[test_names[i]] = -1;
		tp[test_names[i]] = -1;
	}

	// Run initial tests
	cout << "Beginning initial tests..." << endl;
	run_tests(t);

#ifdef VERBOSE
	cout << endl << "Initial test results" << endl;
	for(int i = 0; i < num_tests; i++){
		cout << setw(23) << test_names[i] << ": ";
		cout << t[test_names[i]] << endl;
	}

	cout << endl;
#endif

	// Permutation tests
	cout << "Beginning permutation tests..." << endl;
	for(int i = 0; i < PERMS; i++){

		// Permute the data
		shuffle(data);

		// Run the tests
		run_tests(tp);

		// Aggregate results into the counters
		for(int j = 0; j < num_tests; j++){
			if(tp[test_names[j]] > t[test_names[j]]){
				C[j][0]++;
			}else if(tp[test_names[j]] == tp[test_names[j]]){
				C[j][1]++;
			}
		}
	}

#ifdef VERBOSE
	cout << endl;
	cout << "                statistic  C[i][0]  C[i][1]" << endl;
	cout << "-------------------------------------------" << endl;
	for(int i = 0; i < num_tests; i++){
		if((C[i][0] + C[i][1] <= 5) || C[i][0] >= PERMS-5){
			cout << setw(24) << test_names[i] << "*";
		}else{
			cout << setw(25) << test_names[i];
		}
		cout << setw(8) << C[i][0];
		cout << setw(8) << C[i][1] << endl;
	}

	cout << "(* denotes failed test)" << endl;
#endif

	// for(int i = 0; i < num_tests; i++){
	// 	if((C[i][0] + C[i][1] <= 5) || C[i][0] >= PERMS-5){
	// 		exit(-1);
	// 	}
	// }

	cout << endl;

	// Free memory
	for(int i = 0; i < num_tests; i++){
		delete[](C[i]);
	}

	return 0;
}