//----------------------------------------------------
//Copyright (C), 2004-2009,  luoshitian.
//版权所有 (C), 2004-2009,   罗侍田.
//所属模块: 硬件定时器
//作者:  罗侍田.
//版本：V1.0.0
//文件描述: 系统初始化文件
//其他说明:
//修订历史:
//2. ...
//1. 日期: 2009-04-24
//   作者:  罗侍田.
//   新版本号：V1.0.0
//   修改说明: 原始版本，本文件内容原来和加载器放在一起
//------------------------------------------------------
#include "os_inc.h"
#include "exception.h"

ptu32_t init_debug_info(ptu32_t para);
void prj_module_init(void);
ptu32_t module_init_fs_nandflash(ptu32_t para);

//----操作系统内核组件初始化---------------------------------------------------
//功能：初始化操作系统功能组件，包括OS正常运行必备的组件，以及部分可选的组件。
//      可选组件初始化函数共有3个:
//      1、sys_module_init，在sys_init.c中，在这里配置的可选组件有一个特征，就是
//          不需要进一步调用跟工程配置相关的函数。比如"时钟日期"模块，因为需要进
//          一步调用rtc模块，故不能放在这里，而是应该放在工程相关目录中。
//      2、prj_module_init，参见该函数注释。
//      3、run_module_init，参见该函数注释。
//      
//参数：无
//返回：无
//---------------------------------------------------------------------------
void sys_module_init(void)
{
    module_init_rsc1(0);
    module_init_lock1(0);
    module_init_rsc2(0);
    module_init_memb(0);
    module_init_lock2(0);
    
#if ((cfg_pan_driver == 1) && (cfg_run_mode != cn_mode_tiny))
    module_init_driver(0);
#endif

#if (cfg_stdin_device_limit != 0)
    module_init_stdin( 0 );
#endif

#if((cfg_debug_info == 1) && (cfg_run_mode != cn_mode_tiny))
    init_debug_info(0);
#endif

}

//----dlsp模式加载程序--------------------------------------------------------
//功能：打开autoexec.bat文件逐行执行，如果命令文件里有main函数则是应用程序，执行
//      之，如果没有则搜索module_init_modulename函数，执行之，两个函数都没有则
//      只加载，不执行里面的代码。
//参数: 无。
//返回: 无。
//----------------------------------------------------------------------------
void dlsp_module_loader(void)
{
}

//----系统启动程序-----------------------------------------------------
//功能：执行操作系统加载，模块(设备)初始化、用户初始化以及启动操作系统
//参数：无
//返回：无
//---------------------------------------------------------------------------
void start_sys(void)
{
#if cfg_nude_terminal == 1
    nude_terminal_init();   //裸奔式输入输出终端初始化
    printf_str_nude("\r\n正在启动...\r\n");
#endif

    //自此可以调用malloc族函数，用准静态堆分配算法，若要释放内存，要求严格配对
    //调用malloc-free函数，或者不调用
    exp_init();
    module_init_heap_static(0);
    __djy_set_delay();
    __djy_init_sys();
    sys_module_init();

    prj_module_init();

#if (cfg_dynamic_mem == 1)
    module_init_heap_dynamic(0);  //自此malloc函数执行块相联算法
#endif
    __djy_start_os();
}

