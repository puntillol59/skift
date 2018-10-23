/* Copyright © 2018 MAKER.                                                    */
/* This code is licensed under the MIT License.                               */
/* See: LICENSE.md                                                            */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <skift/atomic.h>

#include "kernel/cpu/gdt.h"
#include "kernel/cpu/idt.h"
#include "kernel/cpu/irq.h"
#include "kernel/cpu/isr.h"

#include "kernel/filesystem.h"
#include "kernel/graphic.h"
#include "kernel/keyboard.h"
#include "kernel/logger.h"
#include "kernel/memory.h"
#include "kernel/modules.h"
#include "kernel/mouse.h"
#include "kernel/multiboot.h"
#include "kernel/paging.h"
#include "kernel/system.h"
#include "kernel/tasking.h"
#include "kernel/version.h"

#define LINE \
    "================================================================================"

multiboot_info_t mbootinfo;

extern int __end;
uint get_kernel_end(multiboot_info_t *minfo)
{
    return max((uint)&__end, modules_get_end(minfo));
}

void main(multiboot_info_t *info, s32 magic)
{
    log(KERNEL_UNAME);
    log("Booting...");
    log(LINE);

    /* --- Early operation -------------------------------------------------- */
    memcpy(&mbootinfo, info, sizeof(multiboot_info_t));
    //graphic_early_setup(1366, 768);
    graphic_early_setup(800, 600);

    /* --- System check ----------------------------------------------------- */
    log("System check...");
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
        PANIC("Wrong boot loader please use a GRUB or any multiboot2 bootloader (MBOOT_MAGIC=0x%x)!", magic);

    if ((info->mem_lower + info->mem_upper) < 255 * 1024)
    {
        PANIC("No enought memory (%dkib)!", info->mem_lower + info->mem_upper);
    }

    /* --- Setup cpu context ------------------------------------------------ */
    log(LINE);
    log("Initializing cpu context...");
    setup(gdt);
    setup(pic);
    setup(idt);
    setup(isr);
    setup(irq);

    /* --- System context --------------------------------------------------- */
    log(LINE);
    log("Initializing system context...");
    setup(memory, get_kernel_end(&mbootinfo), (mbootinfo.mem_lower + mbootinfo.mem_upper) * 1024);
    setup(tasking);
    setup(filesystem);
    setup(modules, &mbootinfo);

    /* --- Devices ---------------------------------------------------------- */
    log(LINE);
    log("Initializing device context...");
    setup(graphic);
    setup(mouse);
    setup(keyboard);

    /* --- Finalizing System ------------------------------------------------ */
    log(LINE);
    log("Enabling interupts, paging and atomics.");
    sk_atomic_enable();
    sti();
    log(KERNEL_UNAME);
    log(LINE);

    /* --- Entering userspace ----------------------------------------------- */
    PROCESS init = process_exec("app/maker.hideo.compositor", NULL);

    if (init)
    {
        //thread_waitproc(init);

        message_t msg;
        while(messaging_receive(&msg) == SUCCESS)
        {
            log("Recieved message ID=%d from PROCESS=%d: %s.", msg.id, msg.from, msg.name);
        }
    }
    else
    {
       PANIC("Init not found!"); 
    }

    PANIC("The init process has return!");
}