/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (C) 2011 Diogo Ferreira <defer@cyanogenmod.com>
 * Copyright (C) 2012 Andreas Makris <andreas.makris@gmail.com>
 * Copyright (C) 2012 The CyanogenMod Project <http://www.cyanogenmod.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "lights"
#include <cutils/log.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <hardware/lights.h>

char const*const RED_LED_FILE 			= "/sys/class/leds/led1/brightness";
char const*const GREEN_LED_FILE 		= "/sys/class/leds/led2/brightness";
char const*const BLUE_LED_FILE 			= "/sys/class/leds/led3/brightness";
char const*const RED_LED_FILE_TRIGGER	= "/sys/class/leds/led1/trigger";
char const*const GREEN_LED_FILE_TRIGGER	= "/sys/class/leds/led2/trigger";
char const*const BLUE_LED_FILE_TRIGGER	= "/sys/class/leds/led3/trigger";
char const*const RED_LED_FILE_DELAYON	= "/sys/class/leds/led1/delay_on";
char const*const GREEN_LED_FILE_DELAYON	= "/sys/class/leds/led2/delay_on";
char const*const BLUE_LED_FILE_DELAYON	= "/sys/class/leds/led3/delay_on";
char const*const RED_LED_FILE_DELAYOFF	= "/sys/class/leds/led1/delay_off";
char const*const GREEN_LED_FILE_DELAYOFF= "/sys/class/leds/led2/delay_off";
char const*const BLUE_LED_FILE_DELAYOFF	= "/sys/class/leds/led3/delay_off";
char const*const LCD_BACKLIGHT_FILE     = "/dev/backlight-1wire";

/* Synchronization primities */
static pthread_once_t g_init = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
/* Mini-led state machine */
static struct light_state_t g_notification;//led灯的状态
static struct light_state_t g_battery;


static int write_int (const char *path, int value) {
	int fd;
	static int already_warned = 0;
	fd = open(path, O_RDWR);
	if (fd < 0) {
		if (already_warned == 0) {
			ALOGE("write_int failed to open %s\n", path);
			already_warned = 1;
		}
		return -errno;
	}
	char buffer[20];
	int bytes = snprintf(buffer, sizeof(buffer), "%d\n", value);
	int written = write (fd, buffer, bytes);
	close(fd);
	return written == -1 ? -errno : 0;
}
static int write_string (const char *path, const char *value) {
	int fd;
	static int already_warned = 0;
	fd = open(path, O_RDWR);
	if (fd < 0) {
		if (already_warned == 0) {
			ALOGE("write_string failed to open %s\n", path);
			already_warned = 1;
		}
		return -errno;
	}
	char buffer[20];
	int bytes = snprintf(buffer, sizeof(buffer), "%s\n", value);
	int written = write (fd, buffer, bytes);
	close(fd);
	return written == -1 ? -errno : 0;
}
/* Color tools is_lit里面会判断灯的颜色,如果为0就不需要设置,不为0就需要设置)*/
static int is_lit (struct light_state_t const* state) {
	return state->color & 0x00ffffff;
}
static int rgb_to_brightness (struct light_state_t const* state) {
	int color = state->color & 0x00ffffff;
	return ((77*((color>>16)&0x00ff))
			+ (150*((color>>8)&0x00ff)) + (29*(color&0x00ff))) >> 8;
}
/* The actual lights controlling section 背景灯*/
static int set_light_backlight (struct light_device_t *dev, struct light_state_t const *state) {
	int brightness = rgb_to_brightness(state);
	ALOGV("%s brightness=%d color=0x%08x", __func__,brightness,state->color);
	pthread_mutex_lock(&g_lock);//为了多线程互斥得操作led

	/* brightness 0-255 */
	/* LCD_BACKLIGHT_FILE能接收的是0-127 */
	
	write_int (LCD_BACKLIGHT_FILE, brightness/2);
	
	pthread_mutex_unlock(&g_lock);//为了多线程互斥得操作led
	return 0;
}
static void set_shared_light_locked (struct light_device_t *dev, struct light_state_t *state) {
	int r, g, b;
	int delayOn,delayOff;
	/* 提取RGB值 */
	r = (state->color >> 16) & 0xFF;
	g = (state->color >> 8) & 0xFF;
	b = (state->color) & 0xFF;
    delayOn = state->flashOnMS;
	delayOff = state->flashOffMS;
	if (state->flashMode != LIGHT_FLASH_NONE) {
		write_string (RED_LED_FILE_TRIGGER, "timer");//用write_string将“timer”写入某个trigger文件如RED_LED_FILE_TRIGGER
		write_string (GREEN_LED_FILE_TRIGGER, "timer");
		write_string (BLUE_LED_FILE_TRIGGER, "timer");
		write_int (RED_LED_FILE_DELAYON, delayOn);//写入闪烁的时间参数
		write_int (GREEN_LED_FILE_DELAYON, delayOn);
		write_int (BLUE_LED_FILE_DELAYON, delayOn);
		write_int (RED_LED_FILE_DELAYOFF, delayOff);
		write_int (GREEN_LED_FILE_DELAYOFF, delayOff);
		write_int (BLUE_LED_FILE_DELAYOFF, delayOff);
	} else {
		write_string (RED_LED_FILE_TRIGGER, "none");//不闪烁就不往某个trigger文件写参数
		write_string (GREEN_LED_FILE_TRIGGER, "none");
		write_string (BLUE_LED_FILE_TRIGGER, "none");
	}
	write_int (RED_LED_FILE, r);//参数设置好点亮对应的灯
	write_int (GREEN_LED_FILE, g);
	write_int (BLUE_LED_FILE, b);
}
static void handle_shared_battery_locked (struct light_device_t *dev) {
	if (is_lit (&g_notification)) {//看is_lit的返回值
		set_shared_light_locked (dev, &g_notification);//此处体现通知灯的优先级比电池灯高，先看是否设置通知灯
	} else {
		set_shared_light_locked (dev, &g_battery);
	}
}
/*电池灯*/
static int set_light_battery (struct light_device_t *dev, struct light_state_t const* state) {

	ALOGV("%s flashMode=%d onMS = %d offMS = %d color=0x%08x", __func__,state->flashMode,state->flashOnMS,state->flashOffMS,state->color);

	pthread_mutex_lock (&g_lock);
	g_battery = *state;
	handle_shared_battery_locked(dev);//设置灯的状态，电池灯和通知灯共享所以是shared_battery
	pthread_mutex_unlock (&g_lock);
	return 0;
}
/*通知灯*/
static int set_light_notifications (struct light_device_t *dev, struct light_state_t const* state) {
	ALOGV("%s flashMode=%d onMS = %d offMS = %d color=0x%08x", __func__,state->flashMode,state->flashOnMS,state->flashOffMS,state->color);
	pthread_mutex_lock (&g_lock);
	g_notification = *state;
	handle_shared_battery_locked(dev);
	pthread_mutex_unlock (&g_lock);
	return 0;
}
/* Initializations */
void init_globals () {
	pthread_mutex_init (&g_lock, NULL);
}
/* Glueing boilerplate */
static int close_lights (struct light_device_t *dev) {
	if (dev)
		free(dev);
	return 0;
}
static int open_lights (const struct hw_module_t* module, char const* name,
						struct hw_device_t** device) {
	int (*set_light)(struct light_device_t* dev,
					 struct light_state_t const *state);
	if (0 == strcmp(LIGHT_ID_BACKLIGHT, name)) {
		set_light = set_light_backlight;//set_light函数指针根据不同的name指向不同的函数
	}
	else if (0 == strcmp(LIGHT_ID_BATTERY, name)) {
		set_light = set_light_battery;
	}
	else if (0 == strcmp(LIGHT_ID_NOTIFICATIONS, name)) {
		set_light = set_light_notifications;
	}
	else {
		return -EINVAL;
	}
	pthread_once (&g_init, init_globals);
	struct light_device_t *dev = malloc(sizeof (struct light_device_t));//分配light_device_t设备自定义结构体
	memset(dev, 0, sizeof(*dev));
	dev->common.tag 	= HARDWARE_DEVICE_TAG;
	dev->common.version = 0;
	dev->common.module 	= (struct hw_module_t*)module;
	dev->common.close 	= (int (*)(struct hw_device_t*))close_lights;
	dev->set_light 		= set_light;
	*device = (struct hw_device_t*)dev;
	return 0;
}
static struct hw_module_methods_t lights_module_methods = {
	.open = open_lights,
};
struct hw_module_t HAL_MODULE_INFO_SYM = {
	.tag = HARDWARE_MODULE_TAG,
	.version_major = 1,
	.version_minor = 0,
	.id = LIGHTS_HARDWARE_MODULE_ID,
	.name = "Sony lights module",
	.author = "Diogo Ferreira <defer@cyanogenmod.com>, Andreas Makris <Andreas.Makris@gmail.com>",
	.methods = &lights_module_methods,
};
