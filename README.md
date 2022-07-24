# rpdiceroller
Dumb console dice roller app for (TT)RPGs or other board games

How to use:
- build (or download binary)
- run
- q to quit
- s to reseed RNG (with time since epoch, optionally input exact seed, must be 64-bit integer)
- AdB for A B-sided dice
- number for post-dice modifier
- +/- for addition/subtraction
- khX/klX to keep top/bottom X dice from the original roll
- ra|rd|rQ to repeat with advantage, with disadvantage, Q times
- never worry about losing dice again- worry instead about having power

Concrete command examples:

- d6: basic die, as in any game that involves them
- d20: basic skill check in D&D without any modifiers
- 4d6kh3r6: roll up a new character in D&D (preferably get DM permission to shuffle stats around to fit what you want to play)
- d1000: for use with the net libram of random magical effects or similar
- 76-4d6-2: dealing 4d6+2 damage to a creature with 76 HP remaining before the attack
- s1000: seeds RNG with seed 1000
