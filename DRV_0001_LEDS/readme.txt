
1. �� leds_4412.c �ŵ�drivers/leds
   �޸� drivers/leds/Makefile:
   obj-y += leds_4412.o

����Makefile��Ҫ��make menuconfig���������ѡ����ܰ�leds_4412.o������
# LED Core
obj-$(CONFIG_NEW_LEDS)			+= led-core.o
obj-$(CONFIG_LEDS_CLASS)		+= led-class.o
obj-$(CONFIG_LEDS_TRIGGERS)		+= led-triggers.o
# LED Triggers
obj-$(CONFIG_LEDS_TRIGGER_TIMER)	+= ledtrig-timer.o


2.���������ں� make menuconfig
-> Device Drivers 
-> LED Support 
[*] LED Class Support 
[*] LED Trigger support 
<*> LED Timer Trigger


3. ���������ں�
make zImage


