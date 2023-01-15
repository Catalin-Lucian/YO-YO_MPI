#include <iostream>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <string>
#include <list>
#include "mpi.h"

using namespace std;

enum NodeType
{
	NO_TYPE,
	SOURCE,
	INTERNAL_NODE,
	SINK,
	PRUNDED
};

enum Decision {
	YES = 0, 
	NO = 1, 
	PRUNE = 2
};

vector<vector<int>>  neighbour_ids = {
	/* 0 */		{6, 8, 9},
	/* 1 */		{2, 7, 10},
	/* 2 */		{1, 3, 14},
	/* 3 */		{2, 15},
	/* 4 */		{6, 7, 11},
	/* 5 */		{9 ,12},
	/* 6 */		{0, 4, 8},
	/* 7 */		{1, 4, 11},
	/* 8 */		{0, 6},
	/* 9 */		{0, 5, 12},
	/* 10 */	{1, 14, 15},
	/* 11 */	{4, 7, 13, 16},
	/* 12 */	{5, 9},
	/* 13 */	{11, 17, 18},
	/* 14 */	{2, 10, 15},
	/* 15 */	{3, 10, 14},
	/* 16 */	{11, 17, 18},
	/* 17 */	{13, 16},
	/* 18 */	{13, 16},
};

int global_rank;

Decision reciveVotes(
	list<int> &childs, list<int> &finalChilds,
	list<int> &parents) {

	MPI_Status status; 
	int message;
	Decision finalResponse = YES;

	list<int> childsCopy = childs;
	for (int child_id : childsCopy) {
	
		MPI_Recv(&message, 1, MPI_INT, child_id, 1, MPI_COMM_WORLD, &status);

		if (message == PRUNE) {
			finalChilds.push_back(child_id);
			childs.remove(child_id);
		}
		else if (message == NO) {
			childs.remove(child_id);
			parents.push_back(child_id);

			finalResponse = NO;
		}
	}

	return finalResponse;
}

int reciveValues(list<int>& parents, list<int> &minSenders, list<int> &nonMinSenders) {
	int minValue = 999999;
	MPI_Status status;
	int recvValue;

	for (int parent : parents) {
		
		MPI_Recv(&recvValue, 1, MPI_INT, parent, 1, MPI_COMM_WORLD, &status);

		if (recvValue < minValue) {
			minValue = recvValue;

			nonMinSenders.merge(minSenders);

			minSenders.clear();
			minSenders.push_back(parent);
		}
		else if (recvValue == minValue) {
			minSenders.push_back(parent);
		}
		else {
			nonMinSenders.push_back(parent);
		}
	}

	return minValue;
}

void sendMinValue(list<int> childs, int minValue) {
	for (int child_id : childs) {
		MPI_Send(&minValue, 1, MPI_INT, child_id, 1, MPI_COMM_WORLD);
	}
}

void sendDecision(
	list<int> &minSenders, list<int> &nonMinSenders,
	list<int> &parents, list<int> &finalParents,
	list<int> &childs, Decision finalDecision, 
	NodeType &myType
) {

	int vote = PRUNE;
	// check for 1 parent to prune
	// check for only mins to prune
	if ( myType == SINK && (parents.size() == 1 || nonMinSenders.empty())) {
		for (int sender : minSenders) {
			MPI_Send(&vote, 1, MPI_INT, sender, 1, MPI_COMM_WORLD);

			finalParents.push_back(sender);
			parents.remove(sender);
		}
		myType = PRUNDED;
		return;
	}

	vote = NO;
	for (auto sender : nonMinSenders) {
		int vote = NO;
		MPI_Send(&vote, 1, MPI_INT, sender, 1, MPI_COMM_WORLD);

		parents.remove(sender);
		childs.push_back(sender);
	}

	if (finalDecision == NO) {
		for (auto sender : minSenders) {
			MPI_Send(&vote, 1, MPI_INT, sender, 1, MPI_COMM_WORLD);

			parents.remove(sender);
			childs.push_back(sender);
		}
	}
	else if (finalDecision == YES) {
		// send firt min YES and others PRUNE
		int first = minSenders.front();
		vote = YES;
		MPI_Send(&vote, 1, MPI_INT, first, 1, MPI_COMM_WORLD);
		minSenders.remove(first);

		vote = PRUNE;
		for (auto sender : minSenders) {
			MPI_Send(&vote, 1, MPI_INT, sender, 1, MPI_COMM_WORLD);

			parents.remove(sender);
			finalParents.push_back(sender);
		}
	}
}


int main(int argc, char* argv[]) {
	int my_rank;		// rank of process
	int p;				// number of process
	MPI_Status status;	// return status for receive 

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &p);

	list<int> childs = {};
	list<int> parents = {};

	// used to save links after pruning to send the win messsage
	list<int> finalChilds = {};
	list<int> finalParents = {};

	NodeType my_type = NO_TYPE;

	global_rank = my_rank;

	// ============================= Setup =============================
	// exchange id and decide parents and childs
	for (int neighbour_id : neighbour_ids[my_rank]) {

		int recv_value;
		MPI_Sendrecv(&my_rank, 1, MPI_INT, neighbour_id, 1, &recv_value, 1, MPI_INT, neighbour_id, 1, MPI_COMM_WORLD, &status);

		if (recv_value > my_rank)
			childs.push_back(neighbour_id);
		else
			parents.push_back(neighbour_id);
	}

	// ============================= YO-YO ============================= //
	while (my_type != PRUNDED)
	{
		// decide my_type
		if (childs.empty())
			my_type = SINK;
		else if (parents.empty())
			my_type = SOURCE;
		else
			my_type = INTERNAL_NODE;

		
		if (my_type == SOURCE) {

			// send id 
			sendMinValue(childs, my_rank);

			// recive votes
			reciveVotes(childs, finalChilds, parents);

			// check for win condition and send result
			if (childs.empty() && parents.empty()) {
				for (int finalChild : finalChilds) {
					MPI_Send(&my_rank, 1, MPI_INT, finalChild, 1, MPI_COMM_WORLD);
				}
				cout << my_rank << " : I AM THE WINNER" << endl;
				break;
			}
					

		}
		else if (my_type == INTERNAL_NODE) {
			int minValue;
			list<int> minSenders = {};
			list<int> nonMinSenders = {};

			// revice values and get min
			minValue = reciveValues(parents, minSenders, nonMinSenders);

			// send min values to childs
			sendMinValue(childs, minValue);

			// recive votes
			Decision finalDecision = reciveVotes(childs, finalChilds, parents);

			// send decision to parents
			sendDecision(minSenders, nonMinSenders, parents, finalParents, childs, finalDecision, my_type);
		}
		else if (my_type == SINK){
			int minValue;
			list<int> minSenders = {};
			list<int> nonMinSenders = {};

			// recive values and get min value
			minValue = reciveValues(parents, minSenders, nonMinSenders);

			// send decision to parents
			sendDecision(minSenders, nonMinSenders, parents, finalParents, childs, YES, my_type);
		}
	}
		
	if (my_type == PRUNDED) {
		int winner = -1;
		for (int parent : finalParents) {
			MPI_Recv(&winner, 1, MPI_INT, parent, 1, MPI_COMM_WORLD, &status);
		}

		cout << my_rank << " : winner is " << winner << endl;

		for (int child : finalChilds) {
			MPI_Send(&winner, 1, MPI_INT, child, 1, MPI_COMM_WORLD);
		}
	}

	MPI_Finalize();
	return 0;
}