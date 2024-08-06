#include "full_condition.h"

//example:
//|([5]|(=(status,"high"),<(code,500),=(msg,"aaa")),[8]=(code,700))

// Global pointer to the current sensor on which the condition is conditional
Sensor* currentSensor;

// Global map to keep track of existing conditions to avoid duplication
unordered_map<string, Condition*> existingConditions;

// Initialize a static variable to assign unique IDs to each FullCondition instance
int FullCondition::counter = 0;

// Recursively builds the condition tree from the condition string.
Condition* FullCondition::buildNode(const string& condition, int& index, vector<Sensor*>& sensors, map<int, int> bracketIndexes) {
	if (condition.empty())
		throw "Condition string is empty!";

	// Handling sensor reference
	if (condition[index] == '[') {
		int closeBracket = find(condition.begin() + index, condition.end(), ']') - condition.begin();
		string numStr = condition.substr(index + 1, closeBracket - index - 1);
		int id = stoi(numStr);
		index = closeBracket + 1;
		currentSensor = *find_if(sensors.begin(), sensors.end(), [id](Sensor* s) { return s->id == id; });
	}

	int openBracketIndex = find(condition.begin() + index, condition.end(), '(') - condition.begin();
	// Generates a key for the condition with the current sensor's ID (if exists)
	string key = (currentSensor ? to_string(currentSensor->id) : "") + condition.substr(index, bracketIndexes[openBracketIndex] - index + 1);
	// Check if the key already exists in the existingConditions map
	if (existingConditions.find(key) != existingConditions.end()) {
		index = bracketIndexes[openBracketIndex] + 1;
		if (condition[index] == ',')
			index++;
		return existingConditions.find(key)->second;
	}

	// | , & , ( = , < , > , >= , <= , != )
	// Creating a new node
	Condition* conditionPtr;
	if (condition[index] == '|')
		conditionPtr = new OrOperator;
	else if (condition[index] == '&')
		conditionPtr = new AndOperator;
	else
		conditionPtr = new BasicCondition(this->id);
	existingConditions[key] = conditionPtr;

	if (OperatorNode* operatorNode = dynamic_cast<OperatorNode*>(conditionPtr)) {
		index += 2;
		// Going over the internal conditions and creating children
		while (condition[index] != ')') {
			operatorNode->conditions.push_back(buildNode(condition, index, sensors, bracketIndexes));
			operatorNode->conditions[operatorNode->conditions.size() - 1]->parents.push_back(operatorNode);
		}
	}
	else if (BasicCondition* basicCondition = dynamic_cast<BasicCondition*>(conditionPtr)) {
		// Fill the fields in BasicCondition
		basicCondition->operatorType = condition.substr(index, openBracketIndex - index);
		int commaIndex = find(condition.begin() + index, condition.end(), ',') - condition.begin();
		string name = condition.substr(openBracketIndex + 1, commaIndex - openBracketIndex - 1);
		int closeBracket = bracketIndexes[openBracketIndex];
		basicCondition->value = condition.substr(commaIndex + 1, closeBracket - commaIndex - 1);
		// Add the sensor reference to this leaf
		currentSensor->fields[name].second.push_back(basicCondition);
	}

	index = bracketIndexes[openBracketIndex] + 1;
	if (condition[index] == ',')
		index++;

	return conditionPtr;
}

// Maps the positions of opening bracket indexes to their corresponding closing bracket indexes.
map<int, int> findBrackets(string condition)
{
	map<int, int> mapIndexes;
	stack<int> stackIndexes;
	for (int i = 0; i < condition.size(); i++) {
		if (condition[i] == '(') {
			stackIndexes.push(i);
		}
		else if (condition[i] == ')') {
			mapIndexes[stackIndexes.top()] = i;
			stackIndexes.pop();
		}
	}

	return mapIndexes;
}

// Constructor: Builds the condition tree.
FullCondition::FullCondition(string condition, map<int, string>& actions, vector<Sensor*>& sensors) : actions(actions)
{
	id = FullCondition::counter++;
	map<int, int> bracketsIndexes = findBrackets(condition);
	int index = 0;
	root = this->buildNode(condition, index, sensors, bracketsIndexes);
	existingConditions.clear();
}