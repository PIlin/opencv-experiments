BUILD :=ino build -m
INST :=ino upload -m
UNO :=uno
DUE :=atmega328
LEO :=leonardo

all:



# uno

bu:
	$(BUILD) $(UNO)

iu:
	$(INST) $(UNO)

biu: bu iu


# due

bd:
	$(BUILD) $(DUE)

id:
	$(INST) $(DUE)

bid: bd id

# leo

bl:
	$(BUILD) $(LEO)

il:
	$(INST) $(LEO)

bil: bl il

#

size:
	/Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/bin/avr-size .build/uno/firmware.elf

.PHONY: all bu iu biu bd id bid clean size
