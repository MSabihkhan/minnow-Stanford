Checkpoint 6 Writeup
====================

My name: Muhammad Sabih ud din Khan

My SUNet ID: 23L-0654

I collaborated with: Aammar Noor Zia 23L-0558

I would like to thank/reward these classmates for their help: [list sunetids here]

This checkpoint took me about 4 hours to do. I [did] attend the lab session.

Program Structure and Design of the Router [Describe data
structures and approach taken. Describe alternative designs considered
or tested.  Describe benefits and weaknesses of your design compared
with alternatives -- perhaps in terms of simplicity/complexity, risk
of bugs, asymptotic performance, empirical performance, required
implementation time and difficulty, and other factors. Include any
measurements if applicable.]: 
We used a simple list (std::vector) to store the routing rules. Each rule is a struct with the prefix, length, and next hop.

When a packet arrives, I loop through the list to find the longest matching prefix. list is simpler to code and works fast enough for this assignment so we didnt consider other structures.

Implementation Challenges:
[]

Remaining Bugs:
[0]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
