Checkpoint 1 Writeup
====================

My name: Muhammad Sabih ud din Khan

My SUNet ID: 23L-0654

I collaborated with: Ammar Noor Zia 23L-0558

I would like to thank/reward these classmates for their help: [list sunetids here]

This lab took me about 48 to 54 hours to do. I [did/did not] attend the lab session.

I was surprised by or edified to learn that: [describe]

Report from the hands-on component of the lab checkpoint: [include
information from 2.1(4), and report on your experience in 2.2]

Describe Reassembler structure and design. 
Our reassembler uses a map to store out-of-order segments. When data arrives at the expected position, we write it immediately and then drain the buffer. For out-of-order data, we merge it with overlapping segments.we tried character-by-character storage first but it was too slow and caused timeouts.Before this we were ignoring the space issue and when the test failed we got to know about the overlap issue. The segment-based approach is much faster for large data.The main design choice is handling overlaps by creating one big merged segment instead of keeping separate pieces. Existing data wins over new overlapping data to avoid duplicates. Flow control uses a sliding window based on writer capacity. Data outside this window gets rejected.

Implementation Challenges:
First challenge was that if b arrived @1 with is_last_substring = true and then afterwards that a arrived and both got flushed , it was not closing. So added a special condition at the start of the fucntuon to monitor and also added new variable last_index to keep track of that.
Second challenge was to get the  issue of the storage resolved caused due to the overlap issue. Then after trying many things using countless nested if-else , we thought why not save each character indivisially with its key so that even if an overlap arrives it will simply get overwritten instead of being overlapped.But that resulted in timeout. 
Third challenge was to come back and solve the overlap again. We made an implementation using iterators which had a loophole of getting into an infinite loop as we were comparing values and changing them side by side. 
Figured that out and finally came to the option of creating merged segments whenever we get a hole or in conssitent kind of incoming string that needs to be stored in the buffer.

Remaining Bugs:
I hope none are left now 

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I'm not sure about: [describe]
