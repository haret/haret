/*
    Linux loader for Windows CE
    Copyright (C) 2005 Ben Dooks

    For conditions of use see file COPYING

	$Id: cpu-s3c24xx.cpp,v 1.1 2005/04/06 10:45:14 fluffy Exp $
*/


#include "haret.h"
#include "xtypes.h"
#define CONFIG_ACCEPT_GPL
#include "setup.h"
#include "memory.h"
#include "util.h"
#include "output.h"
#include "gpio.h"
#include "video.h"
#include "cpu.h"
#include "resource.h"






struct cpu_fns cpu_s3c24xx = {
	"S3C24XX",
	NULL,
	NULL,
	NULL
};