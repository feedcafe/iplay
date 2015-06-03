# Copyright 2013 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	src/iplay.c

LOCAL_MODULE:= iplay

include $(BUILD_EXECUTABLE)
