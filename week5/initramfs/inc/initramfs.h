#ifndef __INITRAMFS_H__
#define __INITRAMFS_H__


/**
 * The function parses initrd/initramfs image and populates
 * ramfs with the content of the image.
 **/

void initramfs_setup(void);

#endif /*__INITRAMFS_H__*/
