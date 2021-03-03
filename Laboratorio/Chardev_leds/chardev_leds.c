#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>	/* for copy_to_user */
#include <linux/cdev.h>

#include <asm-generic/errno.h>
#include <linux/init.h>
#include <linux/tty.h>      /* For fg_console */
#include <linux/kd.h>       /* For KDSETLED */
#include <linux/vt_kern.h>
#include <linux/version.h> /* For LINUX_VERSION_CODE */

#define ALL_LEDS_ON 0x7
#define ALL_LEDS_OFF 0

MODULE_LICENSE("GPL");

/*
 *  Prototypes
 */
int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "leds"	/* Dev name as it appears in /proc/devices   */
#define BUF_LEN 80		/* Max length of the message from the device */

/*
 * Global variables are declared as static, so are global within the file.
 */

dev_t start;
struct cdev* chardev_leds=NULL;
struct tty_driver* kbd_driver= NULL;
static int Device_Open = 0;	/* Is device open?
				 * Used to prevent multiple access to device */

static struct file_operations fops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release
};


/* Get driver handler */
struct tty_driver* get_kbd_driver_handler(void)
{
#if ( LINUX_VERSION_CODE > KERNEL_VERSION(2,6,32) )
    return vc_cons[fg_console].d->port.tty->driver;
#else
    return vc_cons[fg_console].d->vc_tty->driver;
#endif
}

/* Set led state to that specified by mask */
static inline int set_leds(struct tty_driver* handler, unsigned int mask)
{
#if ( LINUX_VERSION_CODE > KERNEL_VERSION(2,6,32) )
    return (handler->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED,mask);
#else
    return (handler->ops->ioctl) (vc_cons[fg_console].d->vc_tty, NULL, KDSETLED, mask);
#endif
}


/*
 * This function is called when the module is loaded
 */
int init_module(void)
{
    int major;		/* Major number assigned to our device driver */
    int minor;		/* Minor number assigned to the associated character device */
    int ret;

    /* Get available (major,minor) range */
    if ((ret=alloc_chrdev_region (&start, 0, 1,DEVICE_NAME))) {
        printk(KERN_INFO "Can't allocate chrdev_region()");
        return ret;
    }

    /* Create associated cdev */
    if ((chardev_leds=cdev_alloc())==NULL) {
        printk(KERN_INFO "cdev_alloc() failed ");
        unregister_chrdev_region(start, 1);
        return -ENOMEM;
    }

    cdev_init(chardev_leds,&fops);

    if ((ret=cdev_add(chardev_leds,start,1))) {
        printk(KERN_INFO "cdev_add() failed ");
        kobject_put(&chardev_leds->kobj);
        unregister_chrdev_region(start, 1);
        return ret;
    }

    major=MAJOR(start);
    minor=MINOR(start);

    kbd_driver= get_kbd_driver_handler();

    printk(KERN_INFO "INSTRUCTIONS.\n");
    printk(KERN_INFO "chardev_leds major number is %d.\n", major);
    printk(KERN_INFO "To talk to the driver, create a dev file with\n");
    printk(KERN_INFO "'sudo mknod /dev/%s -m 666 c %d %d'\n", DEVICE_NAME, major,minor);
    printk(KERN_INFO "To turn on/off keyboard leds,\n");
    printk(KERN_INFO "'sudo echo X > /dev/leds'\n");
    printk(KERN_INFO "Remove the device file and module when done.\n");

    return SUCCESS;
}

/*
 * This function is called when the module is unloaded
 */
void cleanup_module(void)
{
    /* Turn off all leds */
    set_leds(kbd_driver,ALL_LEDS_OFF);

    /* Destroy chardev */
    if (chardev_leds)
        cdev_del(chardev_leds);

    /*
     * Unregister the device
     */
    unregister_chrdev_region(start, 1);
}

/*
 * Called when a process tries to open the device file, like
 * "cat /dev/chardev"
 */
static int device_open(struct inode *inode, struct file *file)
{
    if (Device_Open)
        return -EBUSY;

    Device_Open++;

    /* Increase the module's reference counter */
    try_module_get(THIS_MODULE);

    return SUCCESS;
}

/*
 * Called when a process closes the device file.
 */
static int device_release(struct inode *inode, struct file *file)
{
    Device_Open--;		/* We're now ready for our next caller */

    /*
     * Decrement the usage count, or else once you opened the file, you'll
     * never get get rid of the module.
     */
    module_put(THIS_MODULE);

    return 0;
}

/*
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */
static ssize_t device_read(struct file *filp,	/* see include/linux/fs.h   */
                           char *buffer,	/* buffer to fill with data */
                           size_t length,	/* length of the buffer     */
                           loff_t * offset)
{
    printk(KERN_ALERT "Sorry, this operation isn't supported.\n");
    return -EPERM;
}

/*
 * Called when a process writes to dev file: echo "hi" > /dev/leds
 */
static ssize_t
device_write(struct file *filp, const char *buffer, size_t length, loff_t * off)
{
    int bytes_to_write = length;
    char buffer_aux[BUF_LEN];
    int mask = 0x0;
    int i;
    bool badchar = false;

    /* Avoid using directly variable buffer because of its volatility */
    if (copy_from_user(buffer_aux,buffer,bytes_to_write) > BUF_LEN)
        return -ENOSPC;

    /* Looking for the leds that we want turned on */
    printk(KERN_INFO "Turning on leds.\n");
    for (i = 0; i < bytes_to_write - 1 && !badchar; ++i){
        if (buffer_aux[i] == '1'){
            mask = mask | 0x1;
        }
        else if (buffer_aux[i] == '2'){
            mask = mask | 0x2;
        }
        else if (buffer_aux[i] == '3'){
            mask = mask | 0x4;
        }
        else badchar = true;
    }
    if (badchar) return -EINVAL;

    /* Turn leds on/off */
    printk(KERN_INFO "User %d\n",current_uid().val);
    if (current_uid().val == 0){
        set_leds(kbd_driver,mask);
    }
    else return -EPERM;

    return bytes_to_write;
}