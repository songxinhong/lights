
1. 把 leds_4412.c 放到drivers/leds
   修改 drivers/leds/Makefile:
   obj-y += leds_4412.o

根据Makefile需要在make menuconfig中添加以下选项才能把leds_4412.o用起来
# LED Core
obj-$(CONFIG_NEW_LEDS)			+= led-core.o
obj-$(CONFIG_LEDS_CLASS)		+= led-class.o
obj-$(CONFIG_LEDS_TRIGGERS)		+= led-triggers.o
# LED Triggers
obj-$(CONFIG_LEDS_TRIGGER_TIMER)	+= ledtrig-timer.o


2.重新配置内核 make menuconfig
-> Device Drivers 
-> LED Support 
[*] LED Class Support 
[*] LED Trigger support 
<*> LED Timer Trigger


3. 重新配置内核
make zImage


