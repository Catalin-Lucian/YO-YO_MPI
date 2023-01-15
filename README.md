# Yo-yo 

A yo-yo implementation using MPI 

# RESUME

This is a C++ program that uses the MPI library for distributed computing to simulate a network of nodes. The network is defined in the neighbour_ids vector and each element of the vector represents a node and its corresponding neighbours. The code contains a number of functions that are used to simulate the node's behavior in the network. The functions include: reciveVotes, reciveValues, sendMinValue, and sendDecision. The reciveVotes function receives votes from child nodes and updates the parent and final child lists based on the received votes. The reciveValues function receives values from parent nodes, finds the minimum value, and updates the list of minimum value senders and non-minimum value senders. The sendMinValue function sends the minimum value to child nodes. The sendDecision function sends a decision (YES, NO, or PRUNE) to the parent nodes based on the received values and the final decision made by the node. Additionally, the function updates the parent, final parent, and child lists based on the sent decision.
