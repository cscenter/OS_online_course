#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int __init getseg_init(void)
{
	unsigned short sel;

	__asm__ volatile ("movw %%cs, %0" : "=r"(sel));

	pr_debug("CS = %#hx\n", sel);

	return 0;
}

static void __exit getseg_fini(void)
{
}

module_init(getseg_init);
module_exit(getseg_fini);
MODULE_LICENSE("GPL");
