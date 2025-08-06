---
title: "PuzzleClock"
author: "SneakySquid"
description: "PuzleClock is an alarm clock that makes you solve a puzzle to disable it"
created_at: "2024-03-20"
---

Total time: ~14 hours


# Day 1 (30/7) ~4 hours

Today I researched the best moduels to use. I settled on common inexpensive moduels that most people would have around to make this an easy project for others to build, with the exception of the large lcd screen. I decided to use lcd instead of oled as it is a lot cheaper for such a large screen. I decided to use an arduino uno as the microcontroller as it is the most common and most people would have one lying around. I refined the idea of the project and came up with some puzzles/games for the clock: maths question susing random numbers, a maze game, a snake clone and a game where you have to dodge a projected block. I have a joystick module lying around that I can incorporate into the design. I also decided to include an ultrasonic sensor to detect if the user tries to cheat and return to bed within a certain amount of time of disabling the alarm in which case the alarm is reenabled. Next, I created custom symbols for the components I would be using using the datasheets and wired everything up to create the schematic
![alt text](Images/image-1.png)  
I then created the case in fusion 360, after a few iterations going for this slanted design with cutouts for the components. I also engraved th ename of the project into the top left of the case. I will print the back of the case seperately to the rest of the case so its easy to access the inside of the clock once printed. I experimented with a few different designs at first but thought that this more ergonomic slanted case would look the best and is most convenient as it would be easily visble when laying down.
![alt text](Images/image.png)

# Day 2 (31/7) ~6 hours

Today I wrote the arduino sketch for the project which was the most time consuming and difficult part. I need to wait for the lcd display to arrive to be able to test everything properly but I used some random arduino tutorials for different modules to create this sketch which i think should work. I started with base code for each moduel then built out the lcd display logic. I organised all of the logic to read the different sensors and the different games/puzzles into different functions to keep the code modularised and readable for easy debugging on the physical hardware and added a bunch of serial prints and error checks so its easy to tell what went wrong when i test this on the physcial project. I implemented a I also spent a while finding models of electronic components to put into the CAD of the case, importing everything into blender. I  wasn't able to get a render of the clock as my laptop isnt powerful enough but got this which shows roughly where each component will go. due to some weird scaling issues it looks like some of the components dont fit properly or clip through the model but they will fit properly in real life. Peformance issues on my laptop also made blender very difficult to work with.
![alt text](Images/image-2.png)

# Day 3 (4/8) ~ 1 hour
With the review feedback I created this wiring diagram and got a few more angles of the cad for the readme. I also filleted the edges of the case to make it less angular and more asthetically pleasing. Finally I updated the cad files in the repo.
![alt text](Images/image-3.png)

# Day 4 (6/8) ~3 hours
I redesigned the entire case to make it more accessible and printable. I went for a more boxy design as is seen in some modern alarm clocks and i added a breadboard model to make it clear exactly how all of the components were going to be sitting in the case and then modelled cut outs in the case basd on that rather thna using abstract measurements. I also imported all of the components from grabcad into fusion360 so the project can be better visualised in colour and fixed all of the previous scaling issues. I also removed the rotary encoder as it is kind of redundant as we can use the joystick to navigate menus and updated the firmware to reflect this change.
![alt text](Images/image-7.png)
![alt text](Images/image-8.png)
![alt text](Images/image-9.png)
