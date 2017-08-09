
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/delay.h>
 
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

#include <linux/leds.h>//led_classdev结构体在此声明

/* 自定义一个结构体描述led的gpio管脚和管脚对应的字符串名字*/
struct led_desc {
	int gpio;
	char *name;
};

static struct led_desc led_gpios[] = {
	{EXYNOS4212_GPM4(0), "led1"},/*对应led_desc的两个成员*/
	{EXYNOS4212_GPM4(1), "led2"},
	{EXYNOS4212_GPM4(2), "led3"},
	{EXYNOS4212_GPM4(3), "led4"},
};

/* 自定义一个结构体描述设备 */
struct led_classdev_4412 {
	struct led_classdev cdev;
	int gpio;/*用于保存设备对应的gpio*/
};


static struct led_classdev_4412 *led_devs;
static void	 brightness_set_4412(struct led_classdev *led_cdev,
			  enum led_brightness brightness)/*声明去leds.h中拷贝*/
{
	struct led_classdev_4412 *dev = (struct led_classdev_4412 *)led_cdev;

	led_cdev->brightness = brightness;

	if (brightness != LED_OFF)
		gpio_set_value(dev->gpio, 0);
	else
		gpio_set_value(dev->gpio, 1);
}


static int leds_init(void)/*定义成static,可以让不同的驱动程序在入口函数和出口函数同名的情况下共存*/
{
	int i;
	int ret;
	
	/* 1. alloc led_classdev 函数kzalloc申请内存并且全部初始化为0*/
	led_devs = kzalloc(sizeof(struct led_classdev_4412) * sizeof(led_gpios)/sizeof(led_gpios[0]), GFP_KERNEL);
														/*sizeof(led_gpios)/sizeof(led_gpios[0]等于4*/
	/*返回值led_devs是指向led_classdev_4412结构体的指针，kzalloc分配了4个led_classdev_4412结构体大小，所以可以led_devs[i]，i:0~3*/
	if (led_devs == NULL) {
		printk("No memory for device\n");
		return -ENOMEM;
	}
	
	for (i = 0; i < sizeof(led_gpios)/sizeof(led_gpios[0]); i++)
	{

		s3c_gpio_cfgpin(led_gpios[i].gpio, S3C_GPIO_OUTPUT);
		gpio_set_value(led_gpios[i].gpio, 1);//全灭

		/* 2. set */
		led_devs[i].cdev.max_brightness = LED_FULL;//设置最大亮度值
		led_devs[i].cdev.brightness_set = brightness_set_4412;
		led_devs[i].cdev.flags = LED_CORE_SUSPENDRESUME;//支持休眠唤醒功能
		led_devs[i].cdev.brightness = LED_OFF;//表示当前亮度
		led_devs[i].cdev.name = led_gpios[i].name;/*注意是led_desc描述的结构体*/
		//led_devs[i].cdev.default_trigger = "timer";
		led_devs[i].gpio = led_gpios[i].gpio;

		/* 3. led_classdev_register */
		ret = led_classdev_register(NULL, &led_devs[i].cdev);
		if (ret) {
			i--;/*某一个设备（led0,led1,...）注册失败，要把之前的也卸载掉*/
			while (i >= 0) {
				led_classdev_unregister(&led_devs[i].cdev);
				i--;
			}
			kfree(led_devs);/*释放为4个led申请的内存*/
			return -EIO;
		}
	}
	
	return 0;
}

static void leds_exit(void)
{
	int i;
	for (i = 0; i < sizeof(led_gpios)/sizeof(led_gpios[0]); i++)
	{
		led_classdev_unregister(&led_devs[i].cdev);
	}
	kfree(led_devs);
}

module_init(leds_init);
module_exit(leds_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("www.100ask.net");


