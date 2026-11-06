// SPDX-License-Identifier: GPL-2.0
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <linux/utsname.h>

static int factory_value = -1;
static char factory_char[2]="\0";


static int factory_proc_show(struct seq_file *m, void *v)
{
    u32 value = 0x0;
    if  (factory_value == -1){
        return 0;
    }
    if (factory_value == 0)
        value = 0x0;
    if (factory_value == 1)
        value = 0x1;

    seq_printf(m, "0x%x\n",value);

    pr_err("[%s]:factory_proc_show  factory_value=[%d]\n", __func__, factory_value);

    return 0;
}
static int factory_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, factory_proc_show, NULL);
}
static const struct proc_ops factory_proc_fops = {
    .proc_flags	= PROC_ENTRY_PERMANENT,
	.proc_open	= factory_proc_open,
	.proc_lseek	= seq_lseek,
	.proc_release	= seq_release,
};
/*static int root_dev_setup(char *line)
{
	pr_err("[%s]: factory_state,  root_dev_setup, factory_reset  \n", __func__);
    if ('\0' == *line)
        return -1;
    else
        factory_value = simple_strtol(line, NULL, 10);
    return factory_value;
}
__setup("factory_reset=", root_dev_setup);*/
static int __init proc_factory_init(void)
{

    struct proc_dir_entry *prEntry;

   if ('\0' == *factory_char) {
        factory_value=-1;
    } else {
        factory_value = simple_strtol(factory_char, NULL, 10);
    }
    pr_err("proc_factory_init: create /proc/factory_state    factory_char=%s factory_value=[%d]\n", factory_char, factory_value);


    prEntry = proc_create("factory_state", 0, NULL, &factory_proc_fops);
    if (!prEntry)
        pr_notice("[%s]: failed to create /proc/factory_state\n", __func__);
    return 0;
}
static void __exit proc_factory_exit(void)
{
}

module_init(proc_factory_init);
module_exit(proc_factory_exit);

module_param_string(factory_reset, factory_char, sizeof(factory_char), 0644);
MODULE_PARM_DESC(factory_char, "factory_state.factory_reset=0 or 1");

MODULE_LICENSE("GPL");