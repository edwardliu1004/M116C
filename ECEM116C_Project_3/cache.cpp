#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>

using namespace std;

struct CacheLine {
	char state; //MOESIF
	int lru; //for lru replacement policy
	int tag; //address
	bool dirty;
	CacheLine() : state('I'), lru(0), tag(0), dirty(false) {}
};

class Cache {
public: 
	vector<CacheLine> lines;
	Cache() : lines(4) {
		for (int i = 0; i < 4; i++) {
			//initalize cache lins with lru value
			lines[i].lru = i;
		}
	}

	//returns which line has tag
	int findLine(int tag) {
		for (int i = 0; i < 4; i++) {
			//tag is there and line isn't invalid
			if (lines[i].tag == tag && lines[i].state != 'I') {
				return i;
			}
		}
		//line not found
		return -1;
	}

	//returns which line is least recently used
	int findLRU() {
		//find invalid line
		for (int i = 0; i < 4; i++) {
			if (lines[i].state == 'I')
				return i;
		}
		//find least recently used 
		for (int i = 0; i < 4; i++) {
			if (lines[i].lru == 0)
				return i;
		}
		return -1;
	}

	//make accessedIndex line most recently used
	void updateLRU(int accessedIndex) {
		int currentLRU = lines[accessedIndex].lru;
		for (int i = 0; i < 4; i++) {
			//all other lru values are decremented
			if (lines[i].lru > currentLRU) lines[i].lru--;
		}
		//set as most recently used
		lines[accessedIndex].lru = 3;
	}
};

//returns index of which cache also has same tag and isn't in invalid state
int findLineInOtherCaches(vector<Cache>& caches, int requestingCore, int tag) {
	for (int c = 0; c < 4; c++) {
		if (c != requestingCore) {
			for (int i = 0; i < 4; i++) {
				if (caches[c].lines[i].tag == tag && caches[c].lines[i].state != 'I') {
					return c;
				}
			}
		}
	}
	return -1;
}

//returns how many have same tag
int countSharers(vector<Cache>& caches, int tag) {
	int count = 0;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (caches[i].lines[j].tag == tag && caches[i].lines[j].state != 'I') {
				count++;
				break;
			}
		}
	}
	return count;
}

//returns vector of all caches with same tag
vector<int> findAllSharers(vector<Cache>& caches, int tag) {
	vector<int> sharers;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (caches[i].lines[j].tag == tag && caches[i].lines[j].state != 'I') {
				sharers.push_back(i);
				break;
			}
		}
	}
	return sharers;
}

//update states of lines in other caches based on requesting operation
void onBroadcast(vector<Cache>& caches, int requester, int tag, bool isWrite, int& writebacks) {
	vector<int> sharers = findAllSharers(caches, tag);
	if (sharers.empty()) return; //no one has data
	
	//locate line within supplier cache that holds requested data
	int supplier = sharers[0];
	int supplierIndex = -1;
	for (int i = 0; i < 4; i++) {
		if (caches[supplier].lines[i].tag == tag && caches[supplier].lines[i].state != 'I') {
			supplierIndex = i;
			break;
		}
	}
	//obtain state
	char sState = caches[supplier].lines[supplierIndex].state;

	//read miss
	if (!isWrite) {
		if (sState == 'M') {
			//moves to 'O' while remaining dirty
			caches[supplier].lines[supplierIndex].state = 'O';
			caches[supplier].lines[supplierIndex].dirty = true;
		}
		else if (sState == 'E') {
			// E with a new sharer -> F
			caches[supplier].lines[supplierIndex].state = 'F';
			caches[supplier].lines[supplierIndex].dirty = false;
		}
	}
	else {
	//write miss
		for (int c : sharers) {
			//invalidate all other sharers
			for (int i = 0; i < 4; i++) {
				if (caches[c].lines[i].tag == tag && caches[c].lines[i].state != 'I') {
					if (c != requester) {
						if (caches[c].lines[i].dirty) {
							writebacks++;
						}
						caches[c].lines[i].state = 'I';
						caches[c].lines[i].dirty = false;
					}
				}
			}
		}
	}
}

void installLine(Cache& cache, int tag, bool isWrite, vector<Cache>& caches, int requester, int& writebacks) {
	int idx = cache.findLRU();
	//kick out if full
	if (cache.lines[idx].state != 'I') {
		if (cache.lines[idx].dirty) writebacks++;
	}

	cache.lines[idx].tag = tag;
	cache.lines[idx].dirty = false;

	int sharersCount = countSharers(caches, tag);
	if (isWrite) {
		//write miss
		cache.lines[idx].state = 'M';
		cache.lines[idx].dirty = true;
	}
	else {
		//read miss
		if (sharersCount == 0) {
			cache.lines[idx].state = 'E';
			cache.lines[idx].dirty = false;
		}
		else {
			cache.lines[idx].state = 'S';
			cache.lines[idx].dirty = false;
		}
	}
	cache.updateLRU(idx);
}

int main(int argc, char* argv[]) {
	//check correct number of arguments
	if (argc != 2) {
		cerr << "Usage: ./coherentsim <inputfile.txt>" << endl;
		return 1;
	}
	//open input file
	ifstream infile(argv[1]);
	//check file
	if (!infile.is_open()) {
		cerr << "Error: Could not open input file." << endl;
		return 1;
	}

	//one cache per core
	vector<Cache> caches(4);
	unordered_map<string, int> coreMap = { {"P0:", 0}, {"P1:", 1}, {"P2:", 2}, {"P3:", 3} };

	int cacheHits = 0, cacheMisses = 0, writebacks = 0, broadcasts = 0, cacheToCacheTransfers = 0;
	string line;

	while (getline(infile, line)) {
		string core, operation;
		int tag;
		//parse the input line
		istringstream ss(line);
		string temp;
		//read for example P2 into core and "read<100>" into temp
		ss >> core >> operation >> temp;
		//operation = temp.substr(0, temp.find('<'));
		tag = stoi(temp.substr(temp.find('<') + 1, temp.find('>') - temp.find('<') - 1));
		int coreID = coreMap[core];

		Cache& cache = caches[coreID];
		int lineIndex = cache.findLine(tag);
		bool isWrite = (operation == "write");
		bool hit = (lineIndex != -1);
		bool needBroadcast = true;

		if (!hit) {
			//miss
			cacheMisses++;
			if (needBroadcast) broadcasts++;
			int supplier = findLineInOtherCaches(caches, coreID, tag);
			onBroadcast(caches, coreID, tag, isWrite, writebacks);
			//check if other caches still hold line
			int sharersAfter = countSharers(caches, tag);

			//there is a supplier and still a sharer after broadcast
			if (supplier != -1 && sharersAfter > 0) cacheToCacheTransfers++;
			
			installLine(cache, tag, isWrite, caches, coreID, writebacks);
		}
		else {
			//hit
			char state = cache.lines[lineIndex].state;
			//check if exclusive, don't need to broadcast
			if (state == 'E') needBroadcast = false;
			cacheHits++;
			cache.updateLRU(lineIndex);

			if (needBroadcast) {
				broadcasts++;
				if (isWrite) {
					if (state == 'S' || state == 'F' || state == 'O') {
						onBroadcast(caches, coreID, tag, true, writebacks);
						cache.lines[lineIndex].state = 'M';
						cache.lines[lineIndex].dirty = true;
					}
				}
			}
			else {
				//no need for broadcast
				if (isWrite && state == 'E') {
					//upgrade from E to M
					cache.lines[lineIndex].state = 'M';
					cache.lines[lineIndex].dirty = true;
				}
			}
		}
	}

	//output stats
	cout << cacheHits << endl;
	cout << cacheMisses << endl;
	cout << writebacks << endl;
	cout << broadcasts << endl;
	cout << cacheToCacheTransfers;

	return 0;
}