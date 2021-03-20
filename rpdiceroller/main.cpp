#include <iostream>
#include <limits>
#include <vector>
#include <random>
#include <algorithm>
#include <cstdint>
#include <chrono>


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
	CONFLICT
};

typedef struct{
	size_t dieCount = 1;
	size_t dieSides = 0;
	size_t diceToKeep = std::numeric_limits<size_t>::max();
	size_t repeats = 1;
	int modifierAfterDice = 0;
	AdvantageFactor advantageFactor = AdvantageFactor::NONE;
	DiscardMode discardMode = DiscardMode::KEEP_ALL;
}RollInfo;


ParseResult parseInput(std::string inputString, RollInfo &rollInfo);

int64_t roll_and_print(RollInfo rollInfo, std::mt19937_64 &rng);

int64_t roll_and_print_once(RollInfo rollInfo, std::mt19937_64 &rng, std::uniform_int_distribution<> &d);// helper fn

void print_roll_vector(const std::vector<int64_t> &rolls, size_t n = std::numeric_limits<size_t>::max());

/**
 * @brief initializeRng Initializes a new std::mt19937_64 instance;
 * split out from main to allow for reseeding if someone says it's rigged
 * command implementation TBD
 * @return 64-bit mersenne twister instance seeded with the count of nanoseconds
 * since epoch at time of creation, give or take a jiffy
 */
std::mt19937_64 initializeRng(unsigned long long seed = 0){
	if(seed == 0){
		auto nanosecondsSinceEpoch = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
		seed = static_cast<unsigned long long>(nanosecondsSinceEpoch.count());
	}
	//    std::cerr << "[DEBUG]: Initializing random number generator with seed: " << nanosecondsSinceEpoch.count() << std::endl;
	std::mt19937_64 rng(seed);
	return rng;
}

int main()
{
	auto rng = initializeRng();
	std::cout << "Input your roll or q to quit" << std::endl << "Format: XdY(+M|-M)(a+|a-|khZ|klZ)(rQ)" << std::endl;
	do{
		std::string inputString = "";
		RollInfo rollInfo;
		std::cout << ">";
		std::cin >> inputString;
		ParseResult parseResult = parseInput(inputString, rollInfo);
		switch(parseResult){
			case QUIT: return 0;
			case OK:{
				int64_t supertotal = roll_and_print(rollInfo,rng);
				if(rollInfo.repeats > 1){
					for(size_t i=1;i<rollInfo.repeats;i++){
						std::cout << "Repeating roll #" << i+1 << ":" << std::endl;
						supertotal += roll_and_print(rollInfo,rng);
					}
					std::cout << "Sum of all rolls: " << supertotal << std::endl;
				}
			}break;
			case MALFORMED_COMMAND:{
				std::cerr << "Invalid input: malformed command\n";
			}break;
			case UNPARSABLE_NUMBER:{
				std::cerr << "Invalid input: unparsable number\n";
			}break;
			case CONFLICT:{
				std::cerr << "Invalid input; conflicting limits\n";
			}break;
			default:break; // TBD
		}
	}while(true);
}

ParseResult parseInput(std::string inputString, RollInfo &rollInfo){
	size_t parsePos = 0, posIncr = 0;
	if(inputString.at(0) == 'q'){ // quit command
		return QUIT;
	}
	parsePos = inputString.find_first_of('d');
	switch(parsePos){
		case inputString.npos:{
			return MALFORMED_COMMAND; // gotta have the d at least for now
		}
		case 0:break; // "d6" being functionally equivalent to "1d6"
		default:{
			try{
				rollInfo.dieCount = std::stoul(inputString, &parsePos);
			}catch(std::invalid_argument ia){
				return UNPARSABLE_NUMBER;
			}
		}
	}
	parsePos++; // move to past 'd'
	try{
		rollInfo.dieSides = std::stoul(inputString.substr(parsePos),&posIncr);
	}catch(std::invalid_argument ia){
		return UNPARSABLE_NUMBER;
	}
	parsePos = inputString.find_first_of("akr+-",parsePos);
	while(parsePos != std::string::npos && parsePos < inputString.length()){
		switch(inputString[parsePos]){
			case 'a':{ // (dis)advantage
				if(rollInfo.discardMode != KEEP_ALL
				   || rollInfo.advantageFactor != NONE){
					return CONFLICT;
				}
				inputString[parsePos+1] == '+'?
							rollInfo.advantageFactor = ADVANTAGE :
							rollInfo.advantageFactor = DISADVANTAGE;
				// technically this does mean that any character after 'a'
				// that isn't a + will mean disadvantage but
				// if people can follow a D&D/PF PHB/DMG, they can just type a -
				parsePos += 2;
				parsePos = inputString.find_first_of("adkr+-",parsePos);
			}break;
			case 'k':{ // keep top/bottom K dice
				if(rollInfo.advantageFactor != NONE
				   || rollInfo.discardMode != KEEP_ALL){
					return CONFLICT;
				}
				inputString[parsePos+1] == 'h'?
						rollInfo.discardMode = KEEP_HIGH:
						rollInfo.discardMode = KEEP_LOW;
				// same as above, let's be reasonable here
				try{
					parsePos += 2; // "kh" and "kl" are both 2 characters, always
					rollInfo.diceToKeep = std::stoul(inputString.substr(parsePos),&posIncr);
					if(rollInfo.diceToKeep > rollInfo.dieCount){ // silently ignore
						rollInfo.discardMode = KEEP_ALL;
						rollInfo.diceToKeep = 0;
					}
					parsePos+=posIncr;
				}catch(std::invalid_argument ia){
					return UNPARSABLE_NUMBER;
				}
			}break;
			case 'r':{
				try{
					rollInfo.repeats = std::stoul(inputString.substr(parsePos+1),&posIncr);
					parsePos += posIncr+1;
				}catch(std::invalid_argument ia){
					return UNPARSABLE_NUMBER;
				}

			}break;
			case '+':
			case '-':{
				try{
					rollInfo.modifierAfterDice = std::stol(inputString.substr(parsePos),&posIncr);
					parsePos += posIncr;
				}catch(std::invalid_argument ia){
					return UNPARSABLE_NUMBER;
				}
			}break;
			default:{
				return MALFORMED_COMMAND;
			}
		}
	}
	return OK;
}

int64_t roll_and_print(RollInfo rollInfo, std::mt19937_64 &rng){
	std::vector<long> rolls;
	std::uniform_int_distribution<> distr(1,rollInfo.dieSides);
	int64_t rv = 0;
	rv = roll_and_print_once(rollInfo, rng, distr);
	switch(rollInfo.advantageFactor){
		case NONE: break;
		case ADVANTAGE:{
			int64_t rv2 = roll_and_print_once(rollInfo,rng,distr);
			rv = std::max(rv,rv2);
			std::cout << "Rolled with advantage, final result: " << rv << std::endl;
		}break;
		case DISADVANTAGE:{
			int64_t rv2 = roll_and_print_once(rollInfo,rng,distr);
			rv = std::min(rv,rv2);
			std::cout << "Rolled with disadvantage, final result: " << rv << std::endl;
		}break;
	}
	return rv;
}

int64_t roll_and_print_once(RollInfo rollInfo, std::mt19937_64 &rng, std::uniform_int_distribution<> &d){
	std::vector<int64_t> rolls;
	for(size_t i=0;i<rollInfo.dieCount;i++){
		rolls.push_back(d(rng));
	}
	int64_t rv = 0;
	switch(rollInfo.discardMode){
		case KEEP_ALL: rollInfo.diceToKeep = rollInfo.dieCount;// ensure all dice are kept
		break;
		case KEEP_HIGH:{
			std::sort(rolls.begin(),rolls.end(), std::greater<int>()); // descending order
		}break;
		case KEEP_LOW:{
			std::sort(rolls.begin(),rolls.end()); // ascending order
		}break;
	}
	rv = std::accumulate(rolls.begin(),rolls.begin()+rollInfo.diceToKeep,0);
	rv += rollInfo.modifierAfterDice;
	print_roll_vector(rolls,rollInfo.diceToKeep);
	if(rollInfo.modifierAfterDice){
		std::cout << " + " << rollInfo.modifierAfterDice;
	}
	std::cout << " = " << rv << std::endl;
	return rv;
}

void print_roll_vector(const std::vector<int64_t> &rolls, size_t n){
	size_t k = n;
	std::cout << "[ ";
	for(size_t i=0; i<rolls.size(); i++){
		if(i >= k){
			std::cout << '(' << rolls[i] << ')';
		}else{
			std::cout << rolls[i];
		}
		std::cout << ' ';
	}
	std::cout << "]";
}
