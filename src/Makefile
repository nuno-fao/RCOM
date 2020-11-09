all: appBuild

cableBuild:
	gcc cable.c -o ./build/cable
appBuild:
	gcc main.c link.c -o ./build/app
cableRun: cableBuild
	sudo ./build/cable
