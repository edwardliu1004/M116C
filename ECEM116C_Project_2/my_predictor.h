// my_predictor.h
// This file contains a sample my_predictor class.
// It is a simple 32,768-entry gshare with a history length of 15.
// Note that this predictor doesn't use the whole 32 kilobytes available
// for the CBP-2 contest; it is just an example.

//this is implementation of gskewed predictor with 3 separate tables
class my_update : public branch_update {
public:
	//indexes for all 3 tables
	unsigned int index1;
	unsigned int index2;
	unsigned int index3;
};

class my_predictor : public branch_predictor {
public:
#define HISTORY_LENGTH 15
#define TABLE_BITS 15
#define SIZE 32768 //2^15 = 32768

	my_update u;
	branch_info bi;
	unsigned int history;
	unsigned char table1[1 << TABLE_BITS]; //2^15 = 32768 - size of every table
	unsigned char table2[1 << TABLE_BITS];
	unsigned char table3[1 << TABLE_BITS];

	my_predictor(void) : history(0) {
		memset(table1, 0, sizeof(table1));
		memset(table2, 0, sizeof(table2));
		memset(table3, 0, sizeof(table3));
	}

	branch_update* predict(branch_info& b) {
		bi = b;
		if (b.br_flags & BR_CONDITIONAL) {
			//here are different hashing functions I tried and their respective MPKI times
			//
			//6.036
			/*u.index1 = ((b.address & 0x9E3779B9) ^ (history)) & (SIZE - 1);
			u.index2 = ((b.address) ^ (history & 0x9E3779B9)) & (SIZE - 1);
			u.index3 = ((b.address & 0x9E3779B9) ^ (history & 0x7F4A7C15)) & (SIZE - 1);*/

			//5.954
			/*u.index1 = (b.address ^ (history << 1)) & (SIZE - 1);
			u.index2 = (b.address ^ (history >> 1)) & (SIZE - 1);
			u.index3 = ((b.address >> 1) ^ history) & (SIZE - 1);*/

			//5.776
			/*u.index1 = (b.address ^ (history << 2)) & (SIZE - 1);
			u.index2 = (b.address ^ (history >> 3)) & (SIZE - 1);
			u.index3 = ((b.address >> 2) ^ history) & (SIZE - 1);*/

			//5.435
			//hashing function to index
			u.index1 = (((b.address << 3) | (b.address >> 29)) ^ history) & (SIZE - 1);
			u.index2 = (((history << 5) | (history >> 27)) ^ b.address) & (SIZE - 1);
			u.index3 = ((b.address << 1) ^ (history >> 1)) & (SIZE - 1);

			//5.667
			/*u.index1 = ((b.address + (history << 1)) ^ (history >> 2)) & (SIZE - 1);
			u.index2 = ((b.address + (history >> 3)) ^ (b.address << 2)) & (SIZE - 1);
			u.index3 = ((b.address + history) ^ (b.address >> 1)) & (SIZE - 1);*/

			//5.559
			/*u.index1 = (b.address * 31 + (history << 1)) & (SIZE - 1);
			u.index2 = ((history * 37) + (b.address >> 1)) & (SIZE - 1);
			u.index3 = ((b.address * 41) ^ history) & (SIZE - 1);*/

			//5.813
			/*u.index1 = ((b.address & 0xAAAA) ^ (history & 0x5555)) & (SIZE - 1);
			u.index2 = ((b.address & 0x5555) ^ (history & 0xAAAA)) & (SIZE - 1);
			u.index3 = ((b.address & 0x0F0F) ^ (history & 0xF0F0)) & (SIZE - 1);*/

			//5.926
			/*u.index1 = ((b.address ^ history) * (b.address ^ history)) >> ((32 - TABLE_BITS) / 2) & (SIZE - 1);
			u.index2 = (((b.address >> 1) ^ history) * ((b.address >> 1) ^ history)) >> ((32 - TABLE_BITS) / 2) & (SIZE - 1);
			u.index3 = ((b.address ^ (history << 1)) * (b.address ^ (history << 1))) >> ((32 - TABLE_BITS) / 2) & (SIZE - 1);*/

			//gather predictions from the 3 tables
			bool pred1 = (table1[u.index1] >> 1);
			bool pred2 = (table2[u.index2] >> 1);
			bool pred3 = (table3[u.index3] >> 1);

			//combine every table's vote
			int vote = pred1 + pred2 + pred3;

			//if 2 or more voted taken, prediction is to take the branch
			u.direction_prediction(vote >= 2);
		}
		else {
			//non conditional branches are always taken
			u.direction_prediction(true);
		}
		return &u;
	}

	void update(branch_update* u, bool taken, unsigned int target) {
		if (bi.br_flags & BR_CONDITIONAL) {
			//retrieve pointers
			unsigned char* a = &table1[((my_update*)u)->index1];
			unsigned char* b = &table2[((my_update*)u)->index2];
			unsigned char* c = &table3[((my_update*)u)->index3];

			//update table given actual outcome 
			if (taken) {
				if (*a < 3) (*a)++;
				if (*b < 3) (*b)++;
				if (*c < 3) (*c)++;
			}
			else {
				if (*a > 0) (*a)--;
				if (*b > 0) (*b)--;
				if (*c > 0) (*c)--;
			}

			//update history 
			history = ((1 << HISTORY_LENGTH) - 1) & ((history << 1) | taken);

		}
	}

};
