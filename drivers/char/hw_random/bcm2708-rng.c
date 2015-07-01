/**
 * Copyright (c) 2010-2012 Broadcom. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the above-listed copyright holders may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2, as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/hw_random.h>
#include <linux/printk.h>

#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/platform.h>

#define RNG_CTRL		(0x0)
#define RNG_STATUS		(0x4)
#define RNG_DATA		(0x8)
#define RNG_FF_THRESHOLD	(0xc)

/* enable rng */
#define RNG_RBGEN               0x1
/* double speed, less random mode */
#define RNG_RBG2X               0x2

/* the initial numbers generated are "less random" so will be discarded */
#define RNG_WARMUP_COUNT      0x40000

static int bcm2708_rng_data_read(struct hwrng *rng, u32 *buffer)
{
	void __iomem *rng_base = (void __iomem *)rng->priv;
	unsigned words;
	/* wait for a random number to be in fifo */
        do {
		words = __raw_readl(rng_base + RNG_STATUS)>>24;
	}
        while (words == 0);
	/* read the random number */
	*buffer = __raw_readl(rng_base + RNG_DATA);
	return 4;
}

static struct hwrng bcm2708_rng_ops = {
	.name		= "bcm2708",
	.data_read	= bcm2708_rng_data_read,
};

static int __init bcm2708_rng_init(void)
{
	void __iomem *rng_base;
	int err;

	/* map peripheral */
	rng_base = ioremap(RNG_BASE, 0x10);
	pr_info("bcm2708_rng_init=%p\n", rng_base);
	if (!rng_base) {
		pr_err("bcm2708_rng_init failed to ioremap\n");
		return -ENOMEM;
	}
	bcm2708_rng_ops.priv = (unsigned long)rng_base;

	/* set warm-up count & enable */
	__raw_writel(RNG_WARMUP_COUNT, rng_base + RNG_STATUS);
	__raw_writel(RNG_RBGEN, rng_base + RNG_CTRL);

	/* register driver */
	err = hwrng_register(&bcm2708_rng_ops);
	if (err) {
		pr_err("bcm2708_rng_init hwrng_register()=%d\n", err);
		iounmap(rng_base);
	}
	return err;
}

static void __exit bcm2708_rng_exit(void)
{
	void __iomem *rng_base = (void __iomem *)bcm2708_rng_ops.priv;
	pr_info("bcm2708_rng_exit\n");
	/* disable rng hardware */
	__raw_writel(0, rng_base + RNG_CTRL);
	/* unregister driver */
	hwrng_unregister(&bcm2708_rng_ops);
	iounmap(rng_base);
}

module_init(bcm2708_rng_init);
module_exit(bcm2708_rng_exit);

MODULE_DESCRIPTION("BCM2708 H/W Random Number Generator (RNG) driver");
MODULE_LICENSE("GPL and additional rights");
