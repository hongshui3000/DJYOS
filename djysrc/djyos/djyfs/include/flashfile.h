//----------------------------------------------------
//Copyright (C), 2004-2009,  lst.
//版权所有 (C), 2004-2009,   lst.
//所属模块:flash文件系统
//作者：lst
//版本：V1.0.0
//文件描述:flash文件系统主模块
//其他说明:
//修订历史:
//2. ...
//1. 日期: 2009-01-04
//   作者: lst
//   新版本号: V1.0.0
//   修改说明: 原始版本
//------------------------------------------------------
#ifndef _NANDFILE_H_
#define _NANDFILE_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "os_inc.h"

#if ((cfg_djyfs == 1) && (cfg_run_mode != cn_mode_tiny) && (cfg_djyfs_flash == 1))

#include "ecc256.h"


#define cn_MDR_DDR_main     0xfe
#define cn_MDR_DDR_bak      0xfd
#define cn_MDR_PCRB         0xfc
#define cn_DBX_sum_max      3

#define cn_MDR_CDR_size     512     //MDR表中CDR的尺寸，须是8的倍数
#define cn_MDR_DBX_size     56      //MDR表中每个DBX表的尺寸，须是8的倍数
//MDR中CDR和DBX表的总尺寸
#define cn_CDR_and_DBX_size (cn_MDR_CDR_size + cn_DBX_sum_max*cn_MDR_DBX_size)
//滚动记录区偏移地址
#define cn_roll_offset       (((cn_CDR_and_DBX_size + 7) /8)*8)
//MDR表的尺寸，不允许小于856，否则函数DFFSD_install_chip会报错，详见函数注释
#define cn_MDR_size         4096


struct st_MDR_flag
{
    bool_t DBX_created[cn_DBX_sum_max];   //true=文件柜已经建立，false=尚未建立
    uint32_t MDR_main_block;              //存储MDR_main的起始块号
    uint32_t MDR_bak_block;               //存储MDR_bak的起始块号
    uint32_t roll_next_main;              //滚动存储的下一个存储位置
    uint32_t roll_next_bak;               //滚动存储的下一个存储位置
};

struct flash_chip
{
    struct rsc_node chip_node;
    uint32_t block_sum;         //总块数
    uint32_t block_size;        //块尺寸
    uint32_t valid_sum;         //可用块数
    uint32_t rev_blocks;        //保留块数量
    uint32_t rev_start;         //保留区首块块号，保留区前是芯片描述块。
    uint32_t DBX_start;         //用于文件柜的首块块号
    uint32_t DBX_read_buf_size; //读文件缓冲区尺寸
    uint32_t DBX_write_buf_size;//写文件缓冲区尺寸
    struct st_MDR_flag MDR;     //MDR表相关参数
    uint8_t  *block_buf;        //块缓冲区
    struct semaphore_LCB *left_semaphore;
    struct semaphore_LCB *right_semaphore;

    //以下是典型的flash操作函数指针，编写一个具体芯片的驱动时，设为具体芯片的
    //操作函数,这些函数并不是每一种芯片都需要，不需要的用NULL。

    //检查把参数规定的内容写入flash前，是否需要擦除。如果buf=NULL则检查由offset
    //和size设定的区域是否被擦除
    //true = 已准备好，不需要擦除(或空块),false = 需要擦除；
    bool_t   (*query_block_ready_with_ecc)(uint32_t block,uint32_t offset,
                                  uint8_t *buf,uint32_t size);
    bool_t   (*query_block_ready_no_ecc)(uint32_t block,uint32_t offset,
                                  uint8_t *buf,uint32_t size);
    //org_data中缓冲的是flash中原来的数据，欲修改为new_data中数据，检查是否需要
    //先擦除，new_data==NULL则检查把org_data改为任何数据时是否需要擦除,
    //没有带ecc版本。
    bool_t   (*query_ready_with_data)(uint8_t *new_data,uint8_t *org_data,
                                            uint32_t size);
    //擦除1块，返回: true = 成功擦除，false = 坏块
    bool_t (*erase_block)(uint32_t block);
    bool_t   (*check_block)(uint32_t Block);  //返回: true=好块，false=坏块。

    //读一块之内的任意长度数据，返回校验过的正确数据
    //返回：cn_limit_uint32=参数错误，其他值：实际写入数量
    uint32_t (*read_data_with_ecc)(uint32_t block,uint32_t offset,
                          uint8_t *buf,uint32_t size);
    //写入一块之内的任意长度数据，校验方法取决于具体器件驱动程序，也可能无校验。
    //返回：cn_limit_uint32=参数错误，其他值：实际写入数量
    uint32_t (*write_data_with_ecc)(uint32_t block,uint32_t offset,
                           uint8_t *buf,uint32_t size);
    //不带校验的读函数，返回 实际读取的数据量，参数错误则返回cn_limit_uint32
    uint32_t (*read_data_no_ecc)(uint32_t block,uint32_t offset,
                          uint8_t *buf,uint32_t size);
    //不带校验的读函数，返回：cn_limit_uint32=参数错误，其他值：实际写入数量
    uint32_t (*write_data_no_ecc)(uint32_t block,uint32_t offset,
                           uint8_t *buf,uint32_t size);
    //写掉电保护块,PCR_block为保存掉电保护信息的块号，protected_block是被保护的
    //块号，均是芯片的绝对块号。
    //返回: true = 成功，false = 写入错误PCRB_block是坏块
    bool_t (*write_PCRB)(uint32_t PCRB_block,
                         uint32_t protected_block,uint8_t *buf);
    //把掉电恢复块的数据恢复到目标块，这样可以使掉电恢复算法完全由芯片驱动提供
    //PCRB_block是掉电恢复块的绝对块号。
    //返回: true=成功恢复或无需恢复，false=恢复失败
    bool_t (*restore_PCRB)(uint32_t PCRB_block,uint32_t *restored);
};

//MAT表前指针和后指针的最高两位组合起来表示该块的属性，最多16种属性，目前用到7种
//向前指针的高两位为低，后指针为高。所以，MAT表实际上是30位的。
#define cn_file_block_start     0   //文件MAT链的首块(有可能同时也是末块)
#define cn_file_block_continue  1   //文件MAT链的后续块
#define cn_FDT_block            2   //分配给目录表的块
#define cn_invalid_block        3   //坏块
#define cn_free_block           4   //空闲块
#define cn_DDR_main_block       5   //用于保存DDR_main的块
#define cn_DDR_bak_block        6   //用于保存DDR_bak的块
#define cn_PCRB_block           7   //用于保存掉电恢复数据的块

struct MAT_table
{
    uint32_t previous;      //向前指针(块号，与C语言的指针不同)，低30位有效
    uint32_t next;          //向后指针，低30位有效。
    uint8_t  attr;          //块属性，见cn_file_block族常量定义
};
//本结构由st_DBX_device_tag结构中的medium_tag成员指向，芯片driver不可见
struct st_DBX_flash_tag
{
    struct flash_chip *chip;    //文件柜所属芯片
    bool_t   nand_ecc;          //是否做ecc校验，一般来说，nand类型flash做多媒体
                                //存储器时，可不做ecc校验。
    uint32_t DBX_no;            //在所属芯片中的文件柜编号
    uint32_t start_block;       //起始块号，相对于芯片的绝对块号
    uint32_t block_sum;         //总块数
    uint32_t valid_sum;         //有效块数
    uint32_t free_sum;          //空闲块数
    uint32_t PCRB_no;           //掉电恢复块的块号，文件柜内的相对块号
    uint32_t DDR_main;          //首个DDR表块号，文件柜内的相对块号
    uint32_t DDR_bak;           //第二个DDR表块号，文件柜内的相对块号
    uint32_t ero;               //文件柜状态
    uint32_t FDT_capacity;      //已经分配的FDT块的FDT_item容量，文件柜总共的表项数
    uint32_t FDT_free_num;      //已经使用的FDT块数量，还可以使用的FDT表项
    uint32_t DDR_size;          //DDR表尺寸
    uint32_t *DDR_DBL;          //DBL表，先main后bak
    uint32_t DDR_ARB;         //磨损次数基数，加磨损次数表(16bit)得实际磨损次数
    uint32_t DDR_FDSB;          //目录表首块块号
    struct MAT_table *DDR_MAT;  //MAT表
    //主DDR表写入标志数组，true=已写入flash，false=未写，数组大小与DDR表占用的
    //块数相同，主备各一个。
    bool_t  *writed_DDR_main;
    bool_t  *writed_DDR_bak;
    //以下两个表，在分配内存时须按编译器要求保证边界对齐(使用djyos动态内存分配
    //可以保证)
    uint32_t *ART_position; //磨损位置表，该块在磨损顺序表中的位置，
    uint32_t *ART_block_no; //磨损块号表，与次数表同列，表明次数表中对应项的块号
    uint16_t *ART_times;    //磨损次数表，忙块和闲块分别按磨损次数排序
    uint32_t balance_region[19];//磨损分区表，忙块闲块各9个分区，最后是坏块分区
};

struct file_access_tag
{
    sint64_t true_size;         //文件实际长度，它只反映已经写入flash的数据，
                                //而fp中的file_size则包含写缓冲区中的数据，
    uint32_t last_block_no;     //最后一块的块号
    uint32_t write_block_no;    //写指针所在块号
    uint32_t read_block_no;     //读指针所在块号
};

//定义文件属性,与fat格式共有的属性放在fat相同的位号,用于fdt表中的attr成员。
//涉及到编译器行为差异，不宜使用枚举型数据(enum)。
#define cn_FS_ATTR_READ_ONLY   (1<<0)
#define cn_FS_ATTR_HIDDEN      (1<<1)
#define cn_FS_ATTR_USER        (1<<2)   //用户属性
#define cn_FS_ATTR_RESERVE     (1<<3)   //保留
#define cn_FS_ATTR_DIRECTORY   (1<<4)   //目录
#define cn_FS_ATTR_ARCHIVE     (1<<5)    //文件
#define cn_FS_ATTR_DELETED     (1<<7)   //空闲(删除)标志
//特别解释一下cn_FS_ATTR_USER，在fat中，这个位的位置用来表示是否系统文件，然而，
//某文件是否作为系统文件，是文件系统使用者的问题，文件系统driver不应该过问的，
//所以，在这个地方，DFFSD模块放了一个由用户解释的位。
//同样，cn_FS_ATTR_RESERVE位本类是fat的卷标位，亦不用。

#define cn_flash_file_name_limit    215     //未来把fat表，证号奏出256字节，fat表容易计算
#define cn_flash_DBX_name_limit     31      //flash文件柜名字长度

//本结构是flashd river的私有数据结构，芯片driver(例如nand.c)不可见
struct fdt_info
{
    uint8_t  mattr;              //固定格式保存在存储介质中的文件属性,
    uint8_t  name_buf_len;       //文件名buf长度，应=cn_flash_file_name_limit+1
                                 //提供给应用程序解读目录表用
    uint8_t  second_create;      //文件建立时间:second,2位组合BCD码
    uint8_t  minute_create;      //文件建立时间:minute,2位组合BCD码
    uint8_t  hour_create;        //文件建立时间:hour,2位组合BCD码
    uint8_t  date_create;        //文件建立日期,2位组合BCD码
    uint8_t  month_create;       //文件建立月份,2位组合BCD码
    uint8_t  year_low_create;    //文件建立年度,4位组合BCD码
    uint8_t  year_high_create;   //文件建立年度,4位组合BCD码
    uint8_t  second_modify;      //文件修改时间:second,2位组合BCD码
    uint8_t  minute_modify;      //文件修改时间:minute,2位组合BCD码
    uint8_t  hour_modify;        //文件修改时间:hour,2位组合BCD码
    uint8_t  date_modify;        //文件修改日期,2位组合BCD码
    uint8_t  month_modify;       //文件修改月份,2位组合BCD码
    uint8_t  year_low_modify;    //文件修改年度,4位组合BCD码
    uint8_t  year_high_modify;   //文件修改年度,4位组合BCD码
    uint8_t  file_size[8];       //文件尺寸，根目录的file_size域的用途比较特殊，
                                 //前4字节是空闲fdt表项数，后4字节是总表项数
    //以下4个指针构成与rsc类似的链表，方便管理
    uint8_t  parent[4];          //父目录的struct fdt_info条目号
    uint8_t  fstart_dson[4];     //文件起始块号/文件夹的子条目号
    uint8_t  previous[4];        //同目录内的前一个文件条目号
    uint8_t  next[4];            //同目录内的后一个文件条目号
    char     name[cn_flash_file_name_limit+1];  //文件名，最多215个字符
};

struct MDR_temp_record
{
    char name[cn_DBX_sum_max][cn_flash_DBX_name_limit+1];  //文件柜名
    bool_t   formatted[cn_DBX_sum_max];         //文件柜格式化否
    uint32_t DDR_main_block_no[cn_DBX_sum_max]; //DDR_main块号
    uint32_t DDR_bak_block_no[cn_DBX_sum_max];  //DDR_bak块号
    uint32_t PCRB_block_no[cn_DBX_sum_max];     //PCRB块号
    uint32_t start_blocks[cn_DBX_sum_max];      //文件柜起始块号
    uint32_t blocks_sum[cn_DBX_sum_max];        //文件柜块数
};
//MDR表中芯片描述部分,占用512字节，本结构只是定义了使用了的部分，
//故不能使用sizeof来申请内存。
struct MDR_CDR_mirror
{
    char chip_init_flag[8];         //芯片初始化标志(8字节=djyosfs)
    uint8_t chip_CDR_size[4];       //CDR尺寸，等于常数cn_MDR_CDR_size
    uint8_t chip_block_sum[4];      //flash芯片总块数（包括坏块）,
    uint8_t chip_block_size[4];     //块尺寸，
    uint8_t chip_MDR_bak[4];        //MDR_bak块号，
    uint8_t chip_reserve[4];        //芯片保留块数量，
    uint8_t chip_MDR_CDR_ECC[4];    //CDR的校验码
};
//MDR表中文件柜描述部分,占用56字节，本结构只是定义了使用了的部分，
//故不能使用sizeof来申请内存。
struct MDR_DBX_mirror
{
    char  DBX_format_flag[8];       //文件柜格式化标志
    char  DBX_name[cn_flash_DBX_name_limit+1]; //文件柜名，相当于windows的卷标
    uint8_t DBX_start_block[4];     //起始块号，在芯片中的绝对序号,
    uint8_t DBX_blocks_sum[4];      //文件柜占用块数
    uint8_t chip_MDR_DBX_ECC[4];    //DBX的校验码
};

bool_t DFFSD_uninstall_chip(struct flash_chip *chip);
bool_t DFFSD_format_DBX(u32 fmt_para1,u32 fmt_para2,
                        struct st_DBX_device_tag *DBX_device_tag);
uint32_t __DFFSD_locate_block(struct st_DBX_flash_tag *DBX_flash_tag,
                              struct file_rsc *fp,sint64_t offset);
uint32_t __DFFSD_last_block(struct st_DBX_flash_tag *DBX_flash_tag,
                            struct file_rsc *fp);
void __DFFSD_get_access_info(struct st_DBX_flash_tag *DBX_flash_tag,
                            struct file_rsc *fp,
                            struct file_access_tag *access_tag);
bool_t __DFFSD_sync_buf_after_write(struct st_DBX_flash_tag *DBX_flash_tag,
                               djyfs_file *fp,uint32_t write_len);
uint32_t __DFFSD_write_update_block(struct st_DBX_flash_tag *DBX_flash_tag,
                             uint32_t block_no,uint32_t offset,uint32_t size);
uint32_t __DFFSD_write_append_block(struct st_DBX_flash_tag *DBX_flash_tag,
                             uint32_t block_no,uint32_t offset,uint32_t size);
uint32_t __DFFSD_write_new_block(struct st_DBX_flash_tag *DBX_flash_tag,
                             uint32_t block_no,uint32_t offset,uint32_t size);
bool_t __DFFSD_move_file_read_ptr(struct st_DBX_flash_tag *DBX_flash_tag,
                            djyfs_file *fp,sint64_t new_ptr);
bool_t __DFFSD_move_file_write_ptr(struct st_DBX_flash_tag *DBX_flash_tag,
                             djyfs_file *fp,sint64_t new_ptr);
uint32_t __DFFSD_write_buf_to_flash(struct st_DBX_flash_tag *DBX_flash_tag,
                                    struct file_rsc *fp);
bool_t __DFFSD_sync_buf_before_read(struct st_DBX_flash_tag *DBX_flash_tag,
                              djyfs_file *fp,uint32_t read_len);
uint32_t DFFSD_write_file(struct file_rsc *fp,uint8_t *buf,uint32_t len);
uint32_t DFFSD_read_file(struct file_rsc *fp,uint8_t *buf,uint32_t len);
uint32_t DFFSD_flush_file(struct file_rsc *fp);
sint64_t DFFSD_query_file_stocks(struct file_rsc *fp);
sint64_t DFFSD_query_file_cubage(struct file_rsc *fp);
sint64_t __DFFSD_set_file_size(struct file_rsc *fp,sint64_t new_size);
uint32_t DFFSD_seek_file(struct file_rsc *fp,struct seek_para *position);
bool_t DFFSD_create_file(char *name,struct file_rsc *parent,
                         union file_attrs attr);
uint32_t DFFSD_check_file(struct file_rsc *parent);
bool_t DFFSD_remove_file(struct file_rsc *fp);
bool_t __DFFSD_cmp_file_name(char *fs_name,char *FDT_name);
uint32_t DFFSD_open_item(char *name,struct file_rsc *parent,
                       struct file_rsc *result,enum file_open_mode mode);
bool_t DFFSD_lookfor_item(char *name,struct file_rsc *parent,
                         struct file_rsc *result);
ptu32_t module_init_DFFSD(ptu32_t para);
bool_t DFFSD_close_item(struct file_rsc *fp);
bool_t DFFSD_rename_item(struct file_rsc *fp,char *newname);
bool_t __DFFSD_repair_FDT(struct st_DBX_flash_tag *DBX_flash_tag);
uint32_t DFFSD_check_FDT_size(struct st_DBX_device_tag *DBX_device_tag);
void DFFSD_read_FDT(struct st_DBX_device_tag *DBX_device_tag,uint8_t *buf);
void __DFFSD_fill_DBX_device_tag(struct st_DBX_device_tag *DBX_device_tag,
                                 char *name);
void __DFFSD_scan_chip(struct flash_chip *chip,uint32_t start_block_no);
bool_t DFFSD_install_chip(struct flash_chip *chip,char *name,uint32_t rev_blocks);
bool_t creat_DBX(char *DBX_name);

#endif //#if ((cfg_djyfs == 1) && (cfg_run_mode != cn_mode_tiny) && (cfg_djyfs_flash == 1))


#ifdef __cplusplus
}
#endif

#endif // _NANDFILE_H_

