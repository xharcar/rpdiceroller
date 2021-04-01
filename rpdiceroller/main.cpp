#include <iostream>
#include <limits>
#include <vector>
#include <random>
#include <algorithm>
#include <cstdint>
#include <chrono>
#include <cctype>
#include <map>

static const std::array<uint64_t,8> commonRolls = {2,4,6,8,10,12,20,100};

enum AdvantageFactor{
	NONE,
	ADVANTAGE,
	DISADVANTAGE
};

enum DiscardMode{
	KEEP_ALL,
	KEEP_HIGH,
	KEEP_LOW
};

enum ParseResult{
	RESEED = -2,
	QUIT = -1,
	OK = 0,
	MALFORMED_COMMAND,
	UNPARSABLE_NUMBER,
};

typedef struct{
	size_t dieCount = 1;
	size_t dieSides = 20;
	char sign = '+';
	size_t diceToKeep = std::numeric_limits<size_t>::max();
	DiscardMode discardMode = DiscardMode::KEEP_ALL;
}SingleRoll;


typedef struct{
	std::vector<SingleRoll> rolls;
	size_t repeats = 1;
	int modifierAfterDice = 0;
	AdvantageFactor advantageFactor = AdvantageFactor::NONE;
}RollInfo;


/**
 * @brief initializeRng Initializes a new std::mt19937_64 instance;
 * split out from main to allow for reseeding if someone says it's rigged
 * command implementation TBD
 * @return 64-bit mersenne twister instance seeded with(by default) the count of
 * nanoseconds since epoch at time of creation, give or take a jiffy
 */
std::mt19937_64 initializeRng(unsigned long long seed = 0);

/**
 * @brief parseRollOrMod Helper function to parse either a single roll
 * ( _a_ dice of _b_ sides each, optionally keeping _k_ top/bottom)
 * or a post-dice modifier
 * @param roll string from which a SingleRoll instance will be acquired
 * @param rollInfo ref to RollInfo instance where the SingleRoll will be stored
 * @return whether parsing was successful
 */
ParseResult parseRollOrMod(std::string roll, RollInfo &rollInfo);

/**
 * @brief parseRepetition Helper function to parse repetitions of the entire
 * roll command
 * @param rep string from which info about how many repetitions and (dis)adv
 * will be acquired
 * @param rollInfo ref to RollInfo instance where the info will be stored
 * @return whether parsing was successful
 */
ParseResult parseRepetition(std::string rep, RollInfo &rollInfo);

/**
 * @brief parseInput Parses user input
 * @param inputString user input to be parsed
 * @param rollInfo RollInfo instance where the data retrieved from parsing will
 * be stored
 * @return whether the parse succeeded
 */
ParseResult parseInput(std::string inputString, RollInfo &rollInfo);

/**
 * @brief rollAndPrint rolls dice, prints results
 * @param rollInfo struct holding information about what exactly the user wants
 * to roll
 * @param rng random number generator to be used
 * @param distributions uniform int distributions to sim perfectly fair dice;
 * can be added to at runtime if need be
 * @return result of the roll, i.e.:
 * - if no repetition or discarding, sum of all the dice and the post-dice mod
 * 2d6+5 = first d6 + second d6 + 5
 * - if dice are to be discarded, sum of all kept dice and post-dice mod
 * 4d6kh3 = highest d6 + second highest d6 + third highest d6
 * - if roll is repeated with advantage, the higher of two values
 * d20+2ra = max(first d20+2, second d20+2)
 * - if with disadvantage, the lower of the two
 * d20-3rd = min(first d20-3, second d20-3)
 * - if the roll is repeated without (dis)advantage, the sum of all rolls
 * 4d6kh3r6 = for(i=1;i<7;i++){sum+=top3(d6,d6,d6,d6)}
 */
int64_t rollAndPrint(RollInfo rollInfo, std::mt19937_64 &rng,
			std::map<uint64_t,std::uniform_int_distribution<>> &distributions);

/**
 * @brief rollAndPrintOnce Helper function- rolls and prints one repetition of
 * a roll
 * @param rollInfo info about what exactly the user wants to roll
 * @param rng random number generator to be used
 * @param distributions random int distributions simulating dice
 * @return the sum of all dice in the roll repetition and the post-dice modifier
 */
int64_t rollAndPrintOnce(RollInfo rollInfo, std::mt19937_64 &rng,
			std::map<uint64_t, std::uniform_int_distribution<>> &distributions);
/**
 * @brief print_roll_vector Helper function - prints a vector of dice roll
 * results; e.g. in a 3d6+d8 roll this is called twice, once for the 3d6 and
 * once for the d8
 * @param rolls numbers to print, pre-sorted if need be (keeping top/bottom x)
 * @param toKeep how many to not enclose in parentheses;
 * enclosed in parens = not counted toward final sum
 */
void print_roll_vector(const std::vector<int64_t> &rolls,
					   size_t toKeep = std::numeric_limits<size_t>::max());

/**
 * @brief parseSeed parses a new seed for the RNG from given string
 * @param seedString string from which the new seed is to be parsed
 * @return new seed if parsing was successful, 0 if not
 */
int64_t parseSeed(std::string seedString);

/**
 * @brief removeSpaces removes spaces from a string
 * @param originalString string from which spaces are to be removed
 * @return string with all non-space characters from originalString removed
 */
std::string removeSpaces(std::string originalString){
	std::string rv = "";
	for(unsigned i=0;i<originalString.size();++i){
		if(originalString[i] == ' ' || originalString[i] == '\t'){
			continue;
		}
		rv+=originalString[i];
	}
	return rv;
}


int main(){
	auto rng = initializeRng();
	std::map<uint64_t,std::uniform_int_distribution<>> distributions;
	for(auto i = commonRolls.begin();i != commonRolls.end(); ++i){
		distributions[*i] = std::uniform_int_distribution<>(1,*i);
	}
	std::cout << "Input your roll or q to quit" << std::endl
			  << "Format: (XdY(khZ|klZ)|M)"
			  << "(+M2|-M2|+X2dY2(khZ2|klZ2)|-X2dY2(khZ2|klZ2))*(ra|rd|rQ)"
			  << std::endl;
	std::string input;
	do{
		input.clear();
		RollInfo rollInfo;
		std::cout << ">";
		std::getline(std::cin,input);
		input = removeSpaces(input);
//		std::cerr << input << std::endl;
		ParseResult parseResult = parseInput(input, rollInfo);
		switch(parseResult){
			case QUIT: return 0;
			case OK:{
				int x = rollAndPrint(rollInfo,rng,distributions);
				// x can be used for debugging
			}break;
			case MALFORMED_COMMAND:{
				std::cerr << "Invalid input: malformed command\n";
			}break;
			case UNPARSABLE_NUMBER:{
				std::cerr << "Invalid input: unparsable number\n";
			}break;
			case RESEED:{
				int newSeed = parseSeed(input);
				rng = initializeRng(newSeed);
			}break;
			default:break; // TBD
		}
	}while(true);
}

std::mt19937_64 initializeRng(unsigned long long seed){
	if(seed == 0){
		auto nanosecondsSinceEpoch =
				std::chrono::duration_cast<std::chrono::nanoseconds>
				(std::chrono::high_resolution_clock::now().time_since_epoch());
		seed = static_cast<unsigned long long>(nanosecondsSinceEpoch.count());
	}
//	std::cerr << "[DEBUG]: Initializing random number generator with seed: "
//	  		  << seed << std::endl;
	std::mt19937_64 rng(seed);
	return rng;
}

ParseResult parseRepetition(std::string rep, RollInfo &rollInfo){
//	std::cerr << "Parsing repetition substring: " << rep << std::endl;
	if(rep[1] == 'a'){
		rollInfo.advantageFactor = ADVANTAGE;
		rollInfo.repeats = 2;
	}else if(rep[1] == 'd'){
		rollInfo.advantageFactor = DISADVANTAGE;
		rollInfo.repeats = 2;
	}else if(rep[1] >= '0' && rep[1] <= '9'){
		try{
			rollInfo.repeats = std::stoul(rep.substr(1));
		}catch(std::logic_error le){
			return UNPARSABLE_NUMBER;
		}
	}else{
		return MALFORMED_COMMAND;
	}
	return OK;
}

ParseResult parseRollOrMod(std::string roll, RollInfo &rollInfo){
//	std::cerr << "Parsing roll or mod substring: " << roll << std::endl;
	size_t postSignPos = 0;
	auto dPos = roll.find_first_of('d');
	if(dPos == std::string::npos){
		try{
			rollInfo.modifierAfterDice += std::stol(roll);
		}catch(std::logic_error le){
			return UNPARSABLE_NUMBER;
		}
		return OK;
	}
	SingleRoll sr;
	if(roll [0] == '-'){
		sr.sign = roll[0];
	}
	if(roll[0] == '+' || roll[0] == '-'){
		postSignPos = 1;
	}
	// die count - default 1 if not provided
	if(dPos != postSignPos){
		try{
			sr.dieCount = std::stoul(roll.substr(postSignPos,dPos-postSignPos));
		}catch(std::logic_error le){
			return UNPARSABLE_NUMBER;
		}
	}else{
		sr.dieCount = 1;
	}
	auto keepPos = roll.find_first_of('k');
	// die sides
	try{
		if(keepPos == std::string::npos){
			sr.dieSides = std::stoul(roll.substr(dPos+1));
		}else{
			sr.dieSides = std::stoul(roll.substr(dPos+1,keepPos-(dPos+1)));
		}
	}catch(std::logic_error le){
		return UNPARSABLE_NUMBER;
	}
	if(keepPos != std::string::npos){ // keeping top/bottom k dice
		switch(roll[keepPos+1]){
			case 'h':
				sr.discardMode = KEEP_HIGH;
			break;
			case 'l':
				sr.discardMode = KEEP_LOW;
			break;
			default:
				return MALFORMED_COMMAND;
		}
		try{
			sr.diceToKeep = std::stoul(roll.substr(keepPos+2));
		}catch(std::logic_error le){
			return UNPARSABLE_NUMBER;
		}
	}
	rollInfo.rolls.push_back(sr);
	return OK;
}

ParseResult parseInput(std::string inputString, RollInfo &rollInfo){
	if(inputString[0] == 'q'){ // quit
		return QUIT;
	}
	if(inputString[0] == 's'){ // seed
		return RESEED;
	}
	size_t repPos = 0, posIncr = 0;
	repPos = inputString.find_first_of('r');
	size_t plusMinusPos = inputString.find_first_of("+-");
	size_t pos = 0;
	ParseResult singleRollParseResult;
	while(plusMinusPos != std::string::npos){
		singleRollParseResult
				= parseRollOrMod(inputString.substr(pos,plusMinusPos-pos),
							rollInfo);
		if(singleRollParseResult != OK){
			return singleRollParseResult;
		}
		pos = plusMinusPos;
		plusMinusPos = inputString.find_first_of("+-",pos+1);
	}
	singleRollParseResult =
			parseRollOrMod(inputString.substr(pos,repPos == std::string::npos?
											 repPos:repPos-pos),
					  rollInfo);
	if(singleRollParseResult != OK){
		return singleRollParseResult;
	}
	if(repPos != std::string::npos){
		// repetition will happen
		ParseResult repetitionParseResult =
				parseRepetition(inputString.substr(repPos), rollInfo);
		if(repetitionParseResult != OK){
			return repetitionParseResult;
		}
	}
	return OK;
}

int64_t rollAndPrint(RollInfo rollInfo, std::mt19937_64 &rng,
			std::map<uint64_t,std::uniform_int_distribution<>> &distributions){
	int64_t rv = 0;
	std::vector<int64_t> rollResults;
	for(unsigned i=0;i<rollInfo.repeats;++i){
		rollResults.push_back(rollAndPrintOnce(rollInfo,rng,distributions));
	}
	switch(rollInfo.advantageFactor){
		case NONE:{
			rv = std::accumulate(rollResults.begin(),rollResults.end(),0);
			if(rollInfo.repeats>1){
				std::cout << "Sum of all rolls: " << rv << std::endl;
			}
		}break;
		case ADVANTAGE:{
			rv = std::max(rollResults[0],rollResults[1]);
			std::cout << "Rolled with advantage, final result: "
					  << rv << std::endl;
		}break;
		case DISADVANTAGE:{
			rv = std::min(rollResults[0],rollResults[1]);
			std::cout << "Rolled with disadvantage, final result: "
					  << rv << std::endl;
		}break;
	}
	return rv;
}

int64_t rollAndPrintOnce(RollInfo rollInfo, std::mt19937_64 &rng,
			std::map<uint64_t,std::uniform_int_distribution<>> &distributions){
	int64_t rollResult = 0;
	std::vector<std::vector<int64_t>> rollVectors;
	for(unsigned i=0;i<rollInfo.rolls.size();++i){
		auto &cr = rollInfo.rolls[i];// current roll
		auto distribIt = distributions.find(cr.dieSides);
		if(distribIt == distributions.end()){ // if one doesn't exist, make one
			distributions[cr.dieSides] =
				std::uniform_int_distribution<>(1,cr.dieSides);
//			std::cerr << "creating a distribution for a d" << cr.dieSides
//					  << std::endl;
			distribIt = distributions.find(cr.dieSides);
		}
		std::vector<int64_t> diceResults;
		for(unsigned i=0;i<cr.dieCount;++i){ // roll the dice
			diceResults.push_back(distribIt->second(rng));
		}
		switch(cr.discardMode){ // handle discarding if needed
			case KEEP_ALL:{
			}break;
			case KEEP_HIGH:{
				std::sort(diceResults.begin(),diceResults.end(),
						  std::greater<int64_t>());
			}break;
			case KEEP_LOW:{
				std::sort(diceResults.begin(),diceResults.end());
			}break;
		}
		if(cr.sign == '+'){
			for(unsigned i=0;i<std::min(diceResults.size(),cr.diceToKeep);++i){
				rollResult += diceResults[i]; // count it, if we're keeping it
			}
		}else{ // minus, subtract from result
			for(unsigned i=0;i<std::min(diceResults.size(),cr.diceToKeep);++i){
				rollResult -= diceResults[i]; // count it, if we're keeping it
			}
		}
		// printing
		if(i > 0 || cr.sign == '-'){
			std::cout << cr.sign;
		}
		print_roll_vector(diceResults,cr.diceToKeep);
		rollVectors.push_back(diceResults);
	}
	rollResult += rollInfo.modifierAfterDice; // add the post-dice modifier
	if(rollInfo.modifierAfterDice){
		rollInfo.modifierAfterDice>0? std::cout << " + " : std::cout << " - ";
		std::cout << std::abs(rollInfo.modifierAfterDice);
	}
	std::cout << " = " << rollResult << std::endl;
	return rollResult;
}

void print_roll_vector(const std::vector<int64_t> &rolls, size_t toKeep){
	std::cout << "[";
	for(size_t i=0; i<rolls.size(); i++){
		if(i == toKeep){
			std::cout << '(';
		}
		std::cout << rolls[i];
		if(i == rolls.size()-1 && toKeep < rolls.size()){
			std::cout << ')';
		}else{
			std::cout << ' ';
		}
	}
	std::cout << "]";
}

int64_t parseSeed(std::string seedString){
	int64_t rv = 0;
	try{
		rv = std::stol(seedString.substr(1));
	}catch(std::logic_error le){
		return 0;
	}
	return rv;
}
