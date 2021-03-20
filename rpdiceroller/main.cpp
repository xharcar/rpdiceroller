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

typedef struct{
	size_t dieCount = 1;
	size_t dieSides = 0;
	size_t diceToKeep = 1;
	size_t repeats = 1;
	int modifierAfterDice = 0;
	AdvantageFactor advantageFactor = AdvantageFactor::NONE;
	DiscardMode discardMode = DiscardMode::KEEP_ALL;
}RollInfo;


int parseInput(std::string inputString, RollInfo &rollInfo);

int64_t roll_and_print(RollInfo rollInfo, std::mt19937_64 &rng);

int64_t roll_and_print_once(RollInfo rollInfo, std::mt19937_64 &rng, std::uniform_int_distribution<> &d);// helper fn

void print_roll_vector(const std::vector<int64_t> &rolls, size_t n = std::numeric_limits<size_t>::max());


int main()
{
	auto nanosecondsSinceEpoch = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
	//    std::cerr << "[DEBUG]: Initializing random number generator with nanoseconds since epoch: " << nanosecondsSinceEpoch.count() << std::endl;
	std::mt19937_64 rng(static_cast<unsigned long long>(nanosecondsSinceEpoch.count()));
	std::cout << "Input your roll or q to quit" << std::endl << "Format: XdY(+M|-M)(a(dv)|d(is)|khZ|klZ)(rQ)" << std::endl;
	do{
		std::string inputString = "";
		RollInfo rollInfo;
		std::cout << ">";
		std::cin >> inputString;
		int parseResult = parseInput(inputString, rollInfo);
		switch(parseResult){
			case -1: return 0; // told to exit
			case 0:{ // all good, roll and print
				int64_t supertotal = roll_and_print(rollInfo,rng);
				if(rollInfo.repeats > 1){
					for(size_t i=1;i<rollInfo.repeats;i++){
						std::cout << "Repeating roll #" << i+1 << ":" << std::endl;
						supertotal += roll_and_print(rollInfo,rng);
					}
					std::cout << "Sum of all rolls: " << supertotal << std::endl;
				}
			}break;
			case 1:{ // something missing, like the 'd' in 2d6, or the 'h' in kh3
				std::cerr << "Invalid input: malformed command\n";
			}break;
			case 2:{ // for when someone types d6+! instead of d6+1 accidentally
				std::cerr << "Invalid input, typo in a number caused an exception to be thrown\n";
			}break;
			case 3:{ // rolling say 4d6kh3 with adv/disadv doesn't make much sense; and even then it can be emulated with r2 if needed
				std::cerr << "Invalid input, can't roll with advantage/disadvantage while not keeping all dice or vice-versa\n";
			}break;
			default: break; //?
		}
	}while(true);
}

int parseInput(std::string inputString, RollInfo &rollInfo){
	size_t parsePos = 0, posIncr = 0;
	if(inputString.at(0) == 'q'){ // quit command
		return -1;
	}
	parsePos = inputString.find_first_of('d');
	switch(parsePos){
		case inputString.npos:{
			return 1; // gotta have the d at least for now
		}
		case 0:break; // "d6" being functionally equivalent to "1d6"
		default:{
			try{
				rollInfo.dieCount = std::stoul(inputString, &parsePos);
			}catch(std::invalid_argument ia){
				return 2;
			}
		}
	}
	parsePos++; // move to past 'd'
	try{
		rollInfo.dieSides = std::stoul(inputString.substr(parsePos),&posIncr);
	}catch(std::invalid_argument ia){
		return 2;
	}
	parsePos = inputString.find_first_of("adkr+-",parsePos);
	while(parsePos != std::string::npos && parsePos < inputString.length()){
		switch(inputString[parsePos]){
			case 'a':{ // advantage
				if(rollInfo.discardMode != DiscardMode::KEEP_ALL || rollInfo.advantageFactor != AdvantageFactor::NONE){
					return 3;
				}
				rollInfo.advantageFactor = AdvantageFactor::ADVANTAGE;
				parsePos++;// idea: move 1 character, then search again - seems to work
				parsePos = inputString.find_first_of("adkr+-",parsePos);
			}break;
			case 'd':{ // disadvantage
				if(rollInfo.discardMode != DiscardMode::KEEP_ALL || rollInfo.advantageFactor != AdvantageFactor::NONE){
					return 3;
				}
				rollInfo.advantageFactor = AdvantageFactor::DISADVANTAGE;
				parsePos++;
				parsePos = inputString.find_first_of("adkr+-",parsePos);
			}break;
			case 'k':{ // keep top/bottom K dice
				if(rollInfo.advantageFactor != AdvantageFactor::NONE || rollInfo.discardMode != DiscardMode::KEEP_ALL){
					return 3;
				}
				switch(inputString[parsePos+1]){
					case 'h':{
						rollInfo.discardMode = DiscardMode::KEEP_HIGH;
					}break;
					case 'l':{
						rollInfo.discardMode = DiscardMode::KEEP_LOW;
					}break;
					default:return 1;// keep highest or keep lowest, nothing else makes sense
				}
				try{
					parsePos += 2; // "kh" and "kl" are both 2 characters, always
					rollInfo.diceToKeep = std::stoul(inputString.substr(parsePos),&posIncr);
					if(rollInfo.diceToKeep > rollInfo.dieCount){ // silently ignore
						rollInfo.discardMode = DiscardMode::KEEP_ALL;
						rollInfo.diceToKeep = 0;
					}
					parsePos+=posIncr;
				}catch(std::invalid_argument ia){
					return 2;
				}
			}break;
			case 'r':{
				try{
					rollInfo.repeats = std::stoul(inputString.substr(parsePos+1),&posIncr);
					parsePos += posIncr+1;
				}catch(std::invalid_argument ia){
					return 2;
				}

			}break;
			case '+':
			case '-':{
				try{
					rollInfo.modifierAfterDice = std::stol(inputString.substr(parsePos+1),&posIncr);
					if(inputString[parsePos] == '-'){
						rollInfo.modifierAfterDice *= -1;
					}
					parsePos += posIncr + 1;
				}catch(std::invalid_argument ia){
					return 2;
				}
			}break;
			default:{
				return 1;
			}
		}
	}
	return 0;
}

int64_t roll_and_print(RollInfo rollInfo, std::mt19937_64 &rng){
	std::vector<long> rolls;
	std::uniform_int_distribution<> distr(1,static_cast<int>(rollInfo.dieSides));
	int64_t rv = 0;
	rv = roll_and_print_once(rollInfo, rng, distr);
	switch(rollInfo.advantageFactor){
		case AdvantageFactor::NONE: break;
		case AdvantageFactor::ADVANTAGE:{
			int64_t rv2 = roll_and_print_once(rollInfo,rng,distr);
			rv = std::max(rv,rv2);
			std::cout << "Rolled with advantage, final result: " << rv << std::endl;
		}break;
		case AdvantageFactor::DISADVANTAGE:{
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
		case DiscardMode::KEEP_ALL: rollInfo.diceToKeep = rollInfo.dieCount;// ensure all dice are kept
		break;
		case DiscardMode::KEEP_HIGH:{
			std::sort(rolls.begin(),rolls.end(), std::greater<int>()); // descending order
		}break;
		case DiscardMode::KEEP_LOW:{
			std::sort(rolls.begin(),rolls.end()); // ascending order
		}break;
	}
	rv = std::accumulate(rolls.begin(),rolls.begin()+static_cast<int>(rollInfo.diceToKeep),0);
	rv+=rollInfo.modifierAfterDice;
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
