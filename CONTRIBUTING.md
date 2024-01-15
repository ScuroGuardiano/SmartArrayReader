If you want to contribute to this project, there are some things to do

## RAID 6 with 2 missing drives
See -> [Raid 6 problem](./raid-6-problem)

## Makefile
Really, Makefile feels like hieroglyphs to me, so if you can make Makefile (you see what I've done here? uwu) from [build.sh](./build.sh) then that would be great :purple_heart:

## Code readability improvements
Any contributions that would make code more readable are very welcomed :3

## Don't sacrifice code readability for performance
If you want to make some performance improvements then please, don't sacrifice code readability. I want this code to be as readable as possible so everyone can read it and know how Smart Array Controllers work.

## Parallel reads are not needed
You may see that I read data sequentially from drives instead of using multiple threads to read simultaneously from multiple drives. Even in recovery function I don't do that. Well, it's not needed. On 12 drives RAID 5 with 1 missing drive I get 341MB/s in sequential read. I guess OS optimizes reads from the drives so parallelism is not necessary here. Maybe it could be a little bit faster but look point above ^^
