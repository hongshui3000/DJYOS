//----------------------------------------------------
//Copyright (C), 2004-2009,  luoshitian.
//版权所有 (C), 2004-2009,   罗侍田.
//所属模块:标准输入--触摸屏模块
//作者:  罗侍田.
//版本：V1.1.0
//文件描述: 触摸屏设备驱动程序
//其他说明:
//修订历史:
//1. 日期: 2009-01-04
//   作者:  罗侍田.
//   新版本号: V1.0.0
//   修改说明: 原始版本
//------------------------------------------------------
#if (cfg_single_touch == 1)

#ifndef TOUCH_H_
#define TOUCH_H_

#ifdef __cplusplus
extern "C" {
#endif

struct st_touch_adjust
{
    s32 ratio_x,ratio_y;        //定点小数，低16位是小数位。
    s32 offset_x,offset_y;       //定点小数，低16位是小数位。
};

struct single_touch_private
{
    ufast_t (*read_touch)(struct single_touch_msg *touch_data);
    struct single_touch_msg touch_loc;
    bool_t status;
};

s32 touch_install_device(char *touch_name,struct single_touch_private *touch_pr);

void touch_scan(void);
ptu32_t module_init_touch(ptu32_t para);

#ifdef __cplusplus
}
#endif

#endif /* TOUCH_H_ */


#endif  //for #if (cfg_single_touch == 1)

