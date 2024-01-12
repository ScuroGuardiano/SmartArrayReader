# I need help with RAID 6 Reed Solomon
I tried to implement reading RAID 6 array with 2 missing drives but failed. I followed [this article](https://anadoxin.org/blog/error-recovery-in-raid6.html/) but unfortunately it does not work for HP Smart Array P420. You can see my messy code for that [here](https://github.com/ScuroGuardiano/SmartArrayReader/blob/3fa1692efca7408521b938f637b62de75ccaada4/src/smart_array_raid_6_reader.cpp). It works for Linux RAID though, so I guess Smart Array Controllers codes that a little bit differently. I have no mathematical knowledge to figure out how it's calculate so I am reaching out for help.

I created 2 RAID 6 Arrays - with 4 drives and 5 drives. In each folder there are binary files with one stripe of data. Data drives: `dn.bin`, parity drive: `parity.bin` and reed solomon drive `rs.bin`.  
For example with 4 drives, first byte of `rs.bin` and `parity.bin` is calculated from the first byte of `d1.bin` and `d2.bin`, second from second and so on.

With 4 drives I added all combinations of two bytes, with 5 drives I counld't fit all combinations of three bytes on one stripe but I hope that's enough.

All I need is some function that would allow me to calculate:
- one byte of missing data drive from all remaining data drives and rs drive
- one byte of two missing data drives from all remaining data drives, parity drive and rs drive

I tried for few long hours to figure out how it's calculated but I am unable, I know only basic math so if someone could help me with that I would be veeeery thankful and of course I will make honorable mention of one that would help me in the project README 🙏