#ifndef __RAMFS_H__
#define __RAMFS_H__

#include <stddef.h>
#include <list.h>


/**
 * We need a some kind of file system, so this is a very primitive
 * in-ram file system, that doesn't remove (usually called unlink)
 * operation and directories.
 **/
#define RAMFS_MAX_NAME	255


struct file {
	struct list_head ll;
	long size;
	char name[RAMFS_MAX_NAME + 1];

	struct list_head data;
};


int ramfs_create(const char *name, struct file **res);
int ramfs_open(const char *name, struct file **res);
void ramfs_close(struct file *file);

long ramfs_readat(struct file *file, void *data, long size, long offs);
long ramfs_writeat(struct file *file, const void *data, long size, long offs);

void ramfs_setup(void); 

#endif /*__RAMFS_H__*/
