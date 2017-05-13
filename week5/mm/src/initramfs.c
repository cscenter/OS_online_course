#include <initramfs.h>

#include <ctype.h>
#include <memory.h>
#include <misc.h>
#include <print.h>
#include <ramfs.h>
#include <string.h>


/**
 * The format of the initrd/initramfs is not important but just in
 * case you're interested it's called CPIO and it's the same format
 * Linux Kernel uses for it's initramfs.
 **/

#define S_IFMT	0xF000
#define S_IFREG	0x8000
#define S_ISREG(mode)	(((mode) & S_IFMT) == S_IFREG)


struct cpio_header {
	char magic[6];
	char inode[8];
	char mode[8];
	char uid[8];
	char gid[8];
	char nlink[8];
	char mtime[8];
	char filesize[8];
	char major[8];
	char minor[8];
	char rmajor[8];
	char rminor[8];
	char namesize[8];
	char chksum[8];
} __attribute__((packed));


static unsigned long parse_hex(const char *data)
{
	static const char digits[] = "0123456789abcdef";
	char buffer[9];

	memset(buffer, 0, sizeof(buffer));
	memcpy(buffer, data, sizeof(buffer) - 1);

	unsigned long ans = 0;
	for (const char *ptr = buffer; *ptr; ++ptr)
		ans = ans * 16 + (strchr(digits, tolower(*ptr)) - digits);

	return ans;
}

static void create_file(const char *name, const void *data, size_t size)
{
	struct file *file;

	if (ramfs_create(name, &file)) {
		printf("failed to create file %s\n", name);
		while (1);
	}

	const long ret = ramfs_writeat(file, data, size, 0);

	if (ret != (long)size) {
		printf("failed to write file %s (%ld %ld)\n", name, ret, (long)size);
		while (1);
	}
	ramfs_close(file);
}

static void parse_cpio(const char *data, size_t size)
{
	size_t pos = 0;

	while (pos < size && size - pos >= sizeof(struct cpio_header)) {
		const struct cpio_header *hdr =
					(struct cpio_header *)(data + pos);
		const unsigned long mode = parse_hex(hdr->mode);
		const unsigned long namesize = parse_hex(hdr->namesize);
		const unsigned long filesize = parse_hex(hdr->filesize);

		pos += sizeof(struct cpio_header);
		const char *filename = data + pos;

		pos = (pos + namesize + 3) & ~(size_t)3;

		if (pos > size)
			break;

		if (strcmp(filename, "TRAILER!!!") == 0)
			break;

		const char *filedata = data + pos;
		pos += filesize;

		if (pos > size)
			break;

		if (S_ISREG(mode))
			create_file(filename, filedata, filesize);
		pos = (pos + 3) & ~(size_t)3;
	}
}

void initramfs_setup(void)
{
	parse_cpio(va(initrd_begin), initrd_end - initrd_begin);
}
