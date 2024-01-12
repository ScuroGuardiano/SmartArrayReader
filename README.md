# HP SMART ARRAY RAID READER
> I hope you and I will never have to use this program.

> Everything was implemented and tested using HP Smart Array P420 with 8.00-0 firmware version. I don't know if it will work correctly for arrays operated with different controller or firmware version.

You had very nice Hardware RAID 6 array operated with HP Smart Array Controller but the controller have died? Now you have to buy new controller and wait 'til it arrives and pray for it to work in order to recover your precious data? **NOT ANYMORE**. Here it is brand new tool to read smart drive arrays, all you need to do is to `modprobe nbd` kernel module and provide drives in the correct order with raid level:
```sh
sudo modprobe nbd
sudo ./hewlett-read --raid=6 /dev/sdb /dev/sdc /dev/sdd /dev/sde
```
And now you have it under `/dev/nbd0`! If you had partions there you can mount it:
```
sudo mount -o ro /dev/nbd0p1 /mnt/cool
```
You must remember to mount it as readonly `-o ro`, otherwise it will fail.

And here you go, you can now safely copy all your data without buying another Smart Array Controller.

But wait! What if you have forgotten the order of the drives? Don't worry, just use `packard-tell`!
```sh
sudo ./packard-tell /dev/sdd /dev/sdb /dev/sdc /dev/sde
```
and this what packard-tell will tell you:
```
The correct order is:
/dev/sdb /dev/sdc /dev/sdd /dev/sde
```

And if you have missing drive for RAID 5 or two\* missing drives for RAID 6 you can replace drive with `X`, for example:
```sh
sudo ./hewlett-read /dev/sdb /dev/sdc X /dev/sde
```

> \* *For RAID 6 only 1 missing drive recovery works at this moment.*

## Compiling
Sorry, I can't into `make`, so to compile you just do
```sh
./build.sh
```
and wait for ages for it to compile.

## TODO
- [ ] RAID 6 with 2 missing drives - see [Raid 6 problem](./raid-6-problem)
- [ ] RAID 10
- [ ] RAID 50
- [ ] RAID 60
- [ ] Better error handling
- [ ] Code cleanup
- [ ] Reading metadata from drives with `packard-tell`
- [ ] Writing blogpost about everything that I know about P420 controller

## Libraries used
[BUSE](https://github.com/acozzette/BUSE) - :love:

## Legal
This Project is in no way affiliated with, authorized, maintained, sponsored or endorsed by HP or Hewlett Packard Enterprise or any of its affiliates or subsidiaries.

# License
GPL 3.0
