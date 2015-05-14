/*
 * iplay.c - playback of recorded input event
 *
 * Copyright (C) 2015 Fudong <fudongbai@gamil.com>
 *
 * iplay is based on uhid-example.c under kernel's samples/uhid directory
 * credit goes to David Herrmann <dh.herrmann@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <linux/input.h>
#include <linux/uinput.h>

#define IRECORD_EVT_FILE	"/tmp/record-input.log"

struct timeval tv_old;

static int handle_event(int fd)
{
	struct input_event event;
	struct timeval diff;
	int bytes_written;
	int bytes_read;
	int log_fd;
	unsigned firstup = 1;

	log_fd = open(IRECORD_EVT_FILE, O_RDONLY);
	if (log_fd == -1) {
		fprintf(stderr, "Unable to open input event log file: %s\n",
				IRECORD_EVT_FILE);
		return -errno;
	}

	memset(&event, 0, sizeof(event));

	do {
		bytes_read = read(log_fd, &event, sizeof(event));
		if (bytes_read <= 0)
			break;

		bytes_written = write(fd, &event, sizeof(event));
		if (bytes_written < 0)
			fprintf(stderr, "write uinput failed: %d(%s)\n",
					bytes_written, strerror(errno));

		timersub(&event.time, &tv_old, &diff);
		tv_old = event.time;

		/* skip first up, cause we do not want to sleep too long */
		if (firstup) {
			firstup = 0;
			usleep(10000);
		} else
			usleep(diff.tv_usec);
	} while (bytes_read > 0);

	close(log_fd);
	return 0;
}

int main(int argc, char **argv)
{
	const char *input = "/dev/uinput";
	int fd;
	int rc;
	int i;

	struct uinput_user_dev uinput;

	fd = open(input, O_WRONLY | O_NONBLOCK);
	if (fd < 0) {
		fprintf(stderr, "open uinput failed, fd: %d\n", fd);
		return fd;
	}

	/* set supported key events */
	rc = ioctl(fd, UI_SET_EVBIT, EV_KEY);
	for (i = KEY_ESC; i < KEY_MICMUTE; i++) {
		rc = ioctl(fd, UI_SET_KEYBIT, i);
	}

	/* set mouse events */
	rc = ioctl(fd, UI_SET_EVBIT, EV_REL);
	rc = ioctl(fd, UI_SET_RELBIT, REL_X);
	rc = ioctl(fd, UI_SET_RELBIT, REL_Y);

	for (i = BTN_MOUSE; i < BTN_TASK; i++) {
		rc = ioctl(fd, UI_SET_KEYBIT, i);
	}

	/* set absolute events */
	rc = ioctl(fd, UI_SET_EVBIT, EV_ABS);
	rc = ioctl(fd, UI_SET_ABSBIT, ABS_X);
	rc = ioctl(fd, UI_SET_ABSBIT, ABS_Y);

	memset(&uinput, 0, sizeof(uinput));

	/* setup id */
	snprintf(uinput.name, UINPUT_MAX_NAME_SIZE, "blueberry-input");
	uinput.id.bustype = BUS_BLUETOOTH;
	uinput.id.vendor = 0x0315;
	uinput.id.product = 0x0607;
	uinput.id.version = 0x0412;

	uinput.absmin[ABS_X] = 0;
	uinput.absmax[ABS_X] = 1920;
	uinput.absmin[ABS_Y] = 0;
	uinput.absmax[ABS_Y] = 1080;

	/* create input device */
	rc = write(fd, &uinput, sizeof(uinput));
	rc = ioctl(fd, UI_DEV_CREATE);

	/* read and send input event to kernel */
	handle_event(fd);

	/* destroy input device */
	rc = ioctl(fd, UI_DEV_DESTROY);
	close(fd);

	return rc;
}
