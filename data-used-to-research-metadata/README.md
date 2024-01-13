# Dumped metadata from some arrays
Here's some metadata from which I tried to figure out how HP Smart Array P420 controller saves certain values.
Feel free to use it ^^

* multiple-logical-raid5 contains I think 7 drives, all with default stripe size, which is 256K:
    - 100M
    - 100M
    - 200M
    - 400M
    - 800M
    - 1600M
    - 3200M
* raid0 contains 4 RAID 0 arrays using all space of 3.6TB Drives:
    - 1 drives
    - 2 drives
    - 3 drives
    - 4 drives
* raid5-stripe-size was used by me to find where is stripe size, it has logical drives with stripe sizes of:
    - 8K
    - 16K
    - 32K
    - 64K
    - 128K
    - 256K
    - 512K
    - 1024K

All of this data helped me to figure out where are stored:
- stripe size
- logical drive size
- logical drive offset relative to physical drive
- used space on each physical drive by logical drive

All numbers are big endian, sizes are in 512-byte sectors, including stripe size. So 8K stripe size will be encoded as `0x0010`, 256K stripe size - `0x0200`

Each metadata entry for logical drive is 2080 bytes in size and starts with logical drive label.

I will put a link to metadata layout when I finish metadata parser.